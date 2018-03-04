/**CFile****************************************************************

  FileName    [saigSimMv.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Sequential AIG package.]

  Synopsis    [Multi-valued simulation.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: saigSimMv.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "saig.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#define SAIG_DIFF_VALUES  8
#define SAIG_UNDEF_VALUE  0x1ffffffe  //536870910

// old AIG
typedef struct Saig_MvObj_t_ Saig_MvObj_t;
struct Saig_MvObj_t_
{
    int              iFan0;
    int              iFan1;
    unsigned         Type   :  3;
    unsigned         Value  : 29;
};

// new AIG
typedef struct Saig_MvAnd_t_ Saig_MvAnd_t;
struct Saig_MvAnd_t_
{
    int              iFan0;
    int              iFan1;
    int              iNext;
};

// simulation manager
typedef struct Saig_MvMan_t_ Saig_MvMan_t;
struct Saig_MvMan_t_
{
    // user data
    Aig_Man_t *      pAig;         // original AIG    
    // parameters
    int              nStatesMax;   // maximum number of states
    int              nLevelsMax;   // maximum number of levels
    int              nValuesMax;   // maximum number of values
    int              nFlops;       // number of flops
    // compacted AIG
    Saig_MvObj_t *   pAigOld;      // AIG objects
    Vec_Ptr_t *      vFlops;       // collected flops
    Vec_Int_t *      vXFlops;      // flops that had at least one X-value
    Vec_Ptr_t *      vTired;       // collected flops
    unsigned *       pTStates;     // hash table for states
    int              nTStatesSize; // hash table size
    Aig_MmFixed_t *  pMemStates;   // memory for states
    Vec_Ptr_t *      vStates;      // reached states
    int *            pRegsUndef;   // count the number of undef values
    int **           pRegsValues;  // write the first different values
    int *            nRegsValues;  // count the number of different values
    int              nRUndefs;     // the number of undef registers
    int              nRValues[SAIG_DIFF_VALUES+1]; // the number of registers with given values
    // internal AIG
    Saig_MvAnd_t *   pAigNew;      // AIG nodes
    int              nObjsAlloc;   // the number of objects allocated 
    int              nObjs;        // the number of objects
    int              nPis;         // the number of primary inputs
    int *            pTNodes;      // hash table
    int              nTNodesSize;  // hash table size
    unsigned char *  pLevels;      // levels of AIG nodes
};

static inline int    Saig_MvObjFaninC0( Saig_MvObj_t * pObj )  { return pObj->iFan0 & 1;           }
static inline int    Saig_MvObjFaninC1( Saig_MvObj_t * pObj )  { return pObj->iFan1 & 1;           }
static inline int    Saig_MvObjFanin0( Saig_MvObj_t * pObj )   { return pObj->iFan0 >> 1;          }
static inline int    Saig_MvObjFanin1( Saig_MvObj_t * pObj )   { return pObj->iFan1 >> 1;          }

static inline int    Saig_MvConst0()                           { return 1;                         }
static inline int    Saig_MvConst1()                           { return 0;                         }
static inline int    Saig_MvConst( int c )                     { return !c;                        }
static inline int    Saig_MvUndef()                            { return SAIG_UNDEF_VALUE;          }

static inline int    Saig_MvIsConst0( int iNode )              { return iNode == 1;                }
static inline int    Saig_MvIsConst1( int iNode )              { return iNode == 0;                }
static inline int    Saig_MvIsConst( int iNode )               { return iNode  < 2;                }
static inline int    Saig_MvIsUndef( int iNode )               { return iNode == SAIG_UNDEF_VALUE; }

static inline int    Saig_MvRegular( int iNode )               { return (iNode & ~01);             }
static inline int    Saig_MvNot( int iNode )                   { return (iNode ^  01);             }
static inline int    Saig_MvNotCond( int iNode, int c )        { return (iNode ^ (c));             }
static inline int    Saig_MvIsComplement( int iNode )          { return (int)(iNode & 01);         }

static inline int    Saig_MvLit2Var( int iNode )               { return (iNode >> 1);              }
static inline int    Saig_MvVar2Lit( int iVar )                { return (iVar << 1);               }
static inline int    Saig_MvLev( Saig_MvMan_t * p, int iNode ) { return p->pLevels[iNode >> 1];    }

// iterator over compacted objects
#define Saig_MvManForEachObj( pAig, pEntry ) \
    for ( pEntry = pAig; pEntry->Type != AIG_OBJ_VOID; pEntry++ )

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Creates reduced manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Saig_MvObj_t * Saig_ManCreateReducedAig( Aig_Man_t * p, Vec_Ptr_t ** pvFlops )
{
    Saig_MvObj_t * pAig, * pEntry;
    Aig_Obj_t * pObj;
    int i;
    *pvFlops = Vec_PtrAlloc( Aig_ManRegNum(p) );
    pAig = ABC_CALLOC( Saig_MvObj_t, Aig_ManObjNumMax(p)+1 );
    Aig_ManForEachObj( p, pObj, i )
    {
        pEntry = pAig + i;
        pEntry->Type = pObj->Type;
        if ( Aig_ObjIsCi(pObj) || i == 0 )
        {
            if ( Saig_ObjIsLo(p, pObj) )
            {
                pEntry->iFan0 = (Saig_ObjLoToLi(p, pObj)->Id << 1);
                pEntry->iFan1 = -1;
                Vec_PtrPush( *pvFlops, pEntry );
            }
            continue;
        }
        pEntry->iFan0 = (Aig_ObjFaninId0(pObj) << 1) | Aig_ObjFaninC0(pObj);
        if ( Aig_ObjIsCo(pObj) )
            continue;
        assert( Aig_ObjIsNode(pObj) );
        pEntry->iFan1 = (Aig_ObjFaninId1(pObj) << 1) | Aig_ObjFaninC1(pObj);
    }
    pEntry = pAig + Aig_ManObjNumMax(p);
    pEntry->Type = AIG_OBJ_VOID;
    return pAig;
}

/**Function*************************************************************

  Synopsis    [Creates a new node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Saig_MvCreateObj( Saig_MvMan_t * p, int iFan0, int iFan1 )
{
    Saig_MvAnd_t * pNode;
    if ( p->nObjs == p->nObjsAlloc )
    {
        p->pAigNew = ABC_REALLOC( Saig_MvAnd_t, p->pAigNew, 2*p->nObjsAlloc );
        p->pLevels = ABC_REALLOC( unsigned char, p->pLevels, 2*p->nObjsAlloc );
        p->nObjsAlloc *= 2;
    }
    pNode = p->pAigNew + p->nObjs;
    pNode->iFan0 = iFan0;
    pNode->iFan1 = iFan1;
    pNode->iNext = 0;
    if ( iFan0 || iFan1 )
        p->pLevels[p->nObjs] = 1 + Abc_MaxInt( Saig_MvLev(p, iFan0), Saig_MvLev(p, iFan1) );
    else
        p->pLevels[p->nObjs] = 0, p->nPis++;
    return p->nObjs++;
}

/**Function*************************************************************

  Synopsis    [Creates multi-valued simulation manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Saig_MvMan_t * Saig_MvManStart( Aig_Man_t * pAig, int nFramesSatur )
{
    Saig_MvMan_t * p;
    int i;
    assert( Aig_ManRegNum(pAig) > 0 );
    p = (Saig_MvMan_t *)ABC_ALLOC( Saig_MvMan_t, 1 );
    memset( p, 0, sizeof(Saig_MvMan_t) );
    // set parameters
    p->pAig         = pAig;
    p->nStatesMax   = 2 * nFramesSatur + 100;
    p->nLevelsMax   = 4;
    p->nValuesMax   = SAIG_DIFF_VALUES;
    p->nFlops       = Aig_ManRegNum(pAig);
    // compacted AIG
    p->pAigOld      = Saig_ManCreateReducedAig( pAig, &p->vFlops );
    p->nTStatesSize = Abc_PrimeCudd( p->nStatesMax );
    p->pTStates     = ABC_CALLOC( unsigned, p->nTStatesSize );
    p->pMemStates   = Aig_MmFixedStart( sizeof(int) * (p->nFlops+1), p->nStatesMax );
    p->vStates      = Vec_PtrAlloc( p->nStatesMax );
    Vec_PtrPush( p->vStates, NULL );
    p->pRegsUndef   = ABC_CALLOC( int, p->nFlops );
    p->pRegsValues  = ABC_ALLOC( int *, p->nFlops );
    p->pRegsValues[0] = ABC_ALLOC( int, p->nValuesMax * p->nFlops );
    for ( i = 1; i < p->nFlops; i++ )
        p->pRegsValues[i] = p->pRegsValues[i-1] + p->nValuesMax;
    p->nRegsValues  = ABC_CALLOC( int, p->nFlops );
    p->vTired       = Vec_PtrAlloc( 100 );
    // internal AIG
    p->nObjsAlloc   = 1000000;
    p->pAigNew      = ABC_ALLOC( Saig_MvAnd_t, p->nObjsAlloc );
    p->nTNodesSize  = Abc_PrimeCudd( p->nObjsAlloc / 3 );
    p->pTNodes      = ABC_CALLOC( int, p->nTNodesSize );
    p->pLevels      = ABC_ALLOC( unsigned char, p->nObjsAlloc );
    Saig_MvCreateObj( p, 0, 0 );
    return p;
}

/**Function*************************************************************

  Synopsis    [Destroys multi-valued simulation manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Saig_MvManStop( Saig_MvMan_t * p )
{
    Aig_MmFixedStop( p->pMemStates, 0 );
    Vec_PtrFree( p->vStates );
    Vec_IntFreeP( &p->vXFlops );
    Vec_PtrFree( p->vFlops );
    Vec_PtrFree( p->vTired );
    ABC_FREE( p->pRegsValues[0] );
    ABC_FREE( p->pRegsValues );
    ABC_FREE( p->nRegsValues );
    ABC_FREE( p->pRegsUndef );
    ABC_FREE( p->pAigOld );
    ABC_FREE( p->pTStates );
    ABC_FREE( p->pAigNew );
    ABC_FREE( p->pTNodes );
    ABC_FREE( p->pLevels );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    [Hashing the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Saig_MvHash( int iFan0, int iFan1, int TableSize ) 
{
    unsigned Key = 0;
    assert( iFan0 < iFan1 );
    Key ^= Saig_MvLit2Var(iFan0) * 7937;
    Key ^= Saig_MvLit2Var(iFan1) * 2971;
    Key ^= Saig_MvIsComplement(iFan0) * 911;
    Key ^= Saig_MvIsComplement(iFan1) * 353;
    return (int)(Key % TableSize);
}

/**Function*************************************************************

  Synopsis    [Returns the place where this node is stored (or should be stored).]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int * Saig_MvTableFind( Saig_MvMan_t * p, int iFan0, int iFan1 )
{
    Saig_MvAnd_t * pEntry;
    int * pPlace = p->pTNodes + Saig_MvHash( iFan0, iFan1, p->nTNodesSize );
    for ( pEntry = (*pPlace)? p->pAigNew + *pPlace : NULL; pEntry; 
          pPlace = &pEntry->iNext, pEntry = (*pPlace)? p->pAigNew + *pPlace : NULL )
              if ( pEntry->iFan0 == iFan0 && pEntry->iFan1 == iFan1 )
                  break;
    return pPlace;
}

/**Function*************************************************************

  Synopsis    [Performs an AND-operation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Saig_MvAnd( Saig_MvMan_t * p, int iFan0, int iFan1, int fFirst )
{
    if ( iFan0 == iFan1 )
        return iFan0;
    if ( iFan0 == Saig_MvNot(iFan1) )
        return Saig_MvConst0();
    if ( Saig_MvIsConst(iFan0) )
        return Saig_MvIsConst1(iFan0) ? iFan1 : Saig_MvConst0();
    if ( Saig_MvIsConst(iFan1) )
        return Saig_MvIsConst1(iFan1) ? iFan0 : Saig_MvConst0();
    if ( Saig_MvIsUndef(iFan0) || Saig_MvIsUndef(iFan1) )
        return Saig_MvUndef();
//    if ( Saig_MvLev(p, iFan0) >= p->nLevelsMax || Saig_MvLev(p, iFan1) >= p->nLevelsMax )
//        return Saig_MvUndef();

    // go undef after the first frame
    if ( !fFirst )
        return Saig_MvUndef();

    if ( iFan0 > iFan1 )
    {
        int Temp = iFan0;
        iFan0 = iFan1;
        iFan1 = Temp;
    }
    {
        int * pPlace;
        pPlace = Saig_MvTableFind( p, iFan0, iFan1 );
        if ( *pPlace == 0 )
        {
            if ( pPlace >= (int*)p->pAigNew && pPlace < (int*)(p->pAigNew + p->nObjsAlloc) )
            {
                int iPlace = pPlace - (int*)p->pAigNew;
                int iNode  = Saig_MvCreateObj( p, iFan0, iFan1 );
                ((int*)p->pAigNew)[iPlace] = iNode;
                return Saig_MvVar2Lit( iNode );
            }
            else
                *pPlace = Saig_MvCreateObj( p, iFan0, iFan1 );
        }
        return Saig_MvVar2Lit( *pPlace );
    }
}

/**Function*************************************************************

  Synopsis    [Propagates one edge.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Saig_MvSimulateValue0( Saig_MvObj_t * pAig, Saig_MvObj_t * pObj )
{
    Saig_MvObj_t * pObj0 = pAig + Saig_MvObjFanin0(pObj);
    if ( Saig_MvIsUndef( pObj0->Value ) )
        return Saig_MvUndef();
    return Saig_MvNotCond( pObj0->Value, Saig_MvObjFaninC0(pObj) );
}
static inline int Saig_MvSimulateValue1( Saig_MvObj_t * pAig, Saig_MvObj_t * pObj )
{
    Saig_MvObj_t * pObj1 = pAig + Saig_MvObjFanin1(pObj);
    if ( Saig_MvIsUndef( pObj1->Value ) )
        return Saig_MvUndef();
    return Saig_MvNotCond( pObj1->Value, Saig_MvObjFaninC1(pObj) );
}

/**Function*************************************************************

  Synopsis    [Prints MV state.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Saig_MvPrintState( int Iter, Saig_MvMan_t * p )
{
    Saig_MvObj_t * pEntry;
    int i;
    printf( "%3d : ", Iter );
    Vec_PtrForEachEntry( Saig_MvObj_t *, p->vFlops, pEntry, i )
    {
        if ( pEntry->Value == SAIG_UNDEF_VALUE )
            printf( "    *" );
        else
            printf( "%5d", pEntry->Value );
    }
    printf( "\n" );
}

/**Function*************************************************************

  Synopsis    [Performs one iteration of simulation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Saig_MvSimulateFrame( Saig_MvMan_t * p, int fFirst, int fVerbose )
{
    Saig_MvObj_t * pEntry;
    int i;
    Saig_MvManForEachObj( p->pAigOld, pEntry )
    {
        if ( pEntry->Type == AIG_OBJ_AND )
        {
            pEntry->Value = Saig_MvAnd( p, 
                Saig_MvSimulateValue0(p->pAigOld, pEntry),
                Saig_MvSimulateValue1(p->pAigOld, pEntry), fFirst );
        }
        else if ( pEntry->Type == AIG_OBJ_CO )
            pEntry->Value = Saig_MvSimulateValue0(p->pAigOld, pEntry);
        else if ( pEntry->Type == AIG_OBJ_CI )
        {
            if ( pEntry->iFan1 == 0 ) // true PI
            {
                if ( fFirst )
                    pEntry->Value = Saig_MvVar2Lit( Saig_MvCreateObj( p, 0, 0 ) );
                else
                    pEntry->Value = SAIG_UNDEF_VALUE;
            }
//            else if ( fFirst ) // register output
//                pEntry->Value = Saig_MvConst0();
//            else
//                pEntry->Value = Saig_MvSimulateValue0(p->pAigOld, pEntry);
        }
        else if ( pEntry->Type == AIG_OBJ_CONST1 )
            pEntry->Value = Saig_MvConst1();
        else if ( pEntry->Type != AIG_OBJ_NONE )
            assert( 0 );
    }
    // transfer to registers
    Vec_PtrForEachEntry( Saig_MvObj_t *, p->vFlops, pEntry, i )
        pEntry->Value = Saig_MvSimulateValue0( p->pAigOld, pEntry );
}


/**Function*************************************************************

  Synopsis    [Computes hash value of the node using its simulation info.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Saig_MvSimHash( unsigned * pState, int nFlops, int TableSize )
{
    static int s_SPrimes[16] = { 
     1610612741,
     805306457,
     402653189,
     201326611,
     100663319,
     50331653,
     25165843,
     12582917,
     6291469,
     3145739,
     1572869,
     786433,
     393241,
     196613,
     98317,
     49157
    };
    unsigned uHash = 0;
    int i;
    for ( i = 0; i < nFlops; i++ )
        uHash ^= pState[i] * s_SPrimes[i & 0xF];
    return (int)(uHash % TableSize);
}

/**Function*************************************************************

  Synopsis    [Returns the place where this state is stored (or should be stored).]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline unsigned * Saig_MvSimTableFind( Saig_MvMan_t * p, unsigned * pState )
{
    unsigned * pEntry;
    unsigned * pPlace = p->pTStates + Saig_MvSimHash( pState+1, p->nFlops, p->nTStatesSize );
    for ( pEntry = (*pPlace)? (unsigned *)Vec_PtrEntry(p->vStates, *pPlace) : NULL; pEntry; 
          pPlace = pEntry, pEntry = (*pPlace)? (unsigned *)Vec_PtrEntry(p->vStates, *pPlace) : NULL )
              if ( memcmp( pEntry+1, pState+1, sizeof(int)*p->nFlops ) == 0 )
                  break;
    return pPlace;
}

/**Function*************************************************************

  Synopsis    [Saves current state.]

  Description [Returns -1 if there is no fixed point.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Saig_MvSaveState( Saig_MvMan_t * p )
{
    Saig_MvObj_t * pEntry;
    unsigned * pState, * pPlace;
    int i;
    pState = (unsigned *)Aig_MmFixedEntryFetch( p->pMemStates );
    pState[0] = 0;
    Vec_PtrForEachEntry( Saig_MvObj_t *, p->vFlops, pEntry, i )
        pState[i+1] = pEntry->Value;
    pPlace = Saig_MvSimTableFind( p, pState );
    if ( *pPlace )
        return *pPlace;
    *pPlace = Vec_PtrSize( p->vStates );
    Vec_PtrPush( p->vStates, pState );
    return -1;
}

/**Function*************************************************************

  Synopsis    [Performs multi-valued simulation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Saig_MvManPostProcess( Saig_MvMan_t * p, int iState )
{
    Saig_MvObj_t * pEntry;
    unsigned * pState;
    int i, k, j, nTotal = 0, iFlop;
    Vec_Int_t * vUniques = Vec_IntAlloc( 100 );
    Vec_Int_t * vCounter = Vec_IntAlloc( 100 );
    // count registers that never became undef
    Vec_PtrForEachEntry( Saig_MvObj_t *, p->vFlops, pEntry, i )
        if ( p->pRegsUndef[i] == 0 )
            nTotal++;
    printf( "The number of registers that never became undef = %d. (Total = %d.)\n", nTotal, p->nFlops );
    Vec_PtrForEachEntry( Saig_MvObj_t *, p->vFlops, pEntry, i )
    {
        if ( p->pRegsUndef[i] )
            continue;
        Vec_IntForEachEntry( vUniques, iFlop, k )
        {
            Vec_PtrForEachEntryStart( unsigned *, p->vStates, pState, j, 1 )
                if ( pState[iFlop+1] != pState[i+1] )
                    break;
            if ( j == Vec_PtrSize(p->vStates) )
            {
                Vec_IntAddToEntry( vCounter, k, 1 );
                break;
            }
        }
        if ( k == Vec_IntSize(vUniques) )
        {
            Vec_IntPush( vUniques, i );
            Vec_IntPush( vCounter, 1 );
        }
    }
    Vec_IntForEachEntry( vUniques, iFlop, i )
    {
        printf( "FLOP %5d : (%3d) ", iFlop, Vec_IntEntry(vCounter,i) );
/*
        for ( k = 0; k < p->nRegsValues[iFlop]; k++ )
            if ( p->pRegsValues[iFlop][k] == SAIG_UNDEF_VALUE )
                printf( "* " );
            else
                printf( "%d ", p->pRegsValues[iFlop][k] );
        printf( "\n" );
*/
        Vec_PtrForEachEntryStart( unsigned *, p->vStates, pState, k, 1 )
        {
            if ( k == iState+1 )
                printf( " # " );
            if ( pState[iFlop+1] == SAIG_UNDEF_VALUE )
                printf( "*" );
            else
                printf( "%d", pState[iFlop+1] );
        }
        printf( "\n" );
//        if ( ++Counter == 10 )
//            break;
    }

    Vec_IntFree( vUniques );
    Vec_IntFree( vCounter );
}

/**Function*************************************************************

  Synopsis    [Performs multi-valued simulation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Saig_MvManFindXFlops( Saig_MvMan_t * p )
{
    Vec_Int_t * vXFlops;
    unsigned * pState;
    int i, k;
    vXFlops = Vec_IntStart( p->nFlops );
    Vec_PtrForEachEntryStart( unsigned *, p->vStates, pState, i, 1 )
    {
        for ( k = 0; k < p->nFlops; k++ )
            if ( Saig_MvIsUndef(pState[k+1]) )
                Vec_IntWriteEntry( vXFlops, k, 1 );
    }
    return vXFlops;
}

/**Function*************************************************************

  Synopsis    [Checks if the flop is an oscilator.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Saig_MvManCheckOscilator( Saig_MvMan_t * p, int iFlop )
{
    Vec_Int_t * vValues;
    unsigned * pState;
    int k, Per, Entry;
    // collect values of this flop
    vValues = Vec_IntAlloc( 100 );
    Vec_PtrForEachEntryStart( unsigned *, p->vStates, pState, k, 1 )
    {
        Vec_IntPush( vValues, pState[iFlop+1] );
//printf( "%d ", pState[iFlop+1] );
    }
//printf( "\n" );
    assert( Saig_MvIsConst0( Vec_IntEntry(vValues,0) ) );
    // look for constants
    for ( Per = 0; Per < Vec_IntSize(vValues)/2; Per++ )
    {
        // find the first non-const0
        Vec_IntForEachEntryStart( vValues, Entry, Per, Per )
            if ( !Saig_MvIsConst0(Entry) )
                break;
        if ( Per == Vec_IntSize(vValues) )
            break;
        // find the first const0
        Vec_IntForEachEntryStart( vValues, Entry, Per, Per )
            if ( Saig_MvIsConst0(Entry) )
                break;
        if ( Per == Vec_IntSize(vValues) )
            break;
        // try to determine period
        assert( Saig_MvIsConst0( Vec_IntEntry(vValues,Per) ) );
        for ( k = Per; k < Vec_IntSize(vValues); k++ )
            if ( Vec_IntEntry(vValues, k-Per) != Vec_IntEntry(vValues, k) )
                break;
        if ( k < Vec_IntSize(vValues) )
            continue;
        Vec_IntFree( vValues );
//printf( "Period = %d\n", Per );
        return Per;
    }
    Vec_IntFree( vValues );
    return 0;
}

/**Function*************************************************************

  Synopsis    [Returns const0 and binary flops.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Saig_MvManFindConstBinaryFlops( Saig_MvMan_t * p, Vec_Int_t ** pvBinary )
{
    unsigned * pState;
    Vec_Int_t * vBinary, * vConst0;
    int i, k, fConst0;
    // detect constant flops
    vConst0 = Vec_IntAlloc( p->nFlops );
    vBinary = Vec_IntAlloc( p->nFlops );
    for ( k = 0; k < p->nFlops; k++ )
    {
        // check if this flop is constant 0 in all states
        fConst0 = 1;
        Vec_PtrForEachEntryStart( unsigned *, p->vStates, pState, i, 1 )
        {
            if ( !Saig_MvIsConst0(pState[k+1]) )
                fConst0 = 0;
            if ( Saig_MvIsUndef(pState[k+1]) )
                break;
        }
        if ( i < Vec_PtrSize(p->vStates) )
            continue;
        // the flop is binary-valued        
        if ( fConst0 ) 
            Vec_IntPush( vConst0, k );
        else
            Vec_IntPush( vBinary, k );
    }
    *pvBinary = vBinary;
    return vConst0;
}

/**Function*************************************************************

  Synopsis    [Find oscilators.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Saig_MvManFindOscilators( Saig_MvMan_t * p, Vec_Int_t ** pvConst0 )
{
    Vec_Int_t * vBinary, * vOscils;
    int Entry, i;
    // detect constant flops
    *pvConst0 = Saig_MvManFindConstBinaryFlops( p, &vBinary );
    // check binary flops
    vOscils = Vec_IntAlloc( 100 );
    Vec_IntForEachEntry( vBinary, Entry, i )
        if ( Saig_MvManCheckOscilator( p, Entry ) )
            Vec_IntPush( vOscils, Entry );
    Vec_IntFree( vBinary );
    return vOscils;
}

/**Function*************************************************************

  Synopsis    [Find constants and oscilators.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Saig_MvManCreateNextSkip( Saig_MvMan_t * p )
{
    Vec_Int_t * vConst0, * vOscils, * vXFlops;
    int i, Entry;
    vOscils = Saig_MvManFindOscilators( p, &vConst0 );
//printf( "Found %d constants and %d oscilators.\n", Vec_IntSize(vConst0), Vec_IntSize(vOscils) );
    vXFlops = Vec_IntAlloc( p->nFlops );
    Vec_IntFill( vXFlops, p->nFlops, 1 );
    Vec_IntForEachEntry( vConst0, Entry, i )
        Vec_IntWriteEntry( vXFlops, Entry, 0 );
    Vec_IntForEachEntry( vOscils, Entry, i )
        Vec_IntWriteEntry( vXFlops, Entry, 0 );
    Vec_IntFree( vOscils );
    Vec_IntFree( vConst0 );
    return vXFlops;
}

/**Function*************************************************************

  Synopsis    [Finds equivalent flops.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Saig_MvManDeriveMap( Saig_MvMan_t * p, int fVerbose )
{
    Vec_Int_t * vConst0, * vBinValued;
    Vec_Ptr_t * vMap = NULL;
    Aig_Obj_t * pObj;
    unsigned * pState;
    int i, k, j, FlopK, FlopJ;
    int Counter1 = 0, Counter2 = 0;
    // prepare CI map
    vMap = Vec_PtrAlloc( Aig_ManCiNum(p->pAig) );
    Aig_ManForEachCi( p->pAig, pObj, i )
        Vec_PtrPush( vMap, pObj );
    // detect constant flops
    vConst0 = Saig_MvManFindConstBinaryFlops( p, &vBinValued );
    // set constants
    Vec_IntForEachEntry( vConst0, FlopK, k )
    {
        Vec_PtrWriteEntry( vMap, Saig_ManPiNum(p->pAig) + FlopK, Aig_ManConst0(p->pAig) );
        Counter1++;
    }
    Vec_IntFree( vConst0 ); 

    // detect equivalent (non-ternary flops)
    Vec_IntForEachEntry( vBinValued, FlopK, k )
    if ( FlopK >= 0 )
    Vec_IntForEachEntryStart( vBinValued, FlopJ, j, k+1 )
    if ( FlopJ >= 0 )
    {
        // check if they are equal
        Vec_PtrForEachEntryStart( unsigned *, p->vStates, pState, i, 1 )
            if ( pState[FlopK+1] != pState[FlopJ+1] )
                break;
        if ( i < Vec_PtrSize(p->vStates) )
            continue;
        // set the equivalence
        Vec_PtrWriteEntry( vMap, Saig_ManPiNum(p->pAig) + FlopJ, Saig_ManLo(p->pAig, FlopK) );
        Vec_IntWriteEntry( vBinValued, j, -1 );
        Counter2++;
    }
    if ( fVerbose )
    printf( "Detected %d const0 flops and %d pairs of equiv binary flops.\n", Counter1, Counter2 );
    Vec_IntFree( vBinValued );
    if ( Counter1 == 0 && Counter2 == 0 )
        Vec_PtrFreeP( &vMap );
    return vMap;
}

/**Function*************************************************************

  Synopsis    [Performs multi-valued simulation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Saig_MvManSimulate( Aig_Man_t * pAig, int nFramesSymb, int nFramesSatur, int fVerbose, int fVeryVerbose )
{
    Vec_Ptr_t * vMap;
    Saig_MvMan_t * p;
    Saig_MvObj_t * pEntry;
    int f, i, iState;
    abctime clk = Abc_Clock();
    assert( nFramesSymb >= 1 && nFramesSymb <= nFramesSatur );

    // start manager
    p = Saig_MvManStart( pAig, nFramesSatur );
if ( fVerbose )
ABC_PRT( "Constructing the problem", Abc_Clock() - clk );

    // initialize registers
    Vec_PtrForEachEntry( Saig_MvObj_t *, p->vFlops, pEntry, i )
        pEntry->Value = Saig_MvConst0();
    Saig_MvSaveState( p );
    if ( fVeryVerbose )
        Saig_MvPrintState( 0, p );
    // simulate until convergence
    clk = Abc_Clock();
    for ( f = 0; ; f++ )
    { 
        if ( f == nFramesSatur )
        {
            if ( fVerbose )
            printf( "Begining to saturate simulation after %d frames\n", f );
            // find all flops that have at least one X value in the past and set them to X forever
            p->vXFlops = Saig_MvManFindXFlops( p );            
        }
        if ( f == 2 * nFramesSatur )
        {
            if ( fVerbose )
            printf( "Agressively saturating simulation after %d frames\n", f );
            Vec_IntFree( p->vXFlops );
            p->vXFlops = Saig_MvManCreateNextSkip( p );
        }
        // retire some flops
        if ( p->vXFlops )
        {
            Vec_PtrForEachEntry( Saig_MvObj_t *, p->vFlops, pEntry, i )
                if ( Vec_IntEntry( p->vXFlops, i ) )
                    pEntry->Value = SAIG_UNDEF_VALUE;
        }
        // simulate timeframe
        Saig_MvSimulateFrame( p, (int)(f < nFramesSymb), fVerbose );
        // save and print state
        iState = Saig_MvSaveState( p );
        if ( fVeryVerbose )
            Saig_MvPrintState( f+1, p );
        if ( iState >= 0 )
        {
            if ( fVerbose )
            printf( "Converged after %d frames with lasso in state %d. Cycle = %d.\n", f+1, iState-1, f+2-iState );
//            printf( "Total number of PIs = %d. AND nodes = %d.\n", p->nPis, p->nObjs - p->nPis );
            break;
        }
    }
//    printf( "Coverged after %d frames.\n", f );
if ( fVerbose )
ABC_PRT( "Multi-valued simulation", Abc_Clock() - clk );
    // implement equivalences
//    Saig_MvManPostProcess( p, iState-1 );
    vMap = Saig_MvManDeriveMap( p, fVerbose );
    Saig_MvManStop( p );
//    return Aig_ManDupSimple( pAig );
    return vMap;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

