/**CFile****************************************************************

  FileName    [satProof.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [SAT solver.]

  Synopsis    [Proof manipulation procedures.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: satProof.c,v 1.4 2005/09/16 22:55:03 casem Exp $]

***********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "misc/vec/vec.h"
#include "misc/vec/vecSet.h"
#include "aig/aig/aig.h"
#include "satTruth.h"

ABC_NAMESPACE_IMPL_START

/*
    Proof is represented as a vector of records.
    A resolution record is represented by a handle (an offset in this vector).
    A resolution record entry is <size><label><ant1><ant2>...<antN>
    Label is initialized to 0. Root clauses are given by their handles.
    They are marked by bitshifting by 2 bits up and setting the LSB to 1
*/

typedef struct satset_t satset;
struct satset_t 
{
    unsigned learnt :  1;
    unsigned mark   :  1;
    unsigned partA  :  1;
    unsigned nEnts  : 29;
    int      Id;
    int      pEnts[0];
};

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

//static inline satset* Proof_ClauseRead  ( Vec_Int_t* p, int h )     { assert( h > 0 );     return satset_read( (veci *)p, h );    }
//static inline satset* Proof_ClauseRead  ( Vec_Int_t* p, int h )     { assert( h > 0 );     return (satset *)Vec_IntEntryP( p, h );}
static inline satset* Proof_NodeRead    ( Vec_Set_t* p, int h )     { assert( h > 0 );     return (satset*)Vec_SetEntry( p, h );  }
static inline int     Proof_NodeWordNum ( int nEnts )               { assert( nEnts > 0 ); return 1 + ((nEnts + 1) >> 1);         }

void Proof_ClauseSetEnts( Vec_Set_t* p, int h, int nEnts )          { Proof_NodeRead(p, h)->nEnts = nEnts;             }

// iterating through nodes in the proof
#define Proof_ForeachClauseVec( pVec, p, pNode, i )          \
    for ( i = 0; (i < Vec_IntSize(pVec)) && ((pNode) = Proof_ClauseRead(p, Vec_IntEntry(pVec,i))); i++ )
#define Proof_ForeachNodeVec( pVec, p, pNode, i )            \
    for ( i = 0; (i < Vec_IntSize(pVec)) && ((pNode) = Proof_NodeRead(p, Vec_IntEntry(pVec,i))); i++ )
#define Proof_ForeachNodeVec1( pVec, p, pNode, i )            \
    for ( i = 1; (i < Vec_IntSize(pVec)) && ((pNode) = Proof_NodeRead(p, Vec_IntEntry(pVec,i))); i++ )

// iterating through fanins of a proof node
#define Proof_NodeForeachFanin( pProof, pNode, pFanin, i )        \
    for ( i = 0; (i < (int)pNode->nEnts) && (((pFanin) = (pNode->pEnts[i] & 1) ? NULL : Proof_NodeRead(pProof, pNode->pEnts[i] >> 2)), 1); i++ )
/*
#define Proof_NodeForeachLeaf( pClauses, pNode, pLeaf, i )   \
    for ( i = 0; (i < (int)pNode->nEnts) && (((pLeaf) = (pNode->pEnts[i] & 1) ? Proof_ClauseRead(pClauses, pNode->pEnts[i] >> 2) : NULL), 1); i++ )
#define Proof_NodeForeachFaninLeaf( pProof, pClauses, pNode, pFanin, i )    \
    for ( i = 0; (i < (int)pNode->nEnts) && ((pFanin) = (pNode->pEnts[i] & 1) ? Proof_ClauseRead(pClauses, pNode->pEnts[i] >> 2) : Proof_NodeRead(pProof, pNode->pEnts[i] >> 2)); i++ )
*/

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Cleans collected resultion nodes belonging to the proof.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Proof_CleanCollected( Vec_Set_t * vProof, Vec_Int_t * vUsed )
{
    satset * pNode;
    int hNode;
    Proof_ForeachNodeVec( vUsed, vProof, pNode, hNode )
        pNode->Id = 0;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Proof_CollectUsed_iter( Vec_Set_t * vProof, int hNode, Vec_Int_t * vUsed, Vec_Int_t * vStack )
{
    satset * pNext, * pNode = Proof_NodeRead( vProof, hNode );
    int i;
    if ( pNode->Id )
        return;
    // start with node
    pNode->Id = 1;
    Vec_IntPush( vStack, hNode << 1 );
    // perform DFS search
    while ( Vec_IntSize(vStack) )
    {
        hNode = Vec_IntPop( vStack );
        if ( hNode & 1 ) // extracted second time
        {
            Vec_IntPush( vUsed, hNode >> 1 );
            continue;
        }
        // extracted first time        
        Vec_IntPush( vStack, hNode ^ 1 ); // add second time
        // add its anticedents        ;
        pNode = Proof_NodeRead( vProof, hNode >> 1 );
        Proof_NodeForeachFanin( vProof, pNode, pNext, i )
            if ( pNext && !pNext->Id )
            {
                pNext->Id = 1;
                Vec_IntPush( vStack, (pNode->pEnts[i] >> 2) << 1 ); // add first time
            }
    }
}
Vec_Int_t * Proof_CollectUsedIter( Vec_Set_t * vProof, Vec_Int_t * vRoots, int fSort )
{
    int fVerify = 0;
    Vec_Int_t * vUsed, * vStack;
    abctime clk = Abc_Clock();
    int i, Entry, iPrev = 0;
    vUsed = Vec_IntAlloc( 1000 );
    vStack = Vec_IntAlloc( 1000 );
    Vec_IntForEachEntry( vRoots, Entry, i )
        if ( Entry >= 0 )
            Proof_CollectUsed_iter( vProof, Entry, vUsed, vStack );
    Vec_IntFree( vStack );
//    Abc_PrintTime( 1, "Iterative clause collection time", Abc_Clock() - clk );
    clk = Abc_Clock();
    Abc_MergeSort( Vec_IntArray(vUsed), Vec_IntSize(vUsed) );
//    Abc_PrintTime( 1, "Postprocessing with sorting time", Abc_Clock() - clk );
    // verify topological order
    if ( fVerify )
    {
        iPrev = 0;
        Vec_IntForEachEntry( vUsed, Entry, i )
        {
            if ( iPrev >= Entry )
                printf( "Out of topological order!!!\n" );
            assert( iPrev < Entry );
            iPrev = Entry;
        }
    }
    return vUsed;
}

/**Function*************************************************************

  Synopsis    [Recursively collects useful proof nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Proof_CollectUsed_rec( Vec_Set_t * vProof, int hNode, Vec_Int_t * vUsed )
{
    satset * pNext, * pNode = Proof_NodeRead( vProof, hNode );
    int i;
    if ( pNode->Id )
        return;
    pNode->Id = 1;
    Proof_NodeForeachFanin( vProof, pNode, pNext, i )
        if ( pNext && !pNext->Id )
            Proof_CollectUsed_rec( vProof, pNode->pEnts[i] >> 2, vUsed );
    Vec_IntPush( vUsed, hNode );
}
Vec_Int_t * Proof_CollectUsedRec( Vec_Set_t * vProof, Vec_Int_t * vRoots )
{
    Vec_Int_t * vUsed;
    int i, Entry;
    vUsed = Vec_IntAlloc( 1000 );
    Vec_IntForEachEntry( vRoots, Entry, i )
        if ( Entry >= 0 )
            Proof_CollectUsed_rec( vProof, Entry, vUsed );
    return vUsed;
}

/**Function*************************************************************

  Synopsis    [Recursively visits useful proof nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Proof_MarkUsed_rec( Vec_Set_t * vProof, int hNode )
{
    satset * pNext, * pNode = Proof_NodeRead( vProof, hNode );
    int i, Counter = 1;
    if ( pNode->Id )
        return 0;
    pNode->Id = 1;
    Proof_NodeForeachFanin( vProof, pNode, pNext, i )
        if ( pNext && !pNext->Id )
            Counter += Proof_MarkUsed_rec( vProof, pNode->pEnts[i] >> 2 );
    return Counter;
}
int Proof_MarkUsedRec( Vec_Set_t * vProof, Vec_Int_t * vRoots )
{
    int i, Entry, Counter = 0;
    Vec_IntForEachEntry( vRoots, Entry, i )
        if ( Entry >= 0 )
            Counter += Proof_MarkUsed_rec( vProof, Entry );
    return Counter;
}



  
/**Function*************************************************************

  Synopsis    [Checks the validity of the check point.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
/*
void Sat_ProofReduceCheck_rec( Vec_Set_t * vProof, Vec_Int_t * vClauses, satset * pNode, int hClausePivot, Vec_Ptr_t * vVisited )
{
    satset * pFanin;
    int k;
    if ( pNode->Id )
        return;
    pNode->Id = -1;
    Proof_NodeForeachFaninLeaf( vProof, vClauses, pNode, pFanin, k )
        if ( (pNode->pEnts[k] & 1) == 0 ) // proof node
            Sat_ProofReduceCheck_rec( vProof, vClauses, pFanin, hClausePivot, vVisited );
        else // problem clause
            assert( (pNode->pEnts[k] >> 2) < hClausePivot );
    Vec_PtrPush( vVisited, pNode );
}
void Sat_ProofReduceCheckOne( sat_solver2 * s, int iLearnt, Vec_Ptr_t * vVisited )
{
    Vec_Set_t * vProof   = (Vec_Set_t *)&s->Proofs;
    Vec_Int_t * vClauses = (Vec_Int_t *)&s->clauses;
    Vec_Int_t * vRoots   = (Vec_Int_t *)&s->claProofs;
    int hProofNode = Vec_IntEntry( vRoots, iLearnt );
    satset * pNode = Proof_NodeRead( vProof, hProofNode );
    Sat_ProofReduceCheck_rec( vProof, vClauses, pNode, s->hClausePivot, vVisited );
}
void Sat_ProofReduceCheck( sat_solver2 * s )
{
    Vec_Ptr_t * vVisited;
    satset * c;
    int h, i = 1;
    vVisited = Vec_PtrAlloc( 1000 );
    sat_solver_foreach_learnt( s, c, h )
        if ( h < s->hLearntPivot )
            Sat_ProofReduceCheckOne( s, i++, vVisited );
    Vec_PtrForEachEntry( satset *, vVisited, c, i )
        c->Id = 0;
    Vec_PtrFree( vVisited );
}
*/

/**Function*************************************************************

  Synopsis    [Reduces the proof to contain only roots and their children.]

  Description [The result is updated proof and updated roots.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
/*
void Sat_ProofReduce2( sat_solver2 * s )
{
    Vec_Set_t * vProof   = (Vec_Set_t *)&s->Proofs;
    Vec_Int_t * vRoots   = (Vec_Int_t *)&s->claProofs;
    Vec_Int_t * vClauses = (Vec_Int_t *)&s->clauses;

    int fVerbose = 0;
    Vec_Int_t * vUsed;
    satset * pNode, * pFanin, * pPivot;
    int i, k, hTemp;
    abctime clk = Abc_Clock();
    static abctime TimeTotal = 0;

    // collect visited nodes
    vUsed = Proof_CollectUsedIter( vProof, vRoots, 1 );

    // relabel nodes to use smaller space
    Vec_SetShrinkS( vProof, 2 );
    Proof_ForeachNodeVec( vUsed, vProof, pNode, i )
    {
        pNode->Id = Vec_SetAppendS( vProof, 2+pNode->nEnts );
        Proof_NodeForeachFaninLeaf( vProof, vClauses, pNode, pFanin, k )
            if ( (pNode->pEnts[k] & 1) == 0 ) // proof node
                pNode->pEnts[k] = (pFanin->Id << 2) | (pNode->pEnts[k] & 2);
            else // problem clause
                assert( (int*)pFanin >= Vec_IntArray(vClauses) && (int*)pFanin < Vec_IntArray(vClauses)+Vec_IntSize(vClauses) );
    }
    // update roots
    Proof_ForeachNodeVec1( vRoots, vProof, pNode, i )
        Vec_IntWriteEntry( vRoots, i, pNode->Id );
    // determine new pivot
    assert( s->hProofPivot >= 1 && s->hProofPivot <= Vec_SetHandCurrent(vProof) );
    pPivot = Proof_NodeRead( vProof, s->hProofPivot );
    s->hProofPivot = Vec_SetHandCurrentS(vProof);
    // compact the nodes
    Proof_ForeachNodeVec( vUsed, vProof, pNode, i )
    {
        hTemp = pNode->Id; pNode->Id = 0;
        memmove( Vec_SetEntry(vProof, hTemp), pNode, sizeof(word)*Proof_NodeWordNum(pNode->nEnts) );
        if ( pPivot && pPivot <= pNode )
        { 
            s->hProofPivot = hTemp;
            pPivot = NULL;
        }
    }
    Vec_SetWriteEntryNum( vProof, Vec_IntSize(vUsed) );
    Vec_IntFree( vUsed );

    // report the result
    if ( fVerbose )
    {
        printf( "\n" );
        printf( "The proof was reduced from %6.2f MB to %6.2f MB (by %6.2f %%)  ", 
            1.0 * Vec_SetMemory(vProof) / (1<<20), 1.0 * Vec_SetMemoryS(vProof) / (1<<20), 
            100.0 * (Vec_SetMemory(vProof) - Vec_SetMemoryS(vProof)) / Vec_SetMemory(vProof) );
        TimeTotal += Abc_Clock() - clk;
        Abc_PrintTime( 1, "Time", TimeTotal );
    }
    Vec_SetShrink( vProof, Vec_SetHandCurrentS(vProof) );
//    Sat_ProofReduceCheck( s );
} 
*/


void Sat_ProofCheck0( Vec_Set_t * vProof )
{
    satset * pNode, * pFanin;
    int i, j, k, nSize;
    Vec_SetForEachEntry( satset *, vProof, nSize, pNode, i, j )
    {
        nSize = Vec_SetWordNum( 2 + pNode->nEnts );
        Proof_NodeForeachFanin( vProof, pNode, pFanin, k )
            assert( (pNode->pEnts[k] >> 2) );
    }
}

int Sat_ProofReduce( Vec_Set_t * vProof, void * pRoots, int hProofPivot )
{
//    Vec_Set_t * vProof   = (Vec_Set_t *)&s->Proofs;
//    Vec_Int_t * vRoots   = (Vec_Int_t *)&s->claProofs;
    Vec_Int_t * vRoots   = (Vec_Int_t *)pRoots;
//    Vec_Int_t * vClauses = (Vec_Int_t *)&s->clauses;
    int fVerbose = 0;
    Vec_Ptr_t * vUsed;
    satset * pNode, * pFanin, * pPivot;
    int i, j, k, hTemp, nSize;
    abctime clk = Abc_Clock();
    static abctime TimeTotal = 0;
    int RetValue;
//Sat_ProofCheck0( vProof );

    // collect visited nodes
    nSize = Proof_MarkUsedRec( vProof, vRoots );
    vUsed = Vec_PtrAlloc( nSize );
//Sat_ProofCheck0( vProof );

    // relabel nodes to use smaller space
    Vec_SetShrinkS( vProof, 2 );
    Vec_SetForEachEntry( satset *, vProof, nSize, pNode, i, j )
    {
        nSize = Vec_SetWordNum( 2 + pNode->nEnts );
        if ( pNode->Id == 0 ) 
            continue;
        pNode->Id = Vec_SetAppendS( vProof, 2 + pNode->nEnts );
        assert( pNode->Id > 0 );
        Vec_PtrPush( vUsed, pNode );
        // update fanins
        Proof_NodeForeachFanin( vProof, pNode, pFanin, k )
            if ( (pNode->pEnts[k] & 1) == 0 ) // proof node
            {
                assert( pFanin->Id > 0 );
                pNode->pEnts[k] = (pFanin->Id << 2) | (pNode->pEnts[k] & 2);
            }
//            else // problem clause
//                assert( (int*)pFanin >= Vec_IntArray(vClauses) && (int*)pFanin < Vec_IntArray(vClauses)+Vec_IntSize(vClauses) );
    }
    // update roots
    Proof_ForeachNodeVec1( vRoots, vProof, pNode, i )
        Vec_IntWriteEntry( vRoots, i, pNode->Id );
    // determine new pivot
    assert( hProofPivot >= 1 && hProofPivot <= Vec_SetHandCurrent(vProof) );
    pPivot = Proof_NodeRead( vProof, hProofPivot );
    RetValue = Vec_SetHandCurrentS(vProof);
    // compact the nodes
    Vec_PtrForEachEntry( satset *, vUsed, pNode, i )
    {
        hTemp = pNode->Id; pNode->Id = 0;
        memmove( Vec_SetEntry(vProof, hTemp), pNode, sizeof(word)*Proof_NodeWordNum(pNode->nEnts) );
        if ( pPivot && pPivot <= pNode )
        {
            RetValue = hTemp;
            pPivot = NULL;
        }
    }
    Vec_SetWriteEntryNum( vProof, Vec_PtrSize(vUsed) );
    Vec_PtrFree( vUsed );

    // report the result
    if ( fVerbose )
    {
        printf( "\n" );
        printf( "The proof was reduced from %6.2f MB to %6.2f MB (by %6.2f %%)  ", 
            1.0 * Vec_SetMemory(vProof) / (1<<20), 1.0 * Vec_SetMemoryS(vProof) / (1<<20), 
            100.0 * (Vec_SetMemory(vProof) - Vec_SetMemoryS(vProof)) / Vec_SetMemory(vProof) );
        TimeTotal += Abc_Clock() - clk;
        Abc_PrintTime( 1, "Time", TimeTotal );
    }
    Vec_SetShrink( vProof, Vec_SetHandCurrentS(vProof) );
    Vec_SetShrinkLimits( vProof );
//    Sat_ProofReduceCheck( s );
//Sat_ProofCheck0( vProof );

    return RetValue;
}

#if 0

/**Function*************************************************************

  Synopsis    [Performs one resultion step.]

  Description [Returns ID of the resolvent if success, and -1 if failure.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Sat_ProofCheckResolveOne( Vec_Set_t * p, satset * c1, satset * c2, Vec_Int_t * vTemp )
{
    satset * c;
    int h, i, k, Var = -1, Count = 0;
    // find resolution variable
    for ( i = 0; i < (int)c1->nEnts; i++ )
    for ( k = 0; k < (int)c2->nEnts; k++ )
        if ( (c1->pEnts[i] ^ c2->pEnts[k]) == 1 )
        {
            Var = (c1->pEnts[i] >> 1);
            Count++;
        }
    if ( Count == 0 )
    {
        printf( "Cannot find resolution variable\n" );
        return 0;
    }
    if ( Count > 1 )
    {
        printf( "Found more than 1 resolution variables\n" );
        return 0;
    }
    // perform resolution
    Vec_IntClear( vTemp );
    Vec_IntPush( vTemp, 0 ); // placeholder
    Vec_IntPush( vTemp, 0 );
    for ( i = 0; i < (int)c1->nEnts; i++ )
        if ( (c1->pEnts[i] >> 1) != Var )
            Vec_IntPush( vTemp, c1->pEnts[i] );
    for ( i = 0; i < (int)c2->nEnts; i++ )
        if ( (c2->pEnts[i] >> 1) != Var )
            Vec_IntPushUnique( vTemp, c2->pEnts[i] );
    // create new resolution entry
    h = Vec_SetAppend( p, Vec_IntArray(vTemp), Vec_IntSize(vTemp) );
    // return the new entry
    c = Proof_NodeRead( p, h );
    c->nEnts = Vec_IntSize(vTemp)-2;
    return h;
}

/**Function*************************************************************

  Synopsis    [Checks the proof for consitency.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
satset * Sat_ProofCheckReadOne( Vec_Int_t * vClauses, Vec_Set_t * vProof, Vec_Set_t * vResolves, int iAnt )
{
    satset * pAnt;
    if ( iAnt & 1 )
        return Proof_ClauseRead( vClauses, iAnt >> 2 );
    assert( iAnt > 0 );
    pAnt = Proof_NodeRead( vProof, iAnt >> 2 );
    assert( pAnt->Id > 0 );
    return Proof_NodeRead( vResolves, pAnt->Id );
}

/**Function*************************************************************

  Synopsis    [Checks the proof for consitency.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sat_ProofCheck( sat_solver2 * s )
{
    Vec_Int_t * vClauses = (Vec_Int_t *)&s->clauses;
    Vec_Set_t * vProof   = (Vec_Set_t *)&s->Proofs;
    Vec_Int_t Roots = { 1, 1, &s->hProofLast }, * vRoots = &Roots;
    Vec_Set_t * vResolves;
    Vec_Int_t * vUsed, * vTemp;
    satset * pSet, * pSet0 = NULL, * pSet1;
    int i, k, hRoot, Handle, Counter = 0;
    abctime clk = Abc_Clock(); 
    hRoot = s->hProofLast;
    if ( hRoot == -1 )
        return;
    // collect visited clauses
    vUsed = Proof_CollectUsedIter( vProof, vRoots, 1 );
    Proof_CleanCollected( vProof, vUsed );
    // perform resolution steps
    vTemp = Vec_IntAlloc( 1000 );
    vResolves = Vec_SetAlloc( 20 );
    Proof_ForeachNodeVec( vUsed, vProof, pSet, i )
    {
        Handle = -1;
        pSet0 = Sat_ProofCheckReadOne( vClauses, vProof, vResolves, pSet->pEnts[0] );
        for ( k = 1; k < (int)pSet->nEnts; k++ )
        {
            pSet1  = Sat_ProofCheckReadOne( vClauses, vProof, vResolves, pSet->pEnts[k] );
            Handle = Sat_ProofCheckResolveOne( vResolves, pSet0, pSet1, vTemp );
            pSet0  = Proof_NodeRead( vResolves, Handle );
        }
        pSet->Id = Handle;
        Counter++;
    }
    Vec_IntFree( vTemp );
    // clean the proof
    Proof_CleanCollected( vProof, vUsed );
    // compare the final clause
    printf( "Used %6.2f MB for resolvents.\n", 1.0 * Vec_SetMemory(vResolves) / (1<<20) );
    if ( pSet0->nEnts > 0 )
        printf( "Derived clause with %d lits instead of the empty clause.  ", pSet0->nEnts );
    else
        printf( "Proof verification successful.  " );
    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
    // cleanup
    Vec_SetFree( vResolves );
    Vec_IntFree( vUsed );
}
#endif

/**Function*************************************************************

  Synopsis    [Collects nodes belonging to the UNSAT core.]

  Description [The resulting array contains 1-based IDs of root clauses.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Sat_ProofCollectCore( Vec_Set_t * vProof, Vec_Int_t * vUsed )
{
    Vec_Int_t * vCore;
    satset * pNode, * pFanin;
    unsigned * pBitMap;
    int i, k, MaxCla = 0;
    // find the largest number
    Proof_ForeachNodeVec( vUsed, vProof, pNode, i )
        Proof_NodeForeachFanin( vProof, pNode, pFanin, k )
            if ( pFanin == NULL )
                MaxCla = Abc_MaxInt( MaxCla, pNode->pEnts[k] >> 2 );
    // allocate bitmap
    pBitMap = ABC_CALLOC( unsigned, Abc_BitWordNum(MaxCla) + 1 );
    // collect leaves
    vCore = Vec_IntAlloc( 1000 );
    Proof_ForeachNodeVec( vUsed, vProof, pNode, i )
        Proof_NodeForeachFanin( vProof, pNode, pFanin, k )
            if ( pFanin == NULL )
            {
                int Entry = (pNode->pEnts[k] >> 2);
                if ( Abc_InfoHasBit(pBitMap, Entry) )
                    continue;
                Abc_InfoSetBit(pBitMap, Entry);
                Vec_IntPush( vCore, Entry );
            }
    ABC_FREE( pBitMap );
//    Vec_IntUniqify( vCore );
    return vCore;
}

#if 0

/**Function*************************************************************

  Synopsis    [Verifies that variables are labeled correctly.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sat_ProofInterpolantCheckVars( sat_solver2 * s, Vec_Int_t * vGloVars )
{
    satset* c; 
    Vec_Int_t * vVarMap;
    int i, k, Entry, * pMask;
    int Counts[5] = {0};
    // map variables into their type (A, B, or AB)
    vVarMap = Vec_IntStart( s->size );
    sat_solver_foreach_clause( s, c, i )
        for ( k = 0; k < (int)c->nEnts; k++ )
            *Vec_IntEntryP(vVarMap, lit_var(c->pEnts[k])) |= 2 - c->partA;
    // analyze variables
    Vec_IntForEachEntry( vGloVars, Entry, i )
    {
        pMask = Vec_IntEntryP(vVarMap, Entry);
        assert( *pMask >= 0 && *pMask <= 3 );
        Counts[(*pMask & 3)]++;
        *pMask = 0;
    }
    // count the number of global variables not listed
    Vec_IntForEachEntry( vVarMap, Entry, i )
        if ( Entry == 3 )
            Counts[4]++;
    Vec_IntFree( vVarMap );
    // report
    if ( Counts[0] )
        printf( "Warning: %6d variables listed as global do not appear in clauses (this is normal)\n", Counts[0] );
    if ( Counts[1] )
        printf( "Warning: %6d variables listed as global appear only in A-clauses (this is a BUG)\n", Counts[1] );
    if ( Counts[2] )
        printf( "Warning: %6d variables listed as global appear only in B-clauses (this is a BUG)\n", Counts[2] );
    if ( Counts[3] )
        printf( "Warning: %6d (out of %d) variables listed as global appear in both A- and B-clauses (this is normal)\n", Counts[3], Vec_IntSize(vGloVars) );
    if ( Counts[4] )
        printf( "Warning: %6d variables not listed as global appear in both A- and B-clauses (this is a BUG)\n", Counts[4] );
}

/**Function*************************************************************

  Synopsis    [Computes interpolant of the proof.]

  Description [Aassuming that vars/clause of partA are marked.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void * Sat_ProofInterpolant( sat_solver2 * s, void * pGloVars )
{
    Vec_Int_t * vClauses  = (Vec_Int_t *)&s->clauses;
    Vec_Set_t * vProof    = (Vec_Set_t *)&s->Proofs;
    Vec_Int_t * vGlobVars = (Vec_Int_t *)pGloVars;
    Vec_Int_t Roots = { 1, 1, &s->hProofLast }, * vRoots = &Roots;
    Vec_Int_t * vUsed, * vCore, * vCoreNums, * vVarMap;
    satset * pNode, * pFanin;
    Aig_Man_t * pAig;
    Aig_Obj_t * pObj = NULL;
    int i, k, iVar, Lit, Entry, hRoot;
//    if ( s->hLearntLast < 0 )
//        return NULL;
//    hRoot = veci_begin(&s->claProofs)[satset_read(&s->learnts, s->hLearntLast>>1)->Id];
    hRoot = s->hProofLast;
    if ( hRoot == -1 )
        return NULL;

    Sat_ProofInterpolantCheckVars( s, vGlobVars );

    // collect visited nodes
    vUsed = Proof_CollectUsedIter( vProof, vRoots, 1 );
    // collect core clauses (cleans vUsed and vCore)
    vCore = Sat_ProofCollectCore( vProof, vUsed ); 
    // vCore arrived in terms of clause handles

    // map variables into their global numbers
    vVarMap = Vec_IntStartFull( s->size );
    Vec_IntForEachEntry( vGlobVars, Entry, i )
        Vec_IntWriteEntry( vVarMap, Entry, i );

    // start the AIG
    pAig = Aig_ManStart( 10000 );
    pAig->pName = Abc_UtilStrsav( "interpol" );
    for ( i = 0; i < Vec_IntSize(vGlobVars); i++ )
        Aig_ObjCreateCi( pAig );

    // copy the numbers out and derive interpol for clause
    vCoreNums = Vec_IntAlloc( Vec_IntSize(vCore) );
    Proof_ForeachClauseVec( vCore, vClauses, pNode, i )
    {
        if ( pNode->partA )
        {
            pObj = Aig_ManConst0( pAig );
            satset_foreach_lit( pNode, Lit, k, 0 )
                if ( (iVar = Vec_IntEntry(vVarMap, lit_var(Lit))) >= 0 )
                    pObj = Aig_Or( pAig, pObj, Aig_NotCond(Aig_IthVar(pAig, iVar), lit_sign(Lit)) );
        }
        else
            pObj = Aig_ManConst1( pAig );
        // remember the interpolant
        Vec_IntPush( vCoreNums, pNode->Id );
        pNode->Id = Aig_ObjToLit(pObj);
    }
    Vec_IntFree( vVarMap );

    // copy the numbers out and derive interpol for resolvents
    Proof_ForeachNodeVec( vUsed, vProof, pNode, i )
    {
//        satset_print( pNode );
        assert( pNode->nEnts > 1 );
        Proof_NodeForeachFaninLeaf( vProof, vClauses, pNode, pFanin, k )
        {
            assert( pFanin->Id < 2*Aig_ManObjNumMax(pAig) );
            if ( k == 0 )
                pObj = Aig_ObjFromLit( pAig, pFanin->Id );
            else if ( pNode->pEnts[k] & 2 ) // variable of A
                pObj = Aig_Or( pAig, pObj, Aig_ObjFromLit(pAig, pFanin->Id) );
            else
                pObj = Aig_And( pAig, pObj, Aig_ObjFromLit(pAig, pFanin->Id) );
        }
        // remember the interpolant
        pNode->Id = Aig_ObjToLit(pObj);
    }
    // save the result
//    assert( Proof_NodeHandle(vProof, pNode) == hRoot );
    Aig_ObjCreateCo( pAig, pObj );
    Aig_ManCleanup( pAig );

    // move the results back
    Proof_ForeachClauseVec( vCore, vClauses, pNode, i )
        pNode->Id = Vec_IntEntry( vCoreNums, i );
    Proof_ForeachNodeVec( vUsed, vProof, pNode, i )
        pNode->Id = 0;
    // cleanup
    Vec_IntFree( vCore );
    Vec_IntFree( vUsed );
    Vec_IntFree( vCoreNums );
    return pAig;
}


/**Function*************************************************************

  Synopsis    [Computes interpolant of the proof.]

  Description [Aassuming that vars/clause of partA are marked.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
word * Sat_ProofInterpolantTruth( sat_solver2 * s, void * pGloVars )
{
    Vec_Int_t * vClauses  = (Vec_Int_t *)&s->clauses;
    Vec_Set_t * vProof    = (Vec_Set_t *)&s->Proofs;
    Vec_Int_t * vGlobVars = (Vec_Int_t *)pGloVars;
    Vec_Int_t Roots = { 1, 1, &s->hProofLast }, * vRoots = &Roots;
    Vec_Int_t * vUsed, * vCore, * vCoreNums, * vVarMap;
    satset * pNode, * pFanin;
    Tru_Man_t * pTru;
    int nVars = Vec_IntSize(vGlobVars);
    int nWords = (nVars < 6) ? 1 : (1 << (nVars-6));
    word * pRes = ABC_ALLOC( word, nWords );
    int i, k, iVar, Lit, Entry, hRoot;
    assert( nVars > 0 && nVars <= 16 );
    hRoot = s->hProofLast;
    if ( hRoot == -1 )
        return NULL;

    Sat_ProofInterpolantCheckVars( s, vGlobVars );

    // collect visited nodes
    vUsed = Proof_CollectUsedIter( vProof, vRoots, 1 );
    // collect core clauses (cleans vUsed and vCore)
    vCore = Sat_ProofCollectCore( vProof, vUsed, 0 );
    // vCore arrived in terms of clause handles

    // map variables into their global numbers
    vVarMap = Vec_IntStartFull( s->size );
    Vec_IntForEachEntry( vGlobVars, Entry, i )
        Vec_IntWriteEntry( vVarMap, Entry, i );

    // start the AIG
    pTru = Tru_ManAlloc( nVars );

    // copy the numbers out and derive interpol for clause
    vCoreNums = Vec_IntAlloc( Vec_IntSize(vCore) );
    Proof_ForeachClauseVec( vCore, vClauses, pNode, i )
    {
        if ( pNode->partA )
        {
//            pObj = Aig_ManConst0( pAig );
            Tru_ManClear( pRes, nWords );
            satset_foreach_lit( pNode, Lit, k, 0 )
                if ( (iVar = Vec_IntEntry(vVarMap, lit_var(Lit))) >= 0 )
//                    pObj = Aig_Or( pAig, pObj, Aig_NotCond(Aig_IthVar(pAig, iVar), lit_sign(Lit)) );
                    pRes = Tru_ManOrNotCond( pRes, Tru_ManVar(pTru, iVar), nWords, lit_sign(Lit) );
        }
        else
//            pObj = Aig_ManConst1( pAig );
            Tru_ManFill( pRes, nWords );
        // remember the interpolant
        Vec_IntPush( vCoreNums, pNode->Id );
//        pNode->Id = Aig_ObjToLit(pObj);
        pNode->Id = Tru_ManInsert( pTru, pRes );
    }
    Vec_IntFree( vVarMap );

    // copy the numbers out and derive interpol for resolvents
    Proof_ForeachNodeVec( vUsed, vProof, pNode, i )
    {
//        satset_print( pNode );
        assert( pNode->nEnts > 1 );
        Proof_NodeForeachFaninLeaf( vProof, vClauses, pNode, pFanin, k )
        {
//            assert( pFanin->Id < 2*Aig_ManObjNumMax(pAig) );
//            assert( pFanin->Id <= Tru_ManHandleMax(pTru) );
            if ( k == 0 )
//                pObj = Aig_ObjFromLit( pAig, pFanin->Id );
                pRes = Tru_ManCopyNotCond( pRes, Tru_ManFunc(pTru, pFanin->Id & ~1), nWords, pFanin->Id & 1 );
            else if ( pNode->pEnts[k] & 2 ) // variable of A
//                pObj = Aig_Or( pAig, pObj, Aig_ObjFromLit(pAig, pFanin->Id) );
                pRes = Tru_ManOrNotCond( pRes, Tru_ManFunc(pTru, pFanin->Id & ~1), nWords, pFanin->Id & 1 );
            else
//                pObj = Aig_And( pAig, pObj, Aig_ObjFromLit(pAig, pFanin->Id) );
                pRes = Tru_ManAndNotCond( pRes, Tru_ManFunc(pTru, pFanin->Id & ~1), nWords, pFanin->Id & 1 );
        }
        // remember the interpolant
//        pNode->Id = Aig_ObjToLit(pObj);
        pNode->Id = Tru_ManInsert( pTru, pRes );
    }
    // save the result
//    assert( Proof_NodeHandle(vProof, pNode) == hRoot );
//    Aig_ObjCreateCo( pAig, pObj );
//    Aig_ManCleanup( pAig );

    // move the results back
    Proof_ForeachClauseVec( vCore, vClauses, pNode, i )
        pNode->Id = Vec_IntEntry( vCoreNums, i );
    Proof_ForeachNodeVec( vUsed, vProof, pNode, i )
        pNode->Id = 0;
    // cleanup
    Vec_IntFree( vCore );
    Vec_IntFree( vUsed );
    Vec_IntFree( vCoreNums );
    Tru_ManFree( pTru );
//    ABC_FREE( pRes );
    return pRes;
}

#endif

/**Function*************************************************************

  Synopsis    [Computes UNSAT core.]

  Description [The result is the array of root clause indexes.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void * Proof_DeriveCore( Vec_Set_t * vProof, int hRoot )
{
    Vec_Int_t Roots = { 1, 1, &hRoot }, * vRoots = &Roots;
    Vec_Int_t * vCore, * vUsed;
    if ( hRoot == -1 )
        return NULL;
    // collect visited clauses
    vUsed = Proof_CollectUsedIter( vProof, vRoots, 0 );
    // collect core clauses 
    vCore = Sat_ProofCollectCore( vProof, vUsed );
    Vec_IntFree( vUsed );
    Vec_IntSort( vCore, 1 );
    return vCore;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

ABC_NAMESPACE_IMPL_END

