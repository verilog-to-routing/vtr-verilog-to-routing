/**CFile****************************************************************

  FileName    [giaTsim.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Scalable AIG package.]

  Synopsis    [Ternary simulation.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: giaTsim.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "gia.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static inline int Gia_ManTerSimInfoGet( unsigned * pInfo, int i )
{
    return 3 & (pInfo[i >> 4] >> ((i & 15) << 1));
}
static inline void Gia_ManTerSimInfoSet( unsigned * pInfo, int i, int Value )
{
    assert( Value >= GIA_ZER && Value <= GIA_UND );
    Value ^= Gia_ManTerSimInfoGet( pInfo, i );
    pInfo[i >> 4] ^= (Value << ((i & 15) << 1));
}

static inline unsigned * Gia_ManTerStateNext( unsigned * pState, int nWords )                      { return *((unsigned **)(pState + nWords));  }
static inline void       Gia_ManTerStateSetNext( unsigned * pState, int nWords, unsigned * pNext ) { *((unsigned **)(pState + nWords)) = pNext; }

// ternary simulation manager
typedef struct Gia_ManTer_t_ Gia_ManTer_t;
struct Gia_ManTer_t_
{
    Gia_Man_t *    pAig;
    int            nIters;
    int            nStateWords;
    Vec_Ptr_t *    vStates;
    Vec_Ptr_t *    vFlops;
    Vec_Int_t *    vRetired;     // retired registers
    char *         pRetired;     // retired registers
    int *          pCount0;
    int *          pCountX;
    // hash table for states
    int            nBins;
    unsigned **    pBins;
    // simulation information
    unsigned *     pDataSim;     // simulation data
    unsigned *     pDataSimCis;  // simulation data for CIs
    unsigned *     pDataSimCos;  // simulation data for COs
};

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Creates fast simulation manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_ManTer_t * Gia_ManTerCreate( Gia_Man_t * pAig )
{
    Gia_ManTer_t * p;
    p = ABC_CALLOC( Gia_ManTer_t, 1 );
    p->pAig   = Gia_ManFront( pAig );
    p->nIters = 300;
    p->pDataSim    = ABC_ALLOC( unsigned, Abc_BitWordNum(2*p->pAig->nFront) );
    p->pDataSimCis = ABC_ALLOC( unsigned, Abc_BitWordNum(2*Gia_ManCiNum(p->pAig)) );
    p->pDataSimCos = ABC_ALLOC( unsigned, Abc_BitWordNum(2*Gia_ManCoNum(p->pAig)) );
    // allocate storage for terminary states
    p->nStateWords = Abc_BitWordNum( 2*Gia_ManRegNum(pAig) );
    p->vStates  = Vec_PtrAlloc( 1000 );
    p->pCount0  = ABC_CALLOC( int, Gia_ManRegNum(pAig) );
    p->pCountX  = ABC_CALLOC( int, Gia_ManRegNum(pAig) );
    p->nBins    = Abc_PrimeCudd( 500 );
    p->pBins    = ABC_CALLOC( unsigned *, p->nBins );
    p->vRetired = Vec_IntAlloc( 100 );
    p->pRetired = ABC_CALLOC( char, Gia_ManRegNum(pAig) );
    return p;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManTerStatesFree( Vec_Ptr_t * vStates )
{
    unsigned * pTemp;
    int i;
    Vec_PtrForEachEntry( unsigned *, vStates, pTemp, i )
        ABC_FREE( pTemp );
    Vec_PtrFree( vStates );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManTerDelete( Gia_ManTer_t * p )
{
    if ( p->vStates ) 
        Gia_ManTerStatesFree( p->vStates );
    if ( p->vFlops ) 
        Gia_ManTerStatesFree( p->vFlops );
    Gia_ManStop( p->pAig );
    Vec_IntFree( p->vRetired );
    ABC_FREE( p->pRetired );
    ABC_FREE( p->pCount0 );
    ABC_FREE( p->pCountX );
    ABC_FREE( p->pBins );
    ABC_FREE( p->pDataSim );
    ABC_FREE( p->pDataSimCis );
    ABC_FREE( p->pDataSimCos );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Gia_ManTerSimulateCi( Gia_ManTer_t * p, Gia_Obj_t * pObj, int iCi )
{
    Gia_ManTerSimInfoSet( p->pDataSim, Gia_ObjValue(pObj), Gia_ManTerSimInfoGet(p->pDataSimCis, iCi) );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Gia_ManTerSimulateCo( Gia_ManTer_t * p, int iCo, Gia_Obj_t * pObj )
{
    int Value = Gia_ManTerSimInfoGet( p->pDataSim, Gia_ObjDiff0(pObj) );
    Gia_ManTerSimInfoSet( p->pDataSimCos, iCo, Gia_XsimNotCond( Value, Gia_ObjFaninC0(pObj) ) );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Gia_ManTerSimulateNode( Gia_ManTer_t * p, Gia_Obj_t * pObj )
{
    int Value0 = Gia_ManTerSimInfoGet( p->pDataSim, Gia_ObjDiff0(pObj) );
    int Value1 = Gia_ManTerSimInfoGet( p->pDataSim, Gia_ObjDiff1(pObj) );
    Gia_ManTerSimInfoSet( p->pDataSim, Gia_ObjValue(pObj), Gia_XsimAndCond( Value0, Gia_ObjFaninC0(pObj), Value1, Gia_ObjFaninC1(pObj) ) );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Gia_ManTerSimInfoInit( Gia_ManTer_t * p )
{
    int i = 0;
    for ( ; i < Gia_ManPiNum(p->pAig); i++ )
        Gia_ManTerSimInfoSet( p->pDataSimCis, i, GIA_UND );
    for ( ; i < Gia_ManCiNum(p->pAig); i++ )
        Gia_ManTerSimInfoSet( p->pDataSimCis, i, GIA_ZER );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Gia_ManTerSimInfoTransfer( Gia_ManTer_t * p )
{
    int i = 0;
    for ( ; i < Gia_ManPiNum(p->pAig); i++ )
        Gia_ManTerSimInfoSet( p->pDataSimCis, i, GIA_UND );
    for ( ; i < Gia_ManCiNum(p->pAig); i++ )
        Gia_ManTerSimInfoSet( p->pDataSimCis, i, Gia_ManTerSimInfoGet( p->pDataSimCos, Gia_ManCoNum(p->pAig)-Gia_ManCiNum(p->pAig)+i ) );
}


/**Function*************************************************************

  Synopsis    [Computes hash value of the node using its simulation info.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManTerStateHash( unsigned * pState, int nWords, int nTableSize )
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
unsigned * Gia_ManTerStateLookup( unsigned * pState, int nWords, unsigned ** pBins, int nBins )
{
    unsigned * pEntry;
    int Hash = Gia_ManTerStateHash( pState, nWords, nBins );
    for ( pEntry = pBins[Hash]; pEntry; pEntry = Gia_ManTerStateNext(pEntry, nWords) )
        if ( !memcmp( pEntry, pState, sizeof(unsigned) * nWords ) )
            return pEntry;
    return NULL;
}

/**Function*************************************************************

  Synopsis    [Inserts value into the table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManTerStateInsert( unsigned * pState, int nWords, unsigned ** pBins, int nBins )
{
    int Hash = Gia_ManTerStateHash( pState, nWords, nBins );
    assert( !Gia_ManTerStateLookup( pState, nWords, pBins, nBins ) );
    Gia_ManTerStateSetNext( pState, nWords, pBins[Hash] );
    pBins[Hash] = pState;    
}

/**Function*************************************************************

  Synopsis    [Allocs new ternary state.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned * Gia_ManTerStateAlloc( int nWords )
{
    return (unsigned *)ABC_CALLOC( char, sizeof(unsigned) * nWords + sizeof(unsigned *) );
}

/**Function*************************************************************

  Synopsis    [Creates new ternary state.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned * Gia_ManTerStateCreate( Gia_ManTer_t * p )
{
    int i, Value, nPis = Gia_ManPiNum(p->pAig);
    unsigned * pRes = Gia_ManTerStateAlloc( p->nStateWords );
    for ( i = nPis; i < Gia_ManCiNum(p->pAig); i++ )
    {
        Value = Gia_ManTerSimInfoGet( p->pDataSimCis, i );
        Gia_ManTerSimInfoSet( pRes, i-nPis, Value );
        if ( Value == GIA_ZER )
            p->pCount0[i-nPis]++;
        if ( Value == GIA_UND )
            p->pCountX[i-nPis]++;
    }
    Vec_PtrPush( p->vStates, pRes );
    return pRes;    
}

/**Function*************************************************************

  Synopsis    [Performs one round of ternary simulation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Gia_ManTerSimulateRound( Gia_ManTer_t * p )
{
    Gia_Obj_t * pObj;
    int i, iCis = 0, iCos = 0;
    assert( p->pAig->nFront > 0 );
    assert( Gia_ManConst0(p->pAig)->Value == 0 );
    Gia_ManTerSimInfoSet( p->pDataSim, 0, GIA_ZER );
    Gia_ManForEachObj1( p->pAig, pObj, i )
    {
        if ( Gia_ObjIsAndOrConst0(pObj) )
        {
            assert( Gia_ObjValue(pObj) < p->pAig->nFront );
            Gia_ManTerSimulateNode( p, pObj );
        }
        else if ( Gia_ObjIsCi(pObj) )
        {
            assert( Gia_ObjValue(pObj) < p->pAig->nFront );
            Gia_ManTerSimulateCi( p, pObj, iCis++ );
        }
        else // if ( Gia_ObjIsCo(pObj) )
        {
            assert( Gia_ObjValue(pObj) == GIA_NONE );
            Gia_ManTerSimulateCo( p, iCos++, pObj );
        }
    }
    assert( Gia_ManCiNum(p->pAig) == iCis );
    assert( Gia_ManCoNum(p->pAig) == iCos );
}

/**Function*************************************************************

  Synopsis    [Retires a set of registers to speed up convergence.]

  Description [Retire all non-ternary registers which has max number 
  of ternary values so far.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManTerRetire2( Gia_ManTer_t * p, unsigned * pState )
{
    int i, Entry, iMaxTerValue = -1;
    // find non-retired register with this value
    for ( i = 0; i < Gia_ManRegNum(p->pAig); i++ )
        if ( Gia_ManTerSimInfoGet( pState, i ) != GIA_UND && !p->pRetired[i] && iMaxTerValue < p->pCountX[i] )
            iMaxTerValue = p->pCountX[i];
    assert( iMaxTerValue >= 0 );
    // retire the first registers with this value
    for ( i = 0; i < Gia_ManRegNum(p->pAig); i++ )
        if ( Gia_ManTerSimInfoGet( pState, i ) != GIA_UND && !p->pRetired[i] && iMaxTerValue == p->pCountX[i] )
        {
            assert( p->pRetired[i] == 0 );
            p->pRetired[i] = 1;
            Vec_IntPush( p->vRetired, i );
            if ( iMaxTerValue == 0 )
                break;
        }
    // update all the retired registers
    Vec_IntForEachEntry( p->vRetired, Entry, i )
        Gia_ManTerSimInfoSet( p->pDataSimCis, Gia_ManPiNum(p->pAig)+Entry, GIA_UND );
    return Vec_IntSize(p->vRetired);
}

/**Function*************************************************************

  Synopsis    [Retires a set of registers to speed up convergence.]

  Description [Retire all non-ternary registers which has max number 
  of ternary values so far.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManTerRetire( Gia_ManTer_t * p, unsigned * pThis, unsigned * pPrev )
{
    int i, Entry;
    // find registers whose value has changed
    Vec_IntClear( p->vRetired );
    for ( i = 0; i < Gia_ManRegNum(p->pAig); i++ )
        if ( Gia_ManTerSimInfoGet( pThis, i ) != Gia_ManTerSimInfoGet( pPrev, i ) )
            Vec_IntPush( p->vRetired, i );
    // set all of them to zero
    Vec_IntForEachEntry( p->vRetired, Entry, i )
        Gia_ManTerSimInfoSet( p->pDataSimCis, Gia_ManPiNum(p->pAig)+Entry, GIA_UND );
    return Vec_IntSize(p->vRetired);
}

/**Function*************************************************************

  Synopsis    [Inserts value into the table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManTerStatePrint( unsigned * pState, int nRegs, int iNum )
{
    int i, nZeros = 0, nOnes = 0, nDcs = 0;
    printf( " %4d : ", iNum );
    for ( i = 0; i < nRegs; i++ )
    {
        if ( Gia_ManTerSimInfoGet(pState, i) == GIA_ZER )
            printf( "0" ), nZeros++;
        else if ( Gia_ManTerSimInfoGet(pState, i) == GIA_ONE )
            printf( "1" ), nOnes++;
        else if ( Gia_ManTerSimInfoGet(pState, i) == GIA_UND )
            printf( "x" ), nDcs++;
        else
            assert( 0 );
    }
    printf( " (0=%4d, 1=%4d, x=%4d)\n", nZeros, nOnes, nDcs );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManTerAnalyze2( Vec_Ptr_t * vStates, int nRegs )
{
    unsigned * pTemp, * pStates = (unsigned *)Vec_PtrPop( vStates );
    int i, w, nZeros, nConsts, nStateWords;
    // detect constant zero registers
    nStateWords = Abc_BitWordNum( 2*nRegs );
    memset( pStates, 0, sizeof(int) * nStateWords );
    Vec_PtrForEachEntry( unsigned *, vStates, pTemp, i )
        for ( w = 0; w < nStateWords; w++ )
            pStates[w] |= pTemp[w];
    // count the number of zeros
    nZeros = 0;
    for ( i = 0; i < nRegs; i++ )
        if ( Gia_ManTerSimInfoGet(pStates, i) == GIA_ZER )
            nZeros++;
    printf( "Found %d constant registers.\n", nZeros );
    // detect non-ternary registers
    memset( pStates, 0, sizeof(int) * nStateWords );
    Vec_PtrForEachEntry( unsigned *, vStates, pTemp, i )
        for ( w = 0; w < nStateWords; w++ )
            pStates[w] |= (~(pTemp[w] ^ (pTemp[w] >> 1)) & 0x55555555);
    // count the nonternary registers
    nConsts = 0;
    for ( i = 0; i < nRegs; i++ )
        if ( Gia_ManTerSimInfoGet(pStates, i) == 0 )
            nConsts++;
    printf( "Found %d non-ternary registers.\n", nConsts );
    // return the state back
    Vec_PtrPush( vStates, pStates );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManTerAnalyze( Gia_ManTer_t * p )
{
    int i, nZeros = 0, nConsts = 0;
    for ( i = 0; i < Gia_ManRegNum(p->pAig); i++ )
        if ( p->pCount0[i] == Vec_PtrSize(p->vStates) )
            nZeros++;
        else if ( p->pCountX[i] == 0 )
            nConsts++;
//    printf( "Found %d constant registers.\n", nZeros );
//    printf( "Found %d non-ternary registers.\n", nConsts );
}



/**Function*************************************************************

  Synopsis    [Transposes state vector for non-ternary registers.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Gia_ManTerTranspose( Gia_ManTer_t * p )
{
    Vec_Ptr_t * vFlops;
    unsigned * pState, * pFlop;
    int i, k, nFlopWords;
    vFlops = Vec_PtrAlloc( 100 );
    nFlopWords = Abc_BitWordNum( 2*Vec_PtrSize(p->vStates) );
    for ( i = 0; i < Gia_ManRegNum(p->pAig); i++ )
    {
        if ( p->pCount0[i] == Vec_PtrSize(p->vStates) ) 
            continue;
        if ( p->pCountX[i] > 0 )
            continue;
        pFlop = Gia_ManTerStateAlloc( nFlopWords );
        Vec_PtrPush( vFlops, pFlop );
        Vec_PtrForEachEntry( unsigned *, p->vStates, pState, k )
            Gia_ManTerSimInfoSet( pFlop, k, Gia_ManTerSimInfoGet(pState, i) );
//Gia_ManTerStatePrint( pFlop, Vec_PtrSize(p->vStates), i );
    }
    return vFlops;
}

/**Function*************************************************************

  Synopsis    [Transposes state vector for non-ternary registers.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManFindEqualFlop( Vec_Ptr_t * vFlops, int iFlop, int nFlopWords )
{
    unsigned * pFlop, * pTemp;
    int i;
    pFlop = (unsigned *)Vec_PtrEntry( vFlops, iFlop );
    Vec_PtrForEachEntryStop( unsigned *, vFlops, pTemp, i, iFlop )
        if ( !memcmp( pTemp, pFlop, sizeof(unsigned) * nFlopWords ) )
            return i;
    return -1;
}

/**Function*************************************************************

  Synopsis    [Creates map of registers to replace.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int * Gia_ManTerCreateMap( Gia_ManTer_t * p, int fVerbose )
{
    int * pCi2Lit;
    Gia_Obj_t * pObj;
    Vec_Int_t * vMapKtoI;
    int i, iRepr, nFlopWords, Counter0 = 0, CounterE = 0;
    nFlopWords = Abc_BitWordNum( 2*Vec_PtrSize(p->vStates) );
    p->vFlops = Gia_ManTerTranspose( p );
    pCi2Lit = ABC_FALLOC( int, Gia_ManCiNum(p->pAig) );
    vMapKtoI = Vec_IntAlloc( 100 );
    for ( i = 0; i < Gia_ManRegNum(p->pAig); i++ )
        if ( p->pCount0[i] == Vec_PtrSize(p->vStates) )
            pCi2Lit[Gia_ManPiNum(p->pAig)+i] = 0, Counter0++;
        else if ( p->pCountX[i] == 0 )
        {
            iRepr = Gia_ManFindEqualFlop( p->vFlops, Vec_IntSize(vMapKtoI), nFlopWords );
            Vec_IntPush( vMapKtoI, i );
            if ( iRepr < 0 )
                continue;
            pObj = Gia_ManCi( p->pAig, Gia_ManPiNum(p->pAig)+Vec_IntEntry(vMapKtoI, iRepr) );
            pCi2Lit[Gia_ManPiNum(p->pAig)+i] = Abc_Var2Lit( Gia_ObjId( p->pAig, pObj ), 0 );
            CounterE++;
        }
    Vec_IntFree( vMapKtoI );
    if ( fVerbose )
        printf( "Transforming %d const and %d equiv registers.\n", Counter0, CounterE );
    return pCi2Lit;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_ManTer_t * Gia_ManTerSimulate( Gia_Man_t * pAig, int fVerbose )
{
    Gia_ManTer_t * p;
    unsigned * pState, * pPrev, * pLoop;
    int i, Counter;
    abctime clk, clkTotal = Abc_Clock();
    assert( Gia_ManRegNum(pAig) > 0 );
    // create manager
    clk = Abc_Clock();
    p = Gia_ManTerCreate( pAig );
    if ( 0 )
    {
        printf( "Obj = %8d (%8d). F = %6d. ", 
            pAig->nObjs, Gia_ManCiNum(pAig) + Gia_ManAndNum(pAig), p->pAig->nFront );
        printf( "AIG = %7.2f MB. F-mem = %7.2f MB. Other = %7.2f MB.  ", 
            12.0*Gia_ManObjNum(p->pAig)/(1<<20), 
            4.0*Abc_BitWordNum(2 * p->pAig->nFront)/(1<<20), 
            4.0*Abc_BitWordNum(2 * (Gia_ManCiNum(pAig) + Gia_ManCoNum(pAig)))/(1<<20) );
        ABC_PRT( "Time", Abc_Clock() - clk );
    }
    // perform simulation
    Gia_ManTerSimInfoInit( p );
    // hash the first state
    pState = Gia_ManTerStateCreate( p );
    Gia_ManTerStateInsert( pState, p->nStateWords, p->pBins, p->nBins );
//Gia_ManTerStatePrint( pState, Gia_ManRegNum(pAig), 0 );
    // perform simuluation till convergence
    pPrev = NULL;
    for ( i = 0; ; i++ )
    {
        Gia_ManTerSimulateRound( p );
        Gia_ManTerSimInfoTransfer( p );
        pState = Gia_ManTerStateCreate( p );
//Gia_ManTerStatePrint( pState, Gia_ManRegNum(pAig), i+1 );
        if ( (pLoop = Gia_ManTerStateLookup(pState, p->nStateWords, p->pBins, p->nBins)) )
        {
            pAig->nTerStates = Vec_PtrSize( p->vStates );
            pAig->nTerLoop = Vec_PtrFind( p->vStates, pLoop );
            break;
        }
        Gia_ManTerStateInsert( pState, p->nStateWords, p->pBins, p->nBins );
        if ( i >= p->nIters && i % 10 == 0 )
        {
            Counter = Gia_ManTerRetire( p, pState, pPrev );
//            Counter = Gia_ManTerRetire2( p, pState );
//            if ( fVerbose )
//                printf( "Retired %d registers.\n", Counter );
        }
        pPrev = pState;
    }
    if ( fVerbose )
    {
        printf( "Ternary simulation saturated after %d iterations. ", i+1 );
        ABC_PRT( "Time", Abc_Clock() - clkTotal );
    }
    return p;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManReduceConst( Gia_Man_t * pAig, int fVerbose )
{
    Gia_ManTer_t * p;
    Gia_Man_t * pNew = NULL;
    int * pCi2Lit;
    p = Gia_ManTerSimulate( pAig, fVerbose );
    Gia_ManTerAnalyze( p );
    pCi2Lit = Gia_ManTerCreateMap( p, fVerbose );
    Gia_ManTerDelete( p );
    pNew = Gia_ManDupDfsCiMap( pAig, pCi2Lit, NULL );
    ABC_FREE( pCi2Lit );
    return pNew;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

