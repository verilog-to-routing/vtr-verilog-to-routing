/**CFile****************************************************************

  FileName    [fsimTsim.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Fast sequential AIG simulator.]

  Synopsis    [Varius utilities.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: fsimTsim.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "fsimInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#define FSIM_ZER 1
#define FSIM_ONE 2
#define FSIM_UND 3

static inline int Aig_XsimNotCond( int Value, int fCompl )   
{ 
    if ( Value == FSIM_UND )
        return FSIM_UND;
    if ( Value == FSIM_ZER + fCompl )
        return FSIM_ZER;
    return FSIM_ONE;
}
static inline int Aig_XsimAndCond( int Value0, int fCompl0, int Value1, int fCompl1 )   
{ 
    if ( Value0 == FSIM_UND || Value1 == FSIM_UND )
        return FSIM_UND;
    if ( Value0 == FSIM_ZER + fCompl0 || Value1 == FSIM_ZER + fCompl1 )
        return FSIM_ZER;
    return FSIM_ONE;
}

static inline int Fsim_ManTerSimInfoGet( unsigned * pInfo, int i )
{
    return 3 & (pInfo[i >> 4] >> ((i & 15) << 1));
}
static inline void Fsim_ManTerSimInfoSet( unsigned * pInfo, int i, int Value )
{
    assert( Value >= FSIM_ZER && Value <= FSIM_UND );
    Value ^= Fsim_ManTerSimInfoGet( pInfo, i );
    pInfo[i >> 4] ^= (Value << ((i & 15) << 1));
}

static inline unsigned * Fsim_ManTerStateNext( unsigned * pState, int nWords )                      { return *((unsigned **)(pState + nWords));  }
static inline void       Fsim_ManTerStateSetNext( unsigned * pState, int nWords, unsigned * pNext ) { *((unsigned **)(pState + nWords)) = pNext; }

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Fsim_ManTerSimulateCi( Fsim_Man_t * p, int iNode, int iCi )
{
    Fsim_ManTerSimInfoSet( p->pDataSim, iNode, Fsim_ManTerSimInfoGet(p->pDataSimCis, iCi) );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Fsim_ManTerSimulateCo( Fsim_Man_t * p, int iCo, int iFan0 )
{
    int Value = Fsim_ManTerSimInfoGet( p->pDataSim, Fsim_Lit2Var(iFan0) );
    Fsim_ManTerSimInfoSet( p->pDataSimCos, iCo, Aig_XsimNotCond( Value, Fsim_LitIsCompl(iFan0) ) );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Fsim_ManTerSimulateNode( Fsim_Man_t * p, int iNode, int iFan0, int iFan1 )
{
    int Value0 = Fsim_ManTerSimInfoGet( p->pDataSim, Fsim_Lit2Var(iFan0) );
    int Value1 = Fsim_ManTerSimInfoGet( p->pDataSim, Fsim_Lit2Var(iFan1) );
    Fsim_ManTerSimInfoSet( p->pDataSim, iNode, Aig_XsimAndCond( Value0, Fsim_LitIsCompl(iFan0), Value1, Fsim_LitIsCompl(iFan1) ) );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Fsim_ManTerSimInfoInit( Fsim_Man_t * p )
{
    int iPioNum, i;
    Vec_IntForEachEntry( p->vCis2Ids, iPioNum, i )
    {
        if ( iPioNum < p->nPis )
            Fsim_ManTerSimInfoSet( p->pDataSimCis, i, FSIM_UND );
        else
            Fsim_ManTerSimInfoSet( p->pDataSimCis, i, FSIM_ZER );
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Fsim_ManTerSimInfoTransfer( Fsim_Man_t * p )
{
    int iPioNum, i;
    Vec_IntForEachEntry( p->vCis2Ids, iPioNum, i )
    {
        if ( iPioNum < p->nPis )
            Fsim_ManTerSimInfoSet( p->pDataSimCis, i, FSIM_UND );
        else
            Fsim_ManTerSimInfoSet( p->pDataSimCis, i, Fsim_ManTerSimInfoGet( p->pDataSimCos, p->nPos+iPioNum-p->nPis ) );
    }
}


/**Function*************************************************************

  Synopsis    [Computes hash value of the node using its simulation info.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fsim_ManTerStateHash( unsigned * pState, int nWords, int nTableSize )
{
    static int s_FPrimes[128] = { 
        1009, 1049, 1093, 1151, 1201, 1249, 1297, 1361, 1427, 1459, 
        1499, 1559, 1607, 1657, 1709, 1759, 1823, 1877, 1933, 1997, 
        2039, 2089, 2141, 2213, 2269, 2311, 2371, 2411, 2467, 2543, 
        2609, 2663, 2699, 2741, 2797, 2851, 2909, 2969, 3037, 3089, 
        3169, 3221, 3299, 3331, 3389, 3461, 3517, 3557, 3613, 3671, 
        3719, 3779, 3847, 3907, 3943, 4013, 4073, 4129, 4201, 4243, 
        4289, 4363, 4441, 4493, 4549, 4621, 4663, 4729, 4793, 4871, 
        4933, 4973, 5021, 5087, 5153, 5227, 5281, 5351, 5417, 5471, 
        5519, 5573, 5651, 5693, 5749, 5821, 5861, 5923, 6011, 6073, 
        6131, 6199, 6257, 6301, 6353, 6397, 6481, 6563, 6619, 6689, 
        6737, 6803, 6863, 6917, 6977, 7027, 7109, 7187, 7237, 7309, 
        7393, 7477, 7523, 7561, 7607, 7681, 7727, 7817, 7877, 7933, 
        8011, 8039, 8059, 8081, 8093, 8111, 8123, 8147
    };
    unsigned uHash;
    int i;
    uHash = 0;
    for ( i = 0; i < nWords; i++ )
        uHash ^= pState[i] * s_FPrimes[i & 0x7F];
    return uHash % nTableSize;
}

/**Function*************************************************************

  Synopsis    [Inserts value into the table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fsim_ManTerStateLookup( unsigned * pState, int nWords, unsigned ** pBins, int nBins )
{
    unsigned * pEntry;
    int Hash;
    Hash = Fsim_ManTerStateHash( pState, nWords, nBins );
    for ( pEntry = pBins[Hash]; pEntry; pEntry = Fsim_ManTerStateNext(pEntry, nWords) )
        if ( !memcmp( pEntry, pState, sizeof(unsigned) * nWords ) )
            return 1;
    return 0;
}

/**Function*************************************************************

  Synopsis    [Inserts value into the table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fsim_ManTerStateInsert( unsigned * pState, int nWords, unsigned ** pBins, int nBins )
{
    int Hash = Fsim_ManTerStateHash( pState, nWords, nBins );
    assert( !Fsim_ManTerStateLookup( pState, nWords, pBins, nBins ) );
    Fsim_ManTerStateSetNext( pState, nWords, pBins[Hash] );
    pBins[Hash] = pState;    
}

/**Function*************************************************************

  Synopsis    [Inserts value into the table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned * Fsim_ManTerStateCreate( unsigned * pInfo, int nPis, int nCis, int nWords )
{
    unsigned * pRes;
    int i;
    pRes = (unsigned *)ABC_CALLOC( char, sizeof(unsigned) * nWords + sizeof(unsigned *) );
    for ( i = nPis; i < nCis; i++ )
        Fsim_ManTerSimInfoSet( pRes, i-nPis, Fsim_ManTerSimInfoGet(pInfo, i) );
    return pRes;    
}

/**Function*************************************************************

  Synopsis    [Inserts value into the table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fsim_ManTerStatePrint( unsigned * pState, int nRegs )
{
    int i, Value, nZeros = 0, nOnes = 0, nDcs = 0;
    for ( i = 0; i < nRegs; i++ )
    {
        Value = (Aig_InfoHasBit( pState, 2 * i + 1 ) << 1) | Aig_InfoHasBit( pState, 2 * i );
        if ( Value == 1 )
            printf( "0" ), nZeros++;
        else if ( Value == 2 )
            printf( "1" ), nOnes++;
        else if ( Value == 3 )
            printf( "x" ), nDcs++;
        else
            assert( 0 );
    }
    printf( " (0=%5d, 1=%5d, x=%5d)\n", nZeros, nOnes, nDcs );
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Fsim_ManTerSimulateRound( Fsim_Man_t * p )
{
    int * pCur, * pEnd;
    int iCis = 0, iCos = 0;
    if ( Aig_ObjRefs(Aig_ManConst1(p->pAig)) )
        Fsim_ManTerSimInfoSet( p->pDataSimCis, 1, FSIM_ONE );
    pCur = p->pDataAig2 + 6;
    pEnd = p->pDataAig2 + 3 * p->nObjs;
    while ( pCur < pEnd )
    {
        if ( pCur[1] == 0 )
            Fsim_ManTerSimulateCi( p, pCur[0], iCis++ );
        else if ( pCur[2] == 0 )
            Fsim_ManTerSimulateCo( p, iCos++, pCur[1] );
        else
            Fsim_ManTerSimulateNode( p, pCur[0], pCur[1], pCur[2] );
        pCur += 3;
    }
    assert( iCis == p->nCis );
    assert( iCos == p->nCos );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Fsim_ManTerSimulate( Aig_Man_t * pAig, int fVerbose )
{
    Fsim_Man_t * p;
    Vec_Ptr_t * vStates;
    unsigned ** pBins, * pState;
    int i, nWords, nBins;
    clock_t clk, clkTotal = clock();
    assert( Aig_ManRegNum(pAig) > 0 );
    // create manager
    clk = clock();
    p = Fsim_ManCreate( pAig );
    if ( fVerbose )
    {
        printf( "Obj = %8d (%8d). Cut = %6d. Front = %6d.  FrtMem = %7.2f MB. ", 
            p->nObjs, p->nCis + p->nNodes, p->nCrossCutMax, p->nFront, 
            4.0*Aig_BitWordNum(2 * p->nFront)/(1<<20) );
        ABC_PRT( "Time", clock() - clk );
    }
    // create simulation frontier
    clk = clock();
    Fsim_ManFront( p, 0 );
    if ( fVerbose )
    {
        printf( "Max ID = %8d. Log max ID = %2d.  AigMem = %7.2f MB (%5.2f byte/obj).  ", 
            p->iNumber, Aig_Base2Log(p->iNumber), 
            1.0*(p->pDataCur-p->pDataAig)/(1<<20), 
            1.0*(p->pDataCur-p->pDataAig)/p->nObjs ); 
        ABC_PRT( "Time", clock() - clk );
    }
    // allocate storage for terminary states
    nWords  = Abc_BitWordNum( 2*Aig_ManRegNum(pAig) );
    vStates = Vec_PtrAlloc( 1000 );
    nBins   = Abc_PrimeCudd( 500 );
    pBins   = ABC_ALLOC( unsigned *, nBins );
    memset( pBins, 0, sizeof(unsigned *) * nBins );
    // perform simulation
    assert( p->pDataSim == NULL );
    p->pDataSim = ABC_ALLOC( unsigned, Aig_BitWordNum(2 * p->nFront) * sizeof(unsigned) );
    p->pDataSimCis = ABC_ALLOC( unsigned, Aig_BitWordNum(2 * p->nCis) * sizeof(unsigned) );
    p->pDataSimCos = ABC_ALLOC( unsigned, Aig_BitWordNum(2 * p->nCos) * sizeof(unsigned) );
    Fsim_ManTerSimInfoInit( p );
    // hash the first state
    pState = Fsim_ManTerStateCreate( p->pDataSimCis, p->nPis, p->nCis, nWords );
    Vec_PtrPush( vStates, pState );
    Fsim_ManTerStateInsert( pState, nWords, pBins, nBins );
    // perform simuluation till convergence
    for ( i = 0; ; i++ )
    {
        Fsim_ManTerSimulateRound( p );
        Fsim_ManTerSimInfoTransfer( p );
        // hash the first state
        pState = Fsim_ManTerStateCreate( p->pDataSimCis, p->nPis, p->nCis, nWords );
        Vec_PtrPush( vStates, pState );
        if ( Fsim_ManTerStateLookup(pState, nWords, pBins, nBins) )
            break;
        Fsim_ManTerStateInsert( pState, nWords, pBins, nBins );
    }
    if ( fVerbose )
    {
        printf( "Maxcut = %8d.  AigMem = %7.2f MB.  SimMem = %7.2f MB.  ", 
            p->nCrossCutMax, 
            p->pDataAig2? 12.0*p->nObjs/(1<<20) : 1.0*(p->pDataCur-p->pDataAig)/(1<<20), 
            4.0*(Aig_BitWordNum(2 * p->nFront)+Aig_BitWordNum(2 * p->nCis)+Aig_BitWordNum(2 * p->nCos))/(1<<20) );
        ABC_PRT( "Sim time", clock() - clkTotal );
    }
    ABC_FREE( pBins );
    Fsim_ManDelete( p );
    return vStates;

}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

