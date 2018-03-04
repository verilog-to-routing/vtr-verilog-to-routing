/**CFile****************************************************************

  FileName    [aigTsim.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [AIG package.]

  Synopsis    [Ternary simulation.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - April 28, 2007.]

  Revision    [$Id: aigTsim.c,v 1.00 2007/04/28 00:00:00 alanmi Exp $]

***********************************************************************/

#include "aig.h"
#include "aig/saig/saig.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#define TSI_MAX_ROUNDS    1000
#define TSI_ONE_SERIES     300

#define AIG_XVS0   1
#define AIG_XVS1   2
#define AIG_XVSX   3

static inline void Aig_ObjSetXsim( Aig_Obj_t * pObj, int Value )  { pObj->nCuts = Value;  }
static inline int  Aig_ObjGetXsim( Aig_Obj_t * pObj )             { return pObj->nCuts;   }
static inline int  Aig_XsimInv( int Value )   
{ 
    if ( Value == AIG_XVS0 )
        return AIG_XVS1;
    if ( Value == AIG_XVS1 )
        return AIG_XVS0;
    assert( Value == AIG_XVSX );       
    return AIG_XVSX;
}
static inline int  Aig_XsimAnd( int Value0, int Value1 )   
{ 
    if ( Value0 == AIG_XVS0 || Value1 == AIG_XVS0 )
        return AIG_XVS0;
    if ( Value0 == AIG_XVSX || Value1 == AIG_XVSX )
        return AIG_XVSX;
    assert( Value0 == AIG_XVS1 && Value1 == AIG_XVS1 );
    return AIG_XVS1;
}
static inline int  Aig_XsimRand2()   
{
    return (Aig_ManRandom(0) & 1) ? AIG_XVS1 : AIG_XVS0;
}
static inline int  Aig_XsimRand3()   
{
    int RetValue;
    do { 
        RetValue = Aig_ManRandom(0) & 3; 
    } while ( RetValue == 0 );
    return RetValue;
}
static inline int  Aig_ObjGetXsimFanin0( Aig_Obj_t * pObj )       
{ 
    int RetValue;
    RetValue = Aig_ObjGetXsim(Aig_ObjFanin0(pObj));
    return Aig_ObjFaninC0(pObj)? Aig_XsimInv(RetValue) : RetValue;
}
static inline int  Aig_ObjGetXsimFanin1( Aig_Obj_t * pObj )       
{ 
    int RetValue;
    RetValue = Aig_ObjGetXsim(Aig_ObjFanin1(pObj));
    return Aig_ObjFaninC1(pObj)? Aig_XsimInv(RetValue) : RetValue;
}
static inline void Aig_XsimPrint( FILE * pFile, int Value )   
{ 
    if ( Value == AIG_XVS0 )
    {
        fprintf( pFile, "0" );
        return;
    }
    if ( Value == AIG_XVS1 )
    {
        fprintf( pFile, "1" );
        return;
    }
    assert( Value == AIG_XVSX );       
    fprintf( pFile, "x" );
}

// simulation manager
typedef struct Aig_Tsi_t_ Aig_Tsi_t;
struct Aig_Tsi_t_
{
    Aig_Man_t *      pAig;              // the original AIG manager
    // ternary state representation
    int              nWords;            // the number of words in the states
    Vec_Ptr_t *      vStates;           // the collection of ternary states
    Aig_MmFixed_t *  pMem;              // memory for ternary states
    // hash table for terminary states
    unsigned **      pBins;
    int              nBins;
};

static inline unsigned * Aig_TsiNext( unsigned * pState, int nWords )                      { return *((unsigned **)(pState + nWords));  }
static inline void       Aig_TsiSetNext( unsigned * pState, int nWords, unsigned * pNext ) { *((unsigned **)(pState + nWords)) = pNext; }

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Allocates simulation manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Tsi_t * Aig_TsiStart( Aig_Man_t * pAig )
{
    Aig_Tsi_t * p;
    p = ABC_ALLOC( Aig_Tsi_t, 1 );
    memset( p, 0, sizeof(Aig_Tsi_t) );
    p->pAig    = pAig;
    p->nWords  = Abc_BitWordNum( 2*Aig_ManRegNum(pAig) );
    p->vStates = Vec_PtrAlloc( 1000 );
    p->pMem    = Aig_MmFixedStart( sizeof(unsigned) * p->nWords + sizeof(unsigned *), 10000 );
    p->nBins   = Abc_PrimeCudd(TSI_MAX_ROUNDS/2);
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
void Aig_TsiStop( Aig_Tsi_t * p )
{
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
int Aig_TsiStateHash( unsigned * pState, int nWords, int nTableSize )
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
int Aig_TsiStateLookup( Aig_Tsi_t * p, unsigned * pState, int nWords )
{
    unsigned * pEntry;
    int Hash;
    Hash = Aig_TsiStateHash( pState, nWords, p->nBins );
    for ( pEntry = p->pBins[Hash]; pEntry; pEntry = Aig_TsiNext(pEntry, nWords) )
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
void Aig_TsiStateInsert( Aig_Tsi_t * p, unsigned * pState, int nWords )
{
    int Hash = Aig_TsiStateHash( pState, nWords, p->nBins );
    assert( !Aig_TsiStateLookup( p, pState, nWords ) );
    Aig_TsiSetNext( pState, nWords, p->pBins[Hash] );
    p->pBins[Hash] = pState;    
}

/**Function*************************************************************

  Synopsis    [Inserts value into the table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned * Aig_TsiStateNew( Aig_Tsi_t * p )
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
void Aig_TsiStatePrint( Aig_Tsi_t * p, unsigned * pState )
{
    int i, Value, nZeros = 0, nOnes = 0, nDcs = 0;
    for ( i = 0; i < Aig_ManRegNum(p->pAig); i++ )
    {
        Value = (Abc_InfoHasBit( pState, 2 * i + 1 ) << 1) | Abc_InfoHasBit( pState, 2 * i );
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

  Synopsis    [Count constant values in the state.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aig_TsiStateCount( Aig_Tsi_t * p, unsigned * pState )
{
    Aig_Obj_t * pObjLi, * pObjLo;
    int i, Value, nCounter = 0;
    Aig_ManForEachLiLoSeq( p->pAig, pObjLi, pObjLo, i )
    {
        Value = (Abc_InfoHasBit( pState, 2 * i + 1 ) << 1) | Abc_InfoHasBit( pState, 2 * i );
        nCounter += (Value == 1 || Value == 2);
    }
    return nCounter;
}

/**Function*************************************************************

  Synopsis    [Count constant values in the state.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_TsiStateOrAll( Aig_Tsi_t * pTsi, unsigned * pState )
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
Vec_Ptr_t * Aig_ManTernarySimulate( Aig_Man_t * p, int fVerbose, int fVeryVerbose )
{
    Aig_Tsi_t * pTsi;
    Vec_Ptr_t * vMap;
    Aig_Obj_t * pObj, * pObjLi, * pObjLo;
    unsigned * pState;//, * pPrev;
    int i, f, fConstants, Value, nCounter, nRetired;
    // allocate the simulation manager
    pTsi = Aig_TsiStart( p );
    // initialize the values
    Aig_ObjSetXsim( Aig_ManConst1(p), AIG_XVS1 );
    Aig_ManForEachPiSeq( p, pObj, i )
        Aig_ObjSetXsim( pObj, AIG_XVSX );
    Aig_ManForEachLoSeq( p, pObj, i )
        Aig_ObjSetXsim( pObj, AIG_XVS0 );
    // simulate for the given number of timeframes
    for ( f = 0; f < TSI_MAX_ROUNDS; f++ )
    {
        // collect this state
        pState = Aig_TsiStateNew( pTsi );
        Aig_ManForEachLiLoSeq( p, pObjLi, pObjLo, i )
        {
            Value = Aig_ObjGetXsim(pObjLo);
            if ( Value & 1 )
                Abc_InfoSetBit( pState, 2 * i );
            if ( Value & 2 )
                Abc_InfoSetBit( pState, 2 * i + 1 );
        }

//        printf( "%d ", Aig_TsiStateCount(pTsi, pState) );
if ( fVeryVerbose )
{
printf( "%3d : ", f );
Aig_TsiStatePrint( pTsi, pState );
}
        // check if this state exists
        if ( Aig_TsiStateLookup( pTsi, pState, pTsi->nWords ) )
            break;
//        nCounter = 0;
//        Aig_ManForEachLiLoSeq( p, pObjLi, pObjLo, i )
//            nCounter += (Aig_ObjGetXsim(pObjLo) == AIG_XVS0);
//printf( "%d -> ", nCounter );
        // insert this state
        Aig_TsiStateInsert( pTsi, pState, pTsi->nWords );
        // simulate internal nodes
        Aig_ManForEachNode( p, pObj, i )
        {
            Aig_ObjSetXsim( pObj, Aig_XsimAnd(Aig_ObjGetXsimFanin0(pObj), Aig_ObjGetXsimFanin1(pObj)) );
//            printf( "%d %d    Id = %2d.  Value = %d.\n", 
//                Aig_ObjGetXsimFanin0(pObj), Aig_ObjGetXsimFanin1(pObj),
//                i, Aig_XsimAnd(Aig_ObjGetXsimFanin0(pObj), Aig_ObjGetXsimFanin1(pObj)) );
        }
        // transfer the latch values
        Aig_ManForEachLiSeq( p, pObj, i )
            Aig_ObjSetXsim( pObj, Aig_ObjGetXsimFanin0(pObj) );
        nCounter = 0;
        nRetired = 0;
        Aig_ManForEachLiLoSeq( p, pObjLi, pObjLo, i )
        {
            if ( f < TSI_ONE_SERIES )
                Aig_ObjSetXsim( pObjLo, Aig_ObjGetXsim(pObjLi) );
            else
            {
                if ( Aig_ObjGetXsim(pObjLi) != Aig_ObjGetXsim(pObjLo) )
                {
                    Aig_ObjSetXsim( pObjLo, AIG_XVSX );
                    nRetired++;
                }
            }
            nCounter += (Aig_ObjGetXsim(pObjLo) == AIG_XVS0);
        }
//        if ( nRetired )
//        printf( "Retired %d registers.\n", nRetired );

//        if ( f && (f % 1000 == 0) )
//            printf( "%d \n", f );
//printf( "%d  ", nCounter );
    }
//printf( "\n" );
    if ( f == TSI_MAX_ROUNDS )
    {
        printf( "Aig_ManTernarySimulate(): Did not reach a fixed point after %d iterations (not a bug).\n", TSI_MAX_ROUNDS );
        Aig_TsiStop( pTsi );
        return NULL;
    }
    // OR all the states
    pState = (unsigned *)Vec_PtrEntry( pTsi->vStates, 0 );
    Aig_TsiStateOrAll( pTsi, pState );
    // check if there are constants
    fConstants = 0;
    if ( 2*Aig_ManRegNum(p) == 32*pTsi->nWords )
    {
        for ( i = 0; i < pTsi->nWords; i++ )
            if ( pState[i] != ~0 )
                fConstants = 1;
    }
    else
    {
        for ( i = 0; i < pTsi->nWords - 1; i++ )
            if ( pState[i] != ~0 )
                fConstants = 1;
        if ( pState[i] != Abc_InfoMask( 2*Aig_ManRegNum(p) - 32*(pTsi->nWords-1) ) )
            fConstants = 1;
    }
    if ( fConstants == 0 )
    {
        if ( fVerbose )
        printf( "Detected 0 constants after %d iterations of ternary simulation.\n", f );
        Aig_TsiStop( pTsi );
        return NULL;
    }

    // start mapping by adding the true PIs
    vMap = Vec_PtrAlloc( Aig_ManCiNum(p) );
    Aig_ManForEachPiSeq( p, pObj, i )
        Vec_PtrPush( vMap, pObj );
    // find constant registers
    nCounter = 0;
    Aig_ManForEachLiLoSeq( p, pObjLi, pObjLo, i )
    {
        Value = (Abc_InfoHasBit( pState, 2 * i + 1 ) << 1) | Abc_InfoHasBit( pState, 2 * i );
        nCounter += (Value == 1 || Value == 2);
        if ( Value == 1 )
            Vec_PtrPush( vMap, Aig_ManConst0(p) );
        else if ( Value == 2 )
            Vec_PtrPush( vMap, Aig_ManConst1(p) );
        else if ( Value == 3 )
            Vec_PtrPush( vMap, pObjLo );
        else
            assert( 0 );
//        Aig_XsimPrint( stdout, Value );
    }
//    printf( "\n" );
    Aig_TsiStop( pTsi );
    if ( fVerbose )
    printf( "Detected %d constants after %d iterations of ternary simulation.\n", nCounter, f );
    return vMap;
}

/**Function*************************************************************

  Synopsis    [Reduces the circuit using ternary simulation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Aig_ManConstReduce( Aig_Man_t * p, int fUseMvSweep, int nFramesSymb, int nFramesSatur, int fVerbose, int fVeryVerbose )
{
    Aig_Man_t * pTemp;
    Vec_Ptr_t * vMap;
    while ( Aig_ManRegNum(p) > 0 )
    {
        if ( fUseMvSweep )
            vMap = Saig_MvManSimulate( p, nFramesSymb, nFramesSatur, fVerbose, fVeryVerbose );
        else
            vMap = Aig_ManTernarySimulate( p, fVerbose, fVeryVerbose );
        if ( vMap == NULL )
            break;
        p = Aig_ManRemap( pTemp = p, vMap );
        Vec_PtrFree( vMap );
        Aig_ManSeqCleanup( p );
        if ( fVerbose )
            Aig_ManReportImprovement( pTemp, p );
        Aig_ManStop( pTemp );
    }
    return p;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

