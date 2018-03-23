/**CFile****************************************************************

  FileName    [kitDec.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Computation kit.]

  Synopsis    [Decomposition manager.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - November 18, 2009.]

  Revision    [$Id: kitDec.c,v 1.00 2006/12/06 00:00:00 alanmi Exp $]

***********************************************************************/

#include "kit.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

// decomposition manager
typedef struct Kit_ManDec_t_ Kit_ManDec_t;
struct Kit_ManDec_t_ 
{
    int             nVarsMax;     // the max number of variables
    int             nWordsMax;    // the max number of words
    Vec_Ptr_t *     vTruthVars;   // elementary truth tables
    Vec_Ptr_t *     vTruthNodes;  // internal truth tables
    // current problem
    int             nVarsIn;      // the current number of variables
    Vec_Int_t *     vLutsIn;      // LUT truth tables
    Vec_Int_t *     vSuppIn;      // LUT supports
    char            ATimeIn[64];  // variable arrival times
    // extracted information
    unsigned *      pTruthIn;     // computed truth table
    unsigned *      pTruthOut;    // computed truth table
    int             nVarsOut;     // the current number of variables
    int             nWordsOut;    // the current number of words
    char            Order[32];    // new vars into old vars after supp minimization
    // computed information
    Vec_Int_t *     vLutsOut;     // problem decomposition
    Vec_Int_t *     vSuppOut;     // problem decomposition
    char            ATimeOut[64]; // variable arrival times
};

static inline int   Kit_DecOuputArrival( int nVars, Vec_Int_t * vLuts, char ATimes[] ) { return ATimes[nVars + Vec_IntSize(vLuts) - 1]; }

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Starts Decmetry manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Kit_ManDec_t * Kit_ManDecStart( int nVarsMax )
{
    Kit_ManDec_t * p;
    assert( nVarsMax <= 20 );
    p = ABC_CALLOC( Kit_ManDec_t, 1 );
    p->nVarsMax   = nVarsMax;
    p->nWordsMax  = Kit_TruthWordNum( p->nVarsMax );
    p->vTruthVars = Vec_PtrAllocTruthTables( p->nVarsMax );
    p->vTruthNodes = Vec_PtrAllocSimInfo( 64, p->nWordsMax );
    p->vLutsIn    = Vec_IntAlloc( 50 );
    p->vSuppIn    = Vec_IntAlloc( 50 );
    p->vLutsOut   = Vec_IntAlloc( 50 );
    p->vSuppOut   = Vec_IntAlloc( 50 );
    p->pTruthIn   = ABC_ALLOC( unsigned, p->nWordsMax );
    p->pTruthOut  = ABC_ALLOC( unsigned, p->nWordsMax );
    return p;
}

/**Function*************************************************************

  Synopsis    [Stops Decmetry manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Kit_ManDecStop( Kit_ManDec_t * p )
{
    ABC_FREE( p->pTruthIn );
    ABC_FREE( p->pTruthOut );
    Vec_IntFreeP( &p->vLutsIn );
    Vec_IntFreeP( &p->vSuppIn );
    Vec_IntFreeP( &p->vLutsOut );
    Vec_IntFreeP( &p->vSuppOut );
    Vec_PtrFreeP( &p->vTruthVars );
    Vec_PtrFreeP( &p->vTruthNodes );
    ABC_FREE( p );
}


/**Function*************************************************************

  Synopsis    [Deriving timing information for the decomposed structure.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Kit_DecComputeOuputArrival( int nVars, Vec_Int_t * vSupps, int LutSize, char ATimesIn[], char ATimesOut[] )
{
    int i, v, iVar, nLuts, Delay;
    nLuts = Vec_IntSize(vSupps) / LutSize;
    assert( nLuts > 0 );
    assert( Vec_IntSize(vSupps) % LutSize == 0 );
    for ( v = 0; v < nVars; v++ )
        ATimesOut[v] = ATimesIn[v];
    for ( v = 0; v < nLuts; v++ )
    {
        Delay = 0;
        for ( i = 0; i < LutSize; i++ )
        {
            iVar = Vec_IntEntry( vSupps, v * LutSize + i );
            assert( iVar < nVars + v );
            Delay = Abc_MaxInt( Delay, ATimesOut[iVar] );
        }
        ATimesOut[nVars + v] = Delay + 1;
    }
    return ATimesOut[nVars + nLuts - 1];
}

/**Function*************************************************************

  Synopsis    [Derives the truth table]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Kit_DecComputeTruthOne( int LutSize, unsigned * pTruthLut, int nVars, unsigned * pTruths[], unsigned * pTemp, unsigned * pRes )
{
    int i, v;
    Kit_TruthClear( pRes, nVars );
    for ( i = 0; i < (1<<LutSize); i++ )
    {
        if ( !Kit_TruthHasBit( pTruthLut, i ) )
            continue;
        Kit_TruthFill( pTemp, nVars );
        for ( v = 0; v < LutSize; v++ )
            if ( i & (1<<v) )
                Kit_TruthAnd( pTemp, pTemp, pTruths[v], nVars );
            else
                Kit_TruthSharp( pTemp, pTemp, pTruths[v], nVars );
        Kit_TruthOr( pRes, pRes, pTemp, nVars );
    }
}

/**Function*************************************************************

  Synopsis    [Derives the truth table]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Kit_DecComputeTruth( Kit_ManDec_t * p, int nVars, Vec_Int_t * vSupps, int LutSize, Vec_Int_t * vLuts, unsigned * pRes )
{
    unsigned * pResult, * pTruthLuts, * pTruths[17];
    int nTruthLutWords, i, v, iVar, nLuts;
    nLuts = Vec_IntSize(vSupps) / LutSize;
    pTruthLuts = (unsigned *)Vec_IntArray( vLuts );
    nTruthLutWords = Kit_TruthWordNum( LutSize );
    assert( nLuts > 0 );
    assert( Vec_IntSize(vSupps) % LutSize == 0 );
    assert( nLuts * nTruthLutWords == Vec_IntSize(vLuts) );
    for ( v = 0; v < nLuts; v++ )
    {
        for ( i = 0; i < LutSize; i++ )
        {
            iVar = Vec_IntEntry( vSupps, v * LutSize + i );
            assert( iVar < nVars + v );
            pTruths[i] = (iVar < nVars)? (unsigned *)Vec_PtrEntry(p->vTruthVars, iVar) : (unsigned *)Vec_PtrEntry(p->vTruthNodes, iVar-nVars);
        }
        pResult = (v == nLuts - 1) ? pRes : (unsigned *)Vec_PtrEntry(p->vTruthNodes, v);
        Kit_DecComputeTruthOne( LutSize, pTruthLuts, nVars, pTruths, (unsigned *)Vec_PtrEntry(p->vTruthNodes, v+1), pResult );
        pTruthLuts += nTruthLutWords;
    }
}

/**Function*************************************************************

  Synopsis    [Derives the truth table]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Kit_DecComputePattern( int nVars, unsigned * pTruth, int LutSize, int Pattern[] )
{
    int nCofs = (1 << LutSize);
    int i, k, nMyu = 0;
    assert( LutSize <= 6 );
    assert( LutSize < nVars );
    if ( nVars - LutSize <= 5 )
    {
        unsigned uCofs[64];
        int nBits = (1 << (nVars - LutSize));
        for ( i = 0; i < nCofs; i++ )
            uCofs[i] = (pTruth[(i*nBits)/32] >> ((i*nBits)%32)) & ((1<<nBits)-1);
        for ( i = 0; i < nCofs; i++ )
        {
            for ( k = 0; k < nMyu; k++ )
                if ( uCofs[i] == uCofs[k] )
                {
                    Pattern[i] = k;
                    break;
                }
            if ( k == i )
                Pattern[nMyu++] = i;
        }
    }
    else
    {
        unsigned * puCofs[64];
        int nWords = (1 << (nVars - LutSize - 5));
        for ( i = 0; i < nCofs; i++ )
            puCofs[i] = pTruth + nWords;
        for ( i = 0; i < nCofs; i++ )
        {
            for ( k = 0; k < nMyu; k++ )
                if ( Kit_TruthIsEqual( puCofs[i], puCofs[k], nVars - LutSize - 5 ) )
                {
                    Pattern[i] = k;
                    break;
                }
            if ( k == i )
                Pattern[nMyu++] = i;
        }
    }
    return nMyu;
}

/**Function*************************************************************

  Synopsis    [Returns the number of shared variables.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Kit_DecComputeShared_rec( int Pattern[], int Vars[], int nVars, int Shared[], int iVarTry )
{
    int Pat0[32], Pat1[32], Shared0[5], Shared1[5], VarsNext[5];
    int v, u, iVarsNext, iPat0, iPat1, m, nMints = (1 << nVars);
    int nShared0, nShared1, nShared = 0;
    for ( v = iVarTry; v < nVars; v++ )
    {
        iVarsNext = 0;
        for ( u = 0; u < nVars; u++ )
            if ( u == v )
                VarsNext[iVarsNext++] = Vars[u];
        iPat0 = iPat1 = 0;
        for ( m = 0; m < nMints; m++ )
            if ( m & (1 << v) )
                Pat1[iPat1++] = m;
            else
                Pat0[iPat0++] = m;
        assert( iPat0 == nMints / 2 );
        assert( iPat1 == nMints / 2 );
        nShared0 = Kit_DecComputeShared_rec( Pat0, VarsNext, nVars-1, Shared0, v + 1 );
        if ( nShared0 == 0 )
            continue;
        nShared1 = Kit_DecComputeShared_rec( Pat1, VarsNext, nVars-1, Shared1, v + 1 );
        if ( nShared1 == 0 )
            continue;
        Shared[nShared++] = v;
        for ( u = 0; u < nShared0; u++ )
        for ( m = 0; m < nShared1; m++ )
            if ( Shared0[u] >= 0 && Shared1[m] >= 0 && Shared0[u] == Shared1[m] )
            {
                Shared[nShared++] = Shared0[u];
                Shared0[u] = Shared1[m] = -1;
            }
        return nShared;
    }
    return 0;
}

/**Function*************************************************************

  Synopsis    [Returns the number of shared variables.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Kit_DecComputeShared( int Pattern[], int LutSize, int Shared[] )
{
    int i, Vars[6];
    assert( LutSize <= 6 );
    for ( i = 0; i < LutSize; i++ )
        Vars[i] = i;
    return Kit_DecComputeShared_rec( Pattern, Vars, LutSize, Shared, 0 );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

