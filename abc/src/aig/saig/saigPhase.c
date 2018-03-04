/**CFile****************************************************************

  FileName    [saigPhase.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Sequential AIG package.]

  Synopsis    [Automated phase abstraction.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: saigPhase.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "saig.h"

ABC_NAMESPACE_IMPL_START


/*
    The algorithm is described in the paper: Per Bjesse and Jim Kukula,
    "Automatic Phase Abstraction for Formal Verification", ICCAD 2005
    http://www.iccad.com/data2/iccad/iccad_05acceptedpapers.nsf/9cfb1ebaaf59043587256a6a00031f78/1701ecf34b149e958725702f00708828?OpenDocument
*/

// the maximum number of cycles of termiry simulation
#define TSIM_MAX_ROUNDS    10000
#define TSIM_ONE_SERIES     3000

#define SAIG_XVS0   1
#define SAIG_XVS1   2
#define SAIG_XVSX   3

static inline int  Saig_XsimConvertValue( int v )  { return v == 0? SAIG_XVS0 : (v == 1? SAIG_XVS1 : (v == 2? SAIG_XVSX : -1));  }

static inline void Saig_ObjSetXsim( Aig_Obj_t * pObj, int Value )  { pObj->nCuts = Value;  }
static inline int  Saig_ObjGetXsim( Aig_Obj_t * pObj )             { return pObj->nCuts;   }
static inline int  Saig_XsimInv( int Value )   
{ 
    if ( Value == SAIG_XVS0 )
        return SAIG_XVS1;
    if ( Value == SAIG_XVS1 )
        return SAIG_XVS0;
    assert( Value == SAIG_XVSX );       
    return SAIG_XVSX;
}
static inline int  Saig_XsimAnd( int Value0, int Value1 )   
{ 
    if ( Value0 == SAIG_XVS0 || Value1 == SAIG_XVS0 )
        return SAIG_XVS0;
    if ( Value0 == SAIG_XVSX || Value1 == SAIG_XVSX )
        return SAIG_XVSX;
    assert( Value0 == SAIG_XVS1 && Value1 == SAIG_XVS1 );
    return SAIG_XVS1;
}
static inline int  Saig_XsimRand2()   
{
    return (Aig_ManRandom(0) & 1) ? SAIG_XVS1 : SAIG_XVS0;
}
static inline int  Saig_XsimRand3()   
{
    int RetValue;
    do { 
        RetValue = Aig_ManRandom(0) & 3; 
    } while ( RetValue == 0 );
    return RetValue;
}
static inline int  Saig_ObjGetXsimFanin0( Aig_Obj_t * pObj )       
{ 
    int RetValue;
    RetValue = Saig_ObjGetXsim(Aig_ObjFanin0(pObj));
    return Aig_ObjFaninC0(pObj)? Saig_XsimInv(RetValue) : RetValue;
}
static inline int  Saig_ObjGetXsimFanin1( Aig_Obj_t * pObj )       
{ 
    int RetValue;
    RetValue = Saig_ObjGetXsim(Aig_ObjFanin1(pObj));
    return Aig_ObjFaninC1(pObj)? Saig_XsimInv(RetValue) : RetValue;
}
static inline void Saig_XsimPrint( FILE * pFile, int Value )   
{ 
    if ( Value == SAIG_XVS0 )
    {
        fprintf( pFile, "0" );
        return;
    }
    if ( Value == SAIG_XVS1 )
    {
        fprintf( pFile, "1" );
        return;
    }
    assert( Value == SAIG_XVSX );       
    fprintf( pFile, "x" );
}

// simulation manager
typedef struct Saig_Tsim_t_ Saig_Tsim_t;
struct Saig_Tsim_t_
{
    Aig_Man_t *      pAig;              // the original AIG manager
    int              nWords;            // the number of words in the states
    // ternary state representation
    Vec_Ptr_t *      vStates;           // the collection of ternary states
    Aig_MmFixed_t *  pMem;              // memory for ternary states
    int              nPrefix;           // prefix of the ternary state space
    int              nCycle;            // cycle of the ternary state space
    int              nNonXRegs;         // the number of candidate registers
    Vec_Int_t *      vNonXRegs;         // the candidate registers
    // hash table for terminary states
    unsigned **      pBins;
    int              nBins;
};

static inline unsigned * Saig_TsiNext( unsigned * pState, int nWords )                      { return *((unsigned **)(pState + nWords));  }
static inline void       Saig_TsiSetNext( unsigned * pState, int nWords, unsigned * pNext ) { *((unsigned **)(pState + nWords)) = pNext; }

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Allocates simulation manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Saig_Tsim_t * Saig_TsiStart( Aig_Man_t * pAig )
{
    Saig_Tsim_t * p;
    p = (Saig_Tsim_t *)ABC_ALLOC( char, sizeof(Saig_Tsim_t) );
    memset( p, 0, sizeof(Saig_Tsim_t) );
    p->pAig    = pAig;
    p->nWords  = Abc_BitWordNum( 2*Aig_ManRegNum(pAig) );
    p->vStates = Vec_PtrAlloc( 1000 );
    p->pMem    = Aig_MmFixedStart( sizeof(unsigned) * p->nWords + sizeof(unsigned *), 10000 );
    p->nBins   = Abc_PrimeCudd(TSIM_MAX_ROUNDS/2);
    p->pBins   = ABC_ALLOC( unsigned *, p->nBins );
    memset( p->pBins, 0, sizeof(unsigned *) * p->nBins );
    return p;
}

/**Function*************************************************************

  Synopsis    [Deallocates simulation manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Saig_TsiStop( Saig_Tsim_t * p )
{
    if ( p->vNonXRegs )
        Vec_IntFree( p->vNonXRegs );
    Aig_MmFixedStop( p->pMem, 0 );
    Vec_PtrFree( p->vStates );
    ABC_FREE( p->pBins );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    [Computes hash value of the node using its simulation info.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Saig_TsiStateHash( unsigned * pState, int nWords, int nTableSize )
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

  Synopsis    [Count non-X-valued registers in the simulation data.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Saig_TsiCountNonXValuedRegisters( Saig_Tsim_t * p, int nPref )
{
    unsigned * pState;
    int nRegs = p->pAig->nRegs;
    int Value, i, k;
    assert( p->vNonXRegs == NULL );
    p->vNonXRegs = Vec_IntAlloc( 10 );
    for ( i = 0; i < nRegs; i++ )
    {
        Vec_PtrForEachEntryStart( unsigned *, p->vStates, pState, k, nPref )
        {
            Value = (Abc_InfoHasBit( pState, 2 * i + 1 ) << 1) | Abc_InfoHasBit( pState, 2 * i );
            assert( Value != 0 );
            if ( Value == SAIG_XVSX )
                break;
        }
        if ( k == Vec_PtrSize(p->vStates) )
            Vec_IntPush( p->vNonXRegs, i );
    }
    return Vec_IntSize(p->vNonXRegs);
} 

/**Function*************************************************************

  Synopsis    [Computes flops that are stuck-at constant.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Saig_TsiComputeTransient( Saig_Tsim_t * p, int nPref )
{
    Vec_Int_t * vCounters;
    unsigned * pState;
    int ValueThis = -1, ValuePrev = -1, StepPrev = -1;
    int i, k, nRegs = p->pAig->nRegs;
    vCounters = Vec_IntStart( nPref );
    for ( i = 0; i < nRegs; i++ )
    {
        Vec_PtrForEachEntry( unsigned *, p->vStates, pState, k )
        {
            ValueThis = (Abc_InfoHasBit( pState, 2 * i + 1 ) << 1) | Abc_InfoHasBit( pState, 2 * i );
//printf( "%s", (ValueThis == 1)? "0" : ((ValueThis == 2)? "1" : "x") );
            assert( ValueThis != 0 );
            if ( ValuePrev != ValueThis )
            {
                ValuePrev = ValueThis;
                StepPrev  = k;
            }
        }
//printf( "\n" );
        if ( ValueThis == SAIG_XVSX )
            continue;
        if ( StepPrev >= nPref )
            continue;
        Vec_IntAddToEntry( vCounters, StepPrev, 1 );
    }
    Vec_IntForEachEntry( vCounters, ValueThis, i )
    {
        if ( ValueThis == 0 )
            continue;
//        printf( "%3d : %3d\n", i, ValueThis );
    }
    return vCounters;
}

/**Function*************************************************************

  Synopsis    [Count non-X-valued registers in the simulation data.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Saig_TsiPrintTraces( Saig_Tsim_t * p, int nWords, int nPrefix, int nLoop )
{
    unsigned * pState;
    int nRegs = p->pAig->nRegs;
    int Value, i, k, Counter = 0;
    printf( "Ternary traces for each flop:\n" );
    printf( "      : " );
    for ( i = 0; i < Vec_PtrSize(p->vStates) - nLoop - 1; i++ )
        printf( "%d", i%10 );
    printf( "  " );
    for ( i = 0; i < nLoop; i++ )
        printf( "%d", i%10 );
    printf( "\n" );
    for ( i = 0; i < nRegs; i++ )
    {
/*
        Vec_PtrForEachEntry( unsigned *, p->vStates, pState, k )
        {
            Value = (Abc_InfoHasBit( pState, 2 * i + 1 ) << 1) | Abc_InfoHasBit( pState, 2 * i );
            if ( Value == SAIG_XVSX )
                break;
        }
        if ( k == Vec_PtrSize(p->vStates) )
            Counter++;
        else
            continue;
*/

        // print trace
//        printf( "%5d : %5d %5d  ", Counter, i, Saig_ManLo(p->pAig, i)->Id );
        printf( "%5d : ", Counter++ );
        Vec_PtrForEachEntryStop( unsigned *, p->vStates, pState, k, Vec_PtrSize(p->vStates)-1 )
        {
            Value = (Abc_InfoHasBit( pState, 2 * i + 1 ) << 1) | Abc_InfoHasBit( pState, 2 * i );
            if ( Value == SAIG_XVS0 )
                printf( "0" );
            else if ( Value == SAIG_XVS1 )
                printf( "1" );
            else if ( Value == SAIG_XVSX )
                printf( "x" );
            else
                assert( 0 );
            if ( k == nPrefix - 1 )
                printf( "  " );
        }
        printf( "\n" );
    }
}

/**Function*************************************************************

  Synopsis    [Returns the number of the state.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Saig_TsiComputePrefix( Saig_Tsim_t * p, unsigned * pState, int nWords )
{
    unsigned * pEntry, * pPrev;
    int Hash, i;
    Hash = Saig_TsiStateHash( pState, nWords, p->nBins );
    for ( pEntry = p->pBins[Hash]; pEntry; pEntry = Saig_TsiNext(pEntry, nWords) )
        if ( !memcmp( pEntry, pState, sizeof(unsigned) * nWords ) )
        {
            Vec_PtrForEachEntry( unsigned *, p->vStates, pPrev, i )
            {
                if ( pPrev == pEntry )
                    return i;
            }
            assert( 0 );
            return -1;
        }
    return -1;
}

/**Function*************************************************************

  Synopsis    [Checks if the value exists in the table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Saig_TsiStateLookup( Saig_Tsim_t * p, unsigned * pState, int nWords )
{
    unsigned * pEntry;
    int Hash;
    Hash = Saig_TsiStateHash( pState, nWords, p->nBins );
    for ( pEntry = p->pBins[Hash]; pEntry; pEntry = Saig_TsiNext(pEntry, nWords) )
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
void Saig_TsiStateInsert( Saig_Tsim_t * p, unsigned * pState, int nWords )
{
    int Hash = Saig_TsiStateHash( pState, nWords, p->nBins );
    assert( !Saig_TsiStateLookup( p, pState, nWords ) );
    Saig_TsiSetNext( pState, nWords, p->pBins[Hash] );
    p->pBins[Hash] = pState;    
}

/**Function*************************************************************

  Synopsis    [Inserts value into the table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned * Saig_TsiStateNew( Saig_Tsim_t * p )
{
    unsigned * pState;
    pState = (unsigned *)Aig_MmFixedEntryFetch( p->pMem );
    memset( pState, 0, sizeof(unsigned) * p->nWords );
    Vec_PtrPush( p->vStates, pState );
    return pState;
}

/**Function*************************************************************

  Synopsis    [Inserts value into the table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Saig_TsiStatePrint( Saig_Tsim_t * p, unsigned * pState )
{
    int i, Value, nZeros = 0, nOnes = 0, nDcs = 0;
    for ( i = 0; i < Aig_ManRegNum(p->pAig); i++ )
    {
        Value = (Abc_InfoHasBit( pState, 2 * i + 1 ) << 1) | Abc_InfoHasBit( pState, 2 * i );
        if ( Value == SAIG_XVS0 )
            printf( "0" ), nZeros++;
        else if ( Value == SAIG_XVS1 )
            printf( "1" ), nOnes++;
        else if ( Value == SAIG_XVSX )
            printf( "x" ), nDcs++;
        else
            assert( 0 );
    }
    printf( " (0=%5d, 1=%5d, x=%5d)\n", nZeros, nOnes, nDcs );
}

/**Function*************************************************************

  Synopsis    [Count constant values in the state.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Saig_TsiStateCount( Saig_Tsim_t * p, unsigned * pState )
{
    Aig_Obj_t * pObjLi, * pObjLo;
    int i, Value, nCounter = 0;
    Aig_ManForEachLiLoSeq( p->pAig, pObjLi, pObjLo, i )
    {
        Value = (Abc_InfoHasBit( pState, 2 * i + 1 ) << 1) | Abc_InfoHasBit( pState, 2 * i );
        nCounter += (Value == SAIG_XVS0 || Value == SAIG_XVS1);
    }
    return nCounter;
}

/**Function*************************************************************

  Synopsis    [Count constant values in the state.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Saig_TsiStateOrAll( Saig_Tsim_t * pTsi, unsigned * pState )
{
    unsigned * pPrev;
    int i, k;
    Vec_PtrForEachEntry( unsigned *, pTsi->vStates, pPrev, i )
    {
        for ( k = 0; k < pTsi->nWords; k++ )
            pState[k] |= pPrev[k];
    }
}

/**Function*************************************************************

  Synopsis    [Cycles the circuit to create a new initial state.]

  Description [Simulates the circuit with random input for the given 
  number of timeframes to get a better initial state.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Saig_Tsim_t * Saig_ManReachableTernary( Aig_Man_t * p, Vec_Int_t * vInits, int fVerbose )
{
    Saig_Tsim_t * pTsi;
    Aig_Obj_t * pObj, * pObjLi, * pObjLo;
    unsigned * pState;
    int i, f, Value, nCounter;
    // allocate the simulation manager
    pTsi = Saig_TsiStart( p );
    // initialize the values
    Saig_ObjSetXsim( Aig_ManConst1(p), SAIG_XVS1 );
    Saig_ManForEachPi( p, pObj, i )
        Saig_ObjSetXsim( pObj, SAIG_XVSX );
    if ( vInits )
    {
        Saig_ManForEachLo( p, pObj, i )
            Saig_ObjSetXsim( pObj, Saig_XsimConvertValue(Vec_IntEntry(vInits, i)) );
    }
    else 
    {
        Saig_ManForEachLo( p, pObj, i )
            Saig_ObjSetXsim( pObj, SAIG_XVS0 );
    }
    // simulate for the given number of timeframes
    for ( f = 0; f < TSIM_MAX_ROUNDS; f++ )
    {
        // collect this state
        pState = Saig_TsiStateNew( pTsi );
        Saig_ManForEachLiLo( p, pObjLi, pObjLo, i )
        {
            Value = Saig_ObjGetXsim(pObjLo);
            if ( Value & 1 )
                Abc_InfoSetBit( pState, 2 * i );
            if ( Value & 2 )
                Abc_InfoSetBit( pState, 2 * i + 1 );
        }
//        printf( "%d ", Saig_TsiStateCount(pTsi, pState) );
//        Saig_TsiStatePrint( pTsi, pState );
        // check if this state exists
        if ( Saig_TsiStateLookup( pTsi, pState, pTsi->nWords ) )
        {
            if ( fVerbose )
                printf( "Ternary simulation converged after %d iterations.\n", f );
            return pTsi;
        }
        // insert this state
        Saig_TsiStateInsert( pTsi, pState, pTsi->nWords );
        // simulate internal nodes
        Aig_ManForEachNode( p, pObj, i )
            Saig_ObjSetXsim( pObj, Saig_XsimAnd(Saig_ObjGetXsimFanin0(pObj), Saig_ObjGetXsimFanin1(pObj)) );
        // transfer the latch values
        Saig_ManForEachLi( p, pObj, i )
            Saig_ObjSetXsim( pObj, Saig_ObjGetXsimFanin0(pObj) );
        nCounter = 0;
        Saig_ManForEachLiLo( p, pObjLi, pObjLo, i )
        {
            if ( f < TSIM_ONE_SERIES )
                Saig_ObjSetXsim( pObjLo, Saig_ObjGetXsim(pObjLi) );
            else
            {
                if ( Saig_ObjGetXsim(pObjLi) != Saig_ObjGetXsim(pObjLo) )
                    Saig_ObjSetXsim( pObjLo, SAIG_XVSX );
            }
            nCounter += (Saig_ObjGetXsim(pObjLo) == SAIG_XVS0);
        }
    }
    printf( "Saig_ManReachableTernary(): Did not reach a fixed point after %d iterations (not a bug).\n", TSIM_MAX_ROUNDS );
    Saig_TsiStop( pTsi );
    return NULL;
}

/**Function*************************************************************

  Synopsis    [Analize initial value of the selected register.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Saig_ManAnalizeControl( Aig_Man_t * p, int Reg )
{
    Aig_Obj_t * pObj, * pReg, * pCtrl, * pAnd;
    int i;
    pReg = Saig_ManLo( p, Reg );
    pCtrl = Saig_ManLo( p, Saig_ManRegNum(p)-1 );
    assert( pReg->Id < pCtrl->Id );
    // find a node pointing to both
    pAnd = NULL;
    Aig_ManForEachNode( p, pObj, i )
    {
        if ( Aig_ObjFanin0(pObj) == pReg && Aig_ObjFanin1(pObj) == pCtrl )
        {
            pAnd = pObj;
            break;
        }
    }
    if ( pAnd == NULL )
    {
        printf( "Register is not found.\n" );
        return;
    }
    printf( "Clock-like register: \n" );
    Aig_ObjPrint( p, pReg );
    printf( "\n" );
    printf( "Control register: \n" );
    Aig_ObjPrint( p, pCtrl );
    printf( "\n" );
    printf( "Their fanout: \n" );
    Aig_ObjPrint( p, pAnd );
    printf( "\n" );
 
    // find the fanouts of pAnd
    printf( "Fanouts of the fanout: \n" );
    Aig_ManForEachObj( p, pObj, i )
        if ( Aig_ObjFanin0(pObj) == pAnd || Aig_ObjFanin1(pObj) == pAnd )
        {
            Aig_ObjPrint( p, pObj );
            printf( "\n" );
        }
    printf( "\n" );
}

/**Function*************************************************************

  Synopsis    [Finds the registers to phase-abstract.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Saig_ManFindRegisters( Saig_Tsim_t * pTsi, int nFrames, int fIgnore, int fVerbose )
{
    int Values[257] = {0};
    unsigned * pState;
    int r, i, k, Reg, Value;
    int nTests = pTsi->nPrefix + 2 * pTsi->nCycle;
    assert( nFrames <= 256 );
    r = 0;
    Vec_IntForEachEntry( pTsi->vNonXRegs, Reg, i )
    {
        for ( k = 0; k < nTests; k++ )
        {
            if ( k < pTsi->nPrefix + pTsi->nCycle )
                pState = (unsigned *)Vec_PtrEntry( pTsi->vStates, k );
            else
                pState = (unsigned *)Vec_PtrEntry( pTsi->vStates, k - pTsi->nCycle );
            Value = (Abc_InfoHasBit( pState, 2 * Reg + 1 ) << 1) | Abc_InfoHasBit( pState, 2 * Reg );
            assert( Value == SAIG_XVS0 || Value == SAIG_XVS1 );
            if ( k < nFrames || (fIgnore && k == nFrames) )
                Values[k % nFrames] = Value;
            else if ( Values[k % nFrames] != Value )
                break;
        }
        if ( k < nTests )
            continue;
        // skip stuck at
        if ( fIgnore )
        {
            for ( k = 1; k < nFrames; k++ )
                if ( Values[k] != Values[0] )
                    break;
            if ( k == nFrames )
                continue;
        }
        // report useful register
        Vec_IntWriteEntry( pTsi->vNonXRegs, r++, Reg );
        if ( fVerbose )
        {
            printf( "Register %5d has generator: [", Reg );
            for ( k = 0; k < nFrames; k++ )
                Saig_XsimPrint( stdout, Values[k] );
            printf( "]\n" );

            if ( fVerbose )
            Saig_ManAnalizeControl( pTsi->pAig, Reg );
        }
    }
    Vec_IntShrink( pTsi->vNonXRegs, r );
    if ( fVerbose )
        printf( "Found %3d useful registers.\n", Vec_IntSize(pTsi->vNonXRegs) );
    return Vec_IntSize(pTsi->vNonXRegs);
}


/**Function*************************************************************

  Synopsis    [Mapping of AIG nodes into frames nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Aig_Obj_t * Saig_ObjFrames( Aig_Obj_t ** pObjMap, int nFs, Aig_Obj_t * pObj, int i )                       { return pObjMap[nFs*pObj->Id + i];  }
static inline void        Saig_ObjSetFrames( Aig_Obj_t ** pObjMap, int nFs, Aig_Obj_t * pObj, int i, Aig_Obj_t * pNode ) { pObjMap[nFs*pObj->Id + i] = pNode; }

static inline Aig_Obj_t * Saig_ObjChild0Frames( Aig_Obj_t ** pObjMap, int nFs, Aig_Obj_t * pObj, int i ) { return Aig_ObjFanin0(pObj)? Aig_NotCond(Saig_ObjFrames(pObjMap,nFs,Aig_ObjFanin0(pObj),i), Aig_ObjFaninC0(pObj)) : NULL;  }
static inline Aig_Obj_t * Saig_ObjChild1Frames( Aig_Obj_t ** pObjMap, int nFs, Aig_Obj_t * pObj, int i ) { return Aig_ObjFanin1(pObj)? Aig_NotCond(Saig_ObjFrames(pObjMap,nFs,Aig_ObjFanin1(pObj),i), Aig_ObjFaninC1(pObj)) : NULL;  }

/**Function*************************************************************

  Synopsis    [Performs phase abstraction by unrolling the circuit.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Saig_ManPerformAbstraction( Saig_Tsim_t * pTsi, int nFrames, int fVerbose )
{
    Aig_Man_t * pFrames, * pAig = pTsi->pAig;
    Aig_Obj_t * pObj, * pObjLi, * pObjLo, * pObjNew;
    Aig_Obj_t ** pObjMap;
    unsigned * pState;
    int i, f, Reg, Value;

    assert( Vec_IntSize(pTsi->vNonXRegs) > 0 );

    // create mapping for the frames nodes
    pObjMap = ABC_ALLOC( Aig_Obj_t *, nFrames * Aig_ManObjNumMax(pAig) );
    memset( pObjMap, 0, sizeof(Aig_Obj_t *) * nFrames * Aig_ManObjNumMax(pAig) );

    // start the fraig package
    pFrames = Aig_ManStart( Aig_ManObjNumMax(pAig) * nFrames );
    pFrames->pName = Abc_UtilStrsav( pAig->pName );
    pFrames->pSpec = Abc_UtilStrsav( pAig->pSpec );
    // map constant nodes
    for ( f = 0; f < nFrames; f++ )
        Saig_ObjSetFrames( pObjMap, nFrames, Aig_ManConst1(pAig), f, Aig_ManConst1(pFrames) );
    // create PI nodes for the frames
    for ( f = 0; f < nFrames; f++ )
        Aig_ManForEachPiSeq( pAig, pObj, i )
            Saig_ObjSetFrames( pObjMap, nFrames, pObj, f, Aig_ObjCreateCi(pFrames) );
    // create the latches
    Aig_ManForEachLoSeq( pAig, pObj, i )
        Saig_ObjSetFrames( pObjMap, nFrames, pObj, 0, Aig_ObjCreateCi(pFrames) );

    // add timeframes
    for ( f = 0; f < nFrames; f++ )
    {
        // replace abstracted registers by constants
        Vec_IntForEachEntry( pTsi->vNonXRegs, Reg, i )
        {
            pObj = Saig_ManLo( pAig, Reg );
            pState = (unsigned *)Vec_PtrEntry( pTsi->vStates, f );
            Value = (Abc_InfoHasBit( pState, 2 * Reg + 1 ) << 1) | Abc_InfoHasBit( pState, 2 * Reg );
            assert( Value == SAIG_XVS0 || Value == SAIG_XVS1 );
            pObjNew = (Value == SAIG_XVS1)? Aig_ManConst1(pFrames) : Aig_ManConst0(pFrames);
            Saig_ObjSetFrames( pObjMap, nFrames, pObj, f, pObjNew );
        }
        // add internal nodes of this frame
        Aig_ManForEachNode( pAig, pObj, i ) 
        {
            pObjNew = Aig_And( pFrames, Saig_ObjChild0Frames(pObjMap,nFrames,pObj,f), Saig_ObjChild1Frames(pObjMap,nFrames,pObj,f) );
            Saig_ObjSetFrames( pObjMap, nFrames, pObj, f, pObjNew );
        }
        // set the latch inputs and copy them into the latch outputs of the next frame
        Aig_ManForEachLiLoSeq( pAig, pObjLi, pObjLo, i )
        {
            pObjNew = Saig_ObjChild0Frames(pObjMap,nFrames,pObjLi,f);
            if ( f < nFrames - 1 )
                Saig_ObjSetFrames( pObjMap, nFrames, pObjLo, f+1, pObjNew );
        }
    }
    for ( f = 0; f < nFrames; f++ )
    {
        Aig_ManForEachPoSeq( pAig, pObj, i )
        {
            pObjNew = Aig_ObjCreateCo( pFrames, Saig_ObjChild0Frames(pObjMap,nFrames,pObj,f) );
            Saig_ObjSetFrames( pObjMap, nFrames, pObj, f, pObjNew );
        }
    }
    pFrames->nRegs = pAig->nRegs;
    pFrames->nTruePis = Aig_ManCiNum(pFrames) - Aig_ManRegNum(pFrames); 
    pFrames->nTruePos = Aig_ManCoNum(pFrames) - Aig_ManRegNum(pFrames); 
    Aig_ManForEachLiSeq( pAig, pObj, i )
    {
        pObjNew = Aig_ObjCreateCo( pFrames, Saig_ObjChild0Frames(pObjMap,nFrames,pObj,nFrames-1) );
        Saig_ObjSetFrames( pObjMap, nFrames, pObj, nFrames-1, pObjNew );
    }
//Aig_ManPrintStats( pFrames );
    Aig_ManSeqCleanup( pFrames );
//Aig_ManPrintStats( pFrames );
//    Aig_ManCiCleanup( pFrames );
//Aig_ManPrintStats( pFrames );
    ABC_FREE( pObjMap );
    return pFrames;
}


/**Function*************************************************************

  Synopsis    [Performs automated phase abstraction.]

  Description [Takes the AIG manager and the array of initial states.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Saig_ManPhaseFrameNum( Aig_Man_t * p, Vec_Int_t * vInits )
{
    Saig_Tsim_t * pTsi;
    int nFrames, nPrefix;
    assert( Saig_ManRegNum(p) );
    assert( Saig_ManPiNum(p) );
    assert( Saig_ManPoNum(p) );
    // perform terminary simulation
    pTsi = Saig_ManReachableTernary( p, vInits, 0 );
    if ( pTsi == NULL )
        return 1;
    // derive information
    nPrefix = Saig_TsiComputePrefix( pTsi, (unsigned *)Vec_PtrEntryLast(pTsi->vStates), pTsi->nWords );
    nFrames = Vec_PtrSize(pTsi->vStates) - 1 - nPrefix;
    Saig_TsiStop( pTsi );
    // potentially, may need to reduce nFrames if nPrefix is less than nFrames
    return nFrames;
}

/**Function*************************************************************

  Synopsis    [Performs automated phase abstraction.]

  Description [Takes the AIG manager and the array of initial states.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Saig_ManPhasePrefixLength( Aig_Man_t * p, int fVerbose, int fVeryVerbose, Vec_Int_t ** pvTrans )
{
    Saig_Tsim_t * pTsi;
    int nFrames, nPrefix, nNonXRegs;
    assert( Saig_ManRegNum(p) );
    assert( Saig_ManPiNum(p) );
    assert( Saig_ManPoNum(p) );
    // perform terminary simulation
    pTsi = Saig_ManReachableTernary( p, NULL, 0 );
    if ( pTsi == NULL )
        return 0;
    // derive information
    nPrefix   = Saig_TsiComputePrefix( pTsi, (unsigned *)Vec_PtrEntryLast(pTsi->vStates), pTsi->nWords );
    nFrames   = Vec_PtrSize(pTsi->vStates) - 1 - nPrefix;
    nNonXRegs = Saig_TsiCountNonXValuedRegisters( pTsi, nPrefix );

    if ( pvTrans )
        *pvTrans = Saig_TsiComputeTransient( pTsi, nPrefix );

    // print statistics
    if ( fVerbose )
        printf( "Lead = %5d. Loop = %5d.  Total flops = %5d. Binary flops = %5d.\n", nPrefix, nFrames, p->nRegs, nNonXRegs );
    if ( fVeryVerbose )
        Saig_TsiPrintTraces( pTsi, pTsi->nWords, nPrefix, nFrames );
    Saig_TsiStop( pTsi );
    // potentially, may need to reduce nFrames if nPrefix is less than nFrames
    return nPrefix;
}

/**Function*************************************************************

  Synopsis    [Performs automated phase abstraction.]

  Description [Takes the AIG manager and the array of initial states.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Saig_ManPhaseAbstract( Aig_Man_t * p, Vec_Int_t * vInits, int nFrames, int nPref, int fIgnore, int fPrint, int fVerbose )
{
    Aig_Man_t * pNew = NULL;
    Saig_Tsim_t * pTsi;
    assert( Saig_ManRegNum(p) );
    assert( Saig_ManPiNum(p) );
    assert( Saig_ManPoNum(p) );
    // perform terminary simulation
    pTsi = Saig_ManReachableTernary( p, vInits, fVerbose );
    if ( pTsi == NULL )
        return NULL;
    // derive information
    pTsi->nPrefix = Saig_TsiComputePrefix( pTsi, (unsigned *)Vec_PtrEntryLast(pTsi->vStates), pTsi->nWords );
    pTsi->nCycle = Vec_PtrSize(pTsi->vStates) - 1 - pTsi->nPrefix;
    pTsi->nNonXRegs = Saig_TsiCountNonXValuedRegisters(pTsi, Abc_MinInt(pTsi->nPrefix,nPref));
    // print statistics
    if ( fVerbose )
    {
        printf( "Lead = %5d. Loop = %5d.  Total flops = %5d. Binary flops = %5d.\n", 
            pTsi->nPrefix, pTsi->nCycle, p->nRegs, pTsi->nNonXRegs );
        if ( pTsi->nNonXRegs < 100 && Vec_PtrSize(pTsi->vStates) < 80 )
            Saig_TsiPrintTraces( pTsi, pTsi->nWords, pTsi->nPrefix, pTsi->nCycle );
    }
    if ( fPrint )
        printf( "Print-out finished. Phase assignment is not performed.\n" );
    else if ( nFrames < 2 )
        printf( "The number of frames is less than 2. Phase assignment is not performed.\n" );
    else if ( nFrames > 256 )
        printf( "The number of frames is more than 256. Phase assignment is not performed.\n" );
    else if ( pTsi->nCycle == 1 )
        printf( "The cycle of ternary states is trivial. Phase abstraction cannot be done.\n" );
    else if ( pTsi->nCycle % nFrames != 0 )
        printf( "The cycle (%d) is not modulo the number of frames (%d). Phase abstraction cannot be done.\n", pTsi->nCycle, nFrames );
    else if ( pTsi->nNonXRegs == 0 )
        printf( "All registers have X-valued states. Phase abstraction cannot be done.\n" );
    else if ( !Saig_ManFindRegisters( pTsi, nFrames, fIgnore, fVerbose ) )
        printf( "There is no registers to abstract with %d frames.\n", nFrames );
    else
        pNew = Saig_ManPerformAbstraction( pTsi, nFrames, fVerbose );
    Saig_TsiStop( pTsi );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Performs automated phase abstraction.]

  Description [Takes the AIG manager and the array of initial states.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Saig_ManPhaseAbstractAuto( Aig_Man_t * p, int fVerbose )
{
    Aig_Man_t * pNew = NULL;
    Saig_Tsim_t * pTsi;
    int fPrint = 0;
    int nFrames;
    assert( Saig_ManRegNum(p) );
    assert( Saig_ManPiNum(p) );
    assert( Saig_ManPoNum(p) );
    // perform terminary simulation
    pTsi = Saig_ManReachableTernary( p, NULL, fVerbose );
    if ( pTsi == NULL )
        return NULL;
    // derive information
    pTsi->nPrefix = Saig_TsiComputePrefix( pTsi, (unsigned *)Vec_PtrEntryLast(pTsi->vStates), pTsi->nWords );
    pTsi->nCycle = Vec_PtrSize(pTsi->vStates) - 1 - pTsi->nPrefix;
    pTsi->nNonXRegs = Saig_TsiCountNonXValuedRegisters(pTsi, 0);
    // print statistics
    if ( fVerbose )
    {
        printf( "Lead = %5d. Loop = %5d.  Total flops = %5d. Binary flops = %5d.\n", 
            pTsi->nPrefix, pTsi->nCycle, p->nRegs, pTsi->nNonXRegs );
        if ( pTsi->nNonXRegs < 100 && Vec_PtrSize(pTsi->vStates) < 80 )
            Saig_TsiPrintTraces( pTsi, pTsi->nWords, pTsi->nPrefix, pTsi->nCycle );
    }
    nFrames = pTsi->nCycle;
    if ( fPrint )
    {
        printf( "Print-out finished. Phase assignment is not performed.\n" );
    }
    else if ( nFrames < 2 )
    {
//        printf( "The number of frames is less than 2. Phase assignment is not performed.\n" );
    }
    else if ( nFrames > 256 )
    {
//        printf( "The number of frames is more than 256. Phase assignment is not performed.\n" );
    }
    else if ( pTsi->nCycle == 1 )
    {
//        printf( "The cycle of ternary states is trivial. Phase abstraction cannot be done.\n" );
    }
    else if ( pTsi->nCycle % nFrames != 0 )
    {
//        printf( "The cycle (%d) is not modulo the number of frames (%d). Phase abstraction cannot be done.\n", pTsi->nCycle, nFrames );
    }
    else if ( pTsi->nNonXRegs == 0 )
    {
//        printf( "All registers have X-valued states. Phase abstraction cannot be done.\n" );
    }
    else if ( !Saig_ManFindRegisters( pTsi, nFrames, 0, fVerbose ) )
    {
//        printf( "There is no registers to abstract with %d frames.\n", nFrames );
    }
    else
        pNew = Saig_ManPerformAbstraction( pTsi, nFrames, fVerbose );
    Saig_TsiStop( pTsi );
    if ( pNew == NULL )
        pNew = Aig_ManDupSimple( p );
    if ( Aig_ManCiNum(pNew) == Aig_ManRegNum(pNew) )
    {
        Aig_ManStop( pNew);
        pNew = Aig_ManDupSimple( p );
    }
    return pNew;
}


/**Function*************************************************************

  Synopsis    [Derives CEX for the original AIG from CEX of the unrolled AIG.]

  Description [The number of PIs of the given CEX should divide by the number 
  of PIs of the original AIG without a remainder.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Cex_t * Saig_PhaseTranslateCex( Aig_Man_t * p, Abc_Cex_t * pCex )
{
    Abc_Cex_t * pNew;
    int i, k, iFrame, nFrames;
    // make sure the PI count of the AIG is a multiple of the PI count of the CEX
    // if this is not true, the CEX is not derived as the result of unrolling of pAig
    // or the unrolled CEX went through transformations that change the PI count
    if ( pCex->nPis % Saig_ManPiNum(p) != 0 )
    {
        printf( "The PI count in the AIG and in the CEX do not match.\n" );
        return NULL;
    }
    // get the number of unrolled frames
    nFrames = pCex->nPis / Saig_ManPiNum(p);
    // get the frame where it fails
    iFrame = pCex->iFrame * nFrames + pCex->iPo / Saig_ManPoNum(p);
    // start a new CEX (assigns: p->nRegs, p->nPis, p->nBits)
    pNew = Abc_CexAlloc( Saig_ManRegNum(p), Saig_ManPiNum(p), iFrame+1 );
    assert( pNew->nBits == pNew->nPis * (iFrame + 1) + pNew->nRegs );
    // now assign the failed frame and the failed PO (p->iFrame and p->iPo)
    pNew->iFrame = iFrame;
    pNew->iPo    = pCex->iPo % Saig_ManPoNum(p);
    // copy the bit data
    for ( i = pCex->nRegs, k = pNew->nRegs; k < pNew->nBits; k++, i++ )
        if ( Abc_InfoHasBit( pCex->pData, i ) )
            Abc_InfoSetBit( pNew->pData, k );
    assert( i <= pCex->nBits );
    return pNew;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

