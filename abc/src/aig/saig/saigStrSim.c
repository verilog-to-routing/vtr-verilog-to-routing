/**CFile****************************************************************

  FileName    [saigStrSim.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Sequential AIG package.]

  Synopsis    [Structural matching using simulation.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: saigStrSim.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "saig.h"
#include "proof/ssw/ssw.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#define SAIG_WORDS  16

static inline Aig_Obj_t *  Saig_ObjNext( Aig_Obj_t ** ppNexts, Aig_Obj_t * pObj )                       { return ppNexts[pObj->Id];  }
static inline void         Saig_ObjSetNext( Aig_Obj_t ** ppNexts, Aig_Obj_t * pObj, Aig_Obj_t * pNext ) { ppNexts[pObj->Id] = pNext; }

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Computes hash value of the node using its simulation info.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned Saig_StrSimHash( Aig_Obj_t * pObj )
{
    static int s_SPrimes[128] = {
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
    unsigned * pSims;
    unsigned uHash = 0;
    int i;
    assert( SAIG_WORDS <= 128 );
    pSims = (unsigned *)pObj->pData;
    for ( i = 0; i < SAIG_WORDS; i++ )
        uHash ^= pSims[i] * s_SPrimes[i & 0x7F];
    return uHash;
}

/**Function*************************************************************

  Synopsis    [Computes hash value of the node using its simulation info.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Saig_StrSimIsEqual( Aig_Obj_t * pObj0, Aig_Obj_t * pObj1 )
{
    unsigned * pSims0 = (unsigned *)pObj0->pData;
    unsigned * pSims1 = (unsigned *)pObj1->pData;
    int i;
    for ( i = 0; i < SAIG_WORDS; i++ )
        if ( pSims0[i] != pSims1[i] )
            return 0;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if simulation info is zero.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Saig_StrSimIsZero( Aig_Obj_t * pObj )
{
    unsigned * pSims = (unsigned *)pObj->pData;
    int i;
    for ( i = 0; i < SAIG_WORDS; i++ )
        if ( pSims[i] != 0 )
            return 0;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if simulation info is one.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Saig_StrSimIsOne( Aig_Obj_t * pObj )
{
    unsigned * pSims = (unsigned *)pObj->pData;
    int i;
    for ( i = 0; i < SAIG_WORDS; i++ )
        if ( pSims[i] != ~0 )
            return 0;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Assigns random simulation info.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Saig_StrSimAssignRandom( Aig_Obj_t * pObj )
{
    unsigned * pSims = (unsigned *)pObj->pData;
    int i;
    for ( i = 0; i < SAIG_WORDS; i++ )
        pSims[i] = Aig_ManRandom(0);
}

/**Function*************************************************************

  Synopsis    [Assigns constant 0 simulation info.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Saig_StrSimAssignOne( Aig_Obj_t * pObj )
{
    unsigned * pSims = (unsigned *)pObj->pData;
    int i;
    for ( i = 0; i < SAIG_WORDS; i++ )
        pSims[i] = ~0;
}

/**Function*************************************************************

  Synopsis    [Assigns constant 0 simulation info.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Saig_StrSimAssignZeroInit( Aig_Obj_t * pObj )
{
    unsigned * pSims = (unsigned *)pObj->pData;
    pSims[0] = 0;
}

/**Function*************************************************************

  Synopsis    [Simulated one node.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Saig_StrSimulateNode( Aig_Obj_t * pObj, int i )
{
    unsigned * pSims  = (unsigned *)pObj->pData;
    unsigned * pSims0 = (unsigned *)Aig_ObjFanin0(pObj)->pData;
    unsigned * pSims1 = (unsigned *)Aig_ObjFanin1(pObj)->pData;
    if ( Aig_ObjFaninC0(pObj) && Aig_ObjFaninC1(pObj) )
        pSims[i] = ~(pSims0[i] | pSims1[i]);
    else if ( Aig_ObjFaninC0(pObj) && !Aig_ObjFaninC1(pObj) )
        pSims[i] = (~pSims0[i] & pSims1[i]);
    else if ( !Aig_ObjFaninC0(pObj) && Aig_ObjFaninC1(pObj) )
        pSims[i] = (pSims0[i] & ~pSims1[i]);
    else // if ( !Aig_ObjFaninC0(pObj) && !Aig_ObjFaninC1(pObj) )
        pSims[i] = (pSims0[i] & pSims1[i]);
}

/**Function*************************************************************

  Synopsis    [Saves output of one node.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Saig_StrSimSaveOutput( Aig_Obj_t * pObj, int i )
{
    unsigned * pSims = (unsigned *)pObj->pData;
    unsigned * pSims0 = (unsigned *)Aig_ObjFanin0(pObj)->pData;
    if ( Aig_ObjFaninC0(pObj) )
        pSims[i] = ~pSims0[i];
    else
        pSims[i] = pSims0[i];
}

/**Function*************************************************************

  Synopsis    [Transfers simulation output to another node.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Saig_StrSimTransfer( Aig_Obj_t * pObj0, Aig_Obj_t * pObj1 )
{
    unsigned * pSims0 = (unsigned *)pObj0->pData;
    unsigned * pSims1 = (unsigned *)pObj1->pData;
    int i;
    for ( i = 0; i < SAIG_WORDS; i++ )
        pSims1[i] = pSims0[i];
}

/**Function*************************************************************

  Synopsis    [Transfers simulation output to another node.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Saig_StrSimTransferNext( Aig_Obj_t * pObj0, Aig_Obj_t * pObj1, int i )
{
    unsigned * pSims0 = (unsigned *)pObj0->pData;
    unsigned * pSims1 = (unsigned *)pObj1->pData;
    assert( i < SAIG_WORDS - 1 );
    pSims1[i+1] = pSims0[i];
}

/**Function*************************************************************

  Synopsis    [Perform one round of simulation.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Saig_StrSimulateRound( Aig_Man_t * p0, Aig_Man_t * p1 )
{
    Aig_Obj_t * pObj0, * pObj1;
    int f, i;
    // simulate the nodes
    Aig_ManForEachObj( p0, pObj0, i )
    {
        if ( !Aig_ObjIsCi(pObj0) && !Aig_ObjIsNode(pObj0) )
           continue;
        pObj1 = Aig_ObjRepr(p0, pObj0);
        if ( pObj1 == NULL )
            continue;
        assert( Aig_ObjRepr(p1, pObj1) == pObj0 );
        Saig_StrSimAssignRandom( pObj0 );
        Saig_StrSimTransfer( pObj0, pObj1 );
    }
    // simulate the timeframes
    for ( f = 0; f < SAIG_WORDS; f++ )
    {
        // simulate the first AIG
        Aig_ManForEachNode( p0, pObj0, i )
            if ( Aig_ObjRepr(p0, pObj0) == NULL )
                Saig_StrSimulateNode( pObj0, f );
        Saig_ManForEachLi( p0, pObj0, i )
            Saig_StrSimSaveOutput( pObj0, f );
        if ( f < SAIG_WORDS - 1 )
            Saig_ManForEachLiLo( p0, pObj0, pObj1, i )
                Saig_StrSimTransferNext( pObj0, pObj1, f );
        // simulate the second AIG
        Aig_ManForEachNode( p1, pObj1, i )
            if ( Aig_ObjRepr(p1, pObj1) == NULL )
                Saig_StrSimulateNode( pObj1, f );
        Saig_ManForEachLi( p1, pObj1, i )
            Saig_StrSimSaveOutput( pObj1, f );
        if ( f < SAIG_WORDS - 1 )
            Saig_ManForEachLiLo( p1, pObj1, pObj0, i )
                Saig_StrSimTransferNext( pObj1, pObj0, f );
    }
}

/**Function*************************************************************

  Synopsis    [Checks if the entry exists in the table.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Obj_t * Saig_StrSimTableLookup( Aig_Obj_t ** ppTable, Aig_Obj_t ** ppNexts, int nTableSize, Aig_Obj_t * pObj )
{
    Aig_Obj_t * pEntry;
    int iEntry;
    // find the hash entry
    iEntry = Saig_StrSimHash( pObj ) % nTableSize;
    // check if there are nodes with this signatures
    for ( pEntry = ppTable[iEntry]; pEntry; pEntry = Saig_ObjNext(ppNexts,pEntry) )
        if ( Saig_StrSimIsEqual( pEntry, pObj ) )
            return pEntry;
    return NULL;
}

/**Function*************************************************************

  Synopsis    [Inserts the entry into the table.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Saig_StrSimTableInsert( Aig_Obj_t ** ppTable, Aig_Obj_t ** ppNexts, int nTableSize, Aig_Obj_t * pObj )
{
    // find the hash entry
    int iEntry = Saig_StrSimHash( pObj ) % nTableSize;
    // check if there are nodes with this signatures
    if ( ppTable[iEntry] == NULL )
        ppTable[iEntry] = pObj;
    else
    {
        Saig_ObjSetNext( ppNexts, pObj, Saig_ObjNext(ppNexts, ppTable[iEntry]) );
        Saig_ObjSetNext( ppNexts, ppTable[iEntry], pObj );
    }
}

/**Function*************************************************************

  Synopsis    [Perform one round of matching.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int Saig_StrSimDetectUnique( Aig_Man_t * p0, Aig_Man_t * p1 )
{
    Aig_Obj_t ** ppTable, ** ppNexts, ** ppCands;
    Aig_Obj_t * pObj, * pEntry;
    int i, nTableSize, Counter;

    // allocate the hash table hashing simulation info into nodes
    nTableSize = Abc_PrimeCudd( Aig_ManObjNum(p0)/2 );
    ppTable = ABC_CALLOC( Aig_Obj_t *, nTableSize );
    ppNexts = ABC_CALLOC( Aig_Obj_t *, Aig_ManObjNumMax(p0) );
    ppCands = ABC_CALLOC( Aig_Obj_t *, Aig_ManObjNumMax(p0) );

    // hash nodes of the first AIG
    Aig_ManForEachObj( p0, pObj, i )
    {
        if ( !Aig_ObjIsCi(pObj) && !Aig_ObjIsNode(pObj) )
            continue;
        if ( Aig_ObjRepr(p0, pObj) )
            continue;
        if ( Saig_StrSimIsZero(pObj) || Saig_StrSimIsOne(pObj) )
            continue;
        // check if the entry exists
        pEntry = Saig_StrSimTableLookup( ppTable, ppNexts, nTableSize, pObj );
        if ( pEntry == NULL ) // insert
            Saig_StrSimTableInsert( ppTable, ppNexts, nTableSize, pObj );
        else // mark the entry as not unique
            pEntry->fMarkA = 1;
    }

    // hash nodes from the second AIG
    Aig_ManForEachObj( p1, pObj, i )
    {
        if ( !Aig_ObjIsCi(pObj) && !Aig_ObjIsNode(pObj) )
            continue;
        if ( Aig_ObjRepr(p1, pObj) )
            continue;
        if ( Saig_StrSimIsZero(pObj) || Saig_StrSimIsOne(pObj) )
            continue;
        // check if the entry exists
        pEntry = Saig_StrSimTableLookup( ppTable, ppNexts, nTableSize, pObj );
        if ( pEntry == NULL ) // skip
            continue;
        // if there is no candidate, label it
        if ( Saig_ObjNext( ppCands, pEntry ) == NULL )
            Saig_ObjSetNext( ppCands, pEntry, pObj );
        else // mark the entry as not unique
            pEntry->fMarkA = 1;
    }

    // create representatives for the unique entries
    Counter = 0;
    for ( i = 0; i < nTableSize; i++ )
        for ( pEntry = ppTable[i]; pEntry; pEntry = Saig_ObjNext(ppNexts,pEntry) )
            if ( !pEntry->fMarkA && (pObj = Saig_ObjNext( ppCands, pEntry )) )
            {
//                assert( Aig_ObjIsNode(pEntry) == Aig_ObjIsNode(pObj) );
                if ( Aig_ObjType(pEntry) != Aig_ObjType(pObj) )
                    continue;
                Aig_ObjSetRepr( p0, pEntry, pObj );
                Aig_ObjSetRepr( p1, pObj, pEntry );
                Counter++;
            }

    // cleanup
    Aig_ManCleanMarkA( p0 );
    ABC_FREE( ppTable );
    ABC_FREE( ppNexts );
    ABC_FREE( ppCands );
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Counts the number of matched flops.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int Saig_StrSimCountMatchedFlops( Aig_Man_t * p )
{
    Aig_Obj_t * pObj;
    int i, Counter = 0;
    Saig_ManForEachLo( p, pObj, i )
        if ( Aig_ObjRepr(p, pObj) )
            Counter++;
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Counts the number of matched nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Saig_StrSimCountMatchedNodes( Aig_Man_t * p )
{
    Aig_Obj_t * pObj;
    int i, Counter = 0;
    Aig_ManForEachNode( p, pObj, i )
        if ( Aig_ObjRepr(p, pObj) )
            Counter++;
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Performs structural matching of two AIGs using simulation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Saig_StrSimPrepareAig( Aig_Man_t * p )
{
    Aig_Obj_t * pObj;
    int i;
    Aig_ManReprStart( p, Aig_ManObjNumMax(p) );
    // allocate simulation info
    p->pData2 = Vec_PtrAllocSimInfo( Aig_ManObjNumMax(p), SAIG_WORDS );
    Aig_ManForEachObj( p, pObj, i )
        pObj->pData = Vec_PtrEntry( (Vec_Ptr_t *)p->pData2, i );
    // set simulation info for constant1 and register outputs
    Saig_StrSimAssignOne( Aig_ManConst1(p) );
    Saig_ManForEachLo( p, pObj, i )
        Saig_StrSimAssignZeroInit( pObj );
}

/**Function*************************************************************

  Synopsis    [Performs structural matching of two AIGs using simulation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Saig_StrSimSetInitMatching( Aig_Man_t * p0, Aig_Man_t * p1 )
{
    Aig_Obj_t * pObj0, * pObj1;
    int i;
    pObj0 = Aig_ManConst1( p0 );
    pObj1 = Aig_ManConst1( p1 );
    Aig_ObjSetRepr( p0, pObj0, pObj1 );
    Aig_ObjSetRepr( p1, pObj1, pObj0 );
    Saig_ManForEachPi( p0, pObj0, i )
    {
        pObj1 = Aig_ManCi( p1, i );
        Aig_ObjSetRepr( p0, pObj0, pObj1 );
        Aig_ObjSetRepr( p1, pObj1, pObj0 );
    }
}

/**Function*************************************************************

  Synopsis    [Performs structural matching of two AIGs using simulation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Saig_StrSimSetFinalMatching( Aig_Man_t * p0, Aig_Man_t * p1 )
{
    Aig_Obj_t * pObj0, * pObj1;
    Aig_Obj_t * pFanin00, * pFanin01;
    Aig_Obj_t * pFanin10, * pFanin11;
    int i, CountAll = 0, CountNot = 0;
    Aig_ManIncrementTravId( p0 );
    Aig_ManForEachObj( p0, pObj0, i )
    {
        pObj1 = Aig_ObjRepr( p0, pObj0 );
        if ( pObj1 == NULL )
            continue;
        CountAll++;
        assert( pObj0 == Aig_ObjRepr( p1, pObj1 ) );
        if ( Aig_ObjIsNode(pObj0) )
        {
            assert( Aig_ObjIsNode(pObj1) );
            pFanin00 = Aig_ObjFanin0(pObj0);
            pFanin01 = Aig_ObjFanin1(pObj0);
            pFanin10 = Aig_ObjFanin0(pObj1);
            pFanin11 = Aig_ObjFanin1(pObj1);
            if ( Aig_ObjRepr(p0, pFanin00) != pFanin10 ||
                 Aig_ObjRepr(p0, pFanin01) != pFanin11 )
            {
                Aig_ObjSetTravIdCurrent(p0, pObj0);
                CountNot++;
            }
        }
        else if ( Saig_ObjIsLo(p0, pObj0) )
        {
            assert( Saig_ObjIsLo(p1, pObj1) );
            pFanin00 = Aig_ObjFanin0( Saig_ObjLoToLi(p0, pObj0) );
            pFanin10 = Aig_ObjFanin0( Saig_ObjLoToLi(p1, pObj1) );
            if ( Aig_ObjRepr(p0, pFanin00) != pFanin10 )
            {
                Aig_ObjSetTravIdCurrent(p0, pObj0);
                CountNot++;
            }
        }
    }
    // remove irrelevant matches
    Aig_ManForEachObj( p0, pObj0, i )
    {
        pObj1 = Aig_ObjRepr( p0, pObj0 );
        if ( pObj1 == NULL )
            continue;
        assert( pObj0 == Aig_ObjRepr( p1, pObj1 ) );
        if ( Aig_ObjIsTravIdCurrent( p0, pObj0 ) )
        {
            Aig_ObjSetRepr( p0, pObj0, NULL );
            Aig_ObjSetRepr( p1, pObj1, NULL );
        }
    }
    Abc_Print( 1, "Total matches = %6d.  Wrong matches = %6d.  Ratio = %5.2f %%\n",
        CountAll, CountNot, 100.0*CountNot/CountAll );
}

/**Function*************************************************************

  Synopsis    [Returns the number of dangling nodes removed.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Saig_StrSimSetContiguousMatching_rec( Aig_Man_t * p, Aig_Obj_t * pObj )
{
    Aig_Obj_t * pFanout;
    int i, iFanout = -1;
    if ( Aig_ObjIsTravIdCurrent(p, pObj) )
        return;
    Aig_ObjSetTravIdCurrent(p, pObj);
    if ( Saig_ObjIsPo( p, pObj ) )
        return;
    if ( Saig_ObjIsLi( p, pObj ) )
    {
        Saig_StrSimSetContiguousMatching_rec( p, Saig_ObjLiToLo(p, pObj) );
        return;
    }
    assert( Aig_ObjIsCi(pObj) || Aig_ObjIsNode(pObj) );
    if ( Aig_ObjRepr(p, pObj) == NULL )
        return;
    // go through the fanouts
    Aig_ObjForEachFanout( p, pObj, pFanout, iFanout, i )
        Saig_StrSimSetContiguousMatching_rec( p, pFanout );
    // go through the fanins
    if ( !Aig_ObjIsCi( pObj ) )
    {
        Saig_StrSimSetContiguousMatching_rec( p, Aig_ObjFanin0(pObj) );
        Saig_StrSimSetContiguousMatching_rec( p, Aig_ObjFanin1(pObj) );
    }
}

/**Function*************************************************************

  Synopsis    [Performs structural matching of two AIGs using simulation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Saig_StrSimSetContiguousMatching( Aig_Man_t * p0, Aig_Man_t * p1 )
{
    Aig_Obj_t * pObj0, * pObj1;
    int i, CountAll = 0, CountNot = 0;
    // mark nodes reachable through the PIs
    Aig_ManIncrementTravId( p0 );
    Aig_ObjSetTravIdCurrent( p0, Aig_ManConst1(p0) );
    Saig_ManForEachPi( p0, pObj0, i )
        Saig_StrSimSetContiguousMatching_rec( p0, pObj0 );
    // remove irrelevant matches
    Aig_ManForEachObj( p0, pObj0, i )
    {
        pObj1 = Aig_ObjRepr( p0, pObj0 );
        if ( pObj1 == NULL )
            continue;
        CountAll++;
        assert( pObj0 == Aig_ObjRepr( p1, pObj1 ) );
        if ( !Aig_ObjIsTravIdCurrent( p0, pObj0 ) )
        {
            Aig_ObjSetRepr( p0, pObj0, NULL );
            Aig_ObjSetRepr( p1, pObj1, NULL );
            CountNot++;
        }
    }
    Abc_Print( 1, "Total matches = %6d.  Wrong matches = %6d.  Ratio = %5.2f %%\n",
        CountAll, CountNot, 100.0*CountNot/CountAll );
}



/**Function*************************************************************

  Synopsis    [Establishes relationship between nodes using pairing.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ssw_StrSimMatchingExtendOne( Aig_Man_t * p, Vec_Ptr_t * vNodes )
{
    Aig_Obj_t * pNext, * pObj;
    int i, k, iFan = -1;
    Vec_PtrClear( vNodes );
    Aig_ManIncrementTravId( p );
    Aig_ManForEachObj( p, pObj, i )
    {
        if ( !Aig_ObjIsNode(pObj) && !Aig_ObjIsCi(pObj) )
            continue;
        if ( Aig_ObjRepr( p, pObj ) != NULL )
            continue;
        if ( Saig_ObjIsLo(p, pObj) )
        {
            pNext = Saig_ObjLoToLi(p, pObj);
            pNext = Aig_ObjFanin0(pNext);
            if ( Aig_ObjRepr( p, pNext ) && !Aig_ObjIsTravIdCurrent(p, pNext) && !Aig_ObjIsConst1(pNext) )
            {
                Aig_ObjSetTravIdCurrent(p, pNext);
                Vec_PtrPush( vNodes, pNext );
            }
        }
        if ( Aig_ObjIsNode(pObj) )
        {
            pNext = Aig_ObjFanin0(pObj);
            if ( Aig_ObjRepr( p, pNext )&& !Aig_ObjIsTravIdCurrent(p, pNext) )
            {
                Aig_ObjSetTravIdCurrent(p, pNext);
                Vec_PtrPush( vNodes, pNext );
            }
            pNext = Aig_ObjFanin1(pObj);
            if ( Aig_ObjRepr( p, pNext ) && !Aig_ObjIsTravIdCurrent(p, pNext) )
            {
                Aig_ObjSetTravIdCurrent(p, pNext);
                Vec_PtrPush( vNodes, pNext );
            }
        }
        Aig_ObjForEachFanout( p, pObj, pNext, iFan, k )
        {
            if ( Saig_ObjIsPo(p, pNext) )
                continue;
            if ( Saig_ObjIsLi(p, pNext) )
                pNext = Saig_ObjLiToLo(p, pNext);
            if ( Aig_ObjRepr( p, pNext ) && !Aig_ObjIsTravIdCurrent(p, pNext) )
            {
                Aig_ObjSetTravIdCurrent(p, pNext);
                Vec_PtrPush( vNodes, pNext );
            }
        }
    }
}

/**Function*************************************************************

  Synopsis    [Establishes relationship between nodes using pairing.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ssw_StrSimMatchingCountUnmached( Aig_Man_t * p )
{
    Aig_Obj_t * pObj;
    int i, Counter = 0;
    Aig_ManForEachObj( p, pObj, i )
    {
        if ( !Aig_ObjIsNode(pObj) && !Aig_ObjIsCi(pObj) )
            continue;
        if ( Aig_ObjRepr( p, pObj ) != NULL )
            continue;
        Counter++;
    }
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Establishes relationship between nodes using pairing.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ssw_StrSimMatchingExtend( Aig_Man_t * p0, Aig_Man_t * p1, int nDist, int fVerbose )
{
    Vec_Ptr_t * vNodes0, * vNodes1;
    Aig_Obj_t * pNext0, * pNext1;
    int d, k;
    vNodes0 = Vec_PtrAlloc( 1000 );
    vNodes1 = Vec_PtrAlloc( 1000 );
    if ( fVerbose )
    {
        int nUnmached = Ssw_StrSimMatchingCountUnmached(p0);
        Abc_Print( 1, "Extending islands by %d steps:\n", nDist );
        Abc_Print( 1, "%2d : Total = %6d. Unmatched = %6d.  Ratio = %6.2f %%\n",
            0, Aig_ManCiNum(p0) + Aig_ManNodeNum(p0),
            nUnmached, 100.0 * nUnmached/(Aig_ManCiNum(p0) + Aig_ManNodeNum(p0)) );
    }
    for ( d = 0; d < nDist; d++ )
    {
        Ssw_StrSimMatchingExtendOne( p0, vNodes0 );
        Ssw_StrSimMatchingExtendOne( p1, vNodes1 );
        Vec_PtrForEachEntry( Aig_Obj_t *, vNodes0, pNext0, k )
        {
            pNext1 = Aig_ObjRepr( p0, pNext0 );
            if ( pNext1 == NULL )
                continue;
            assert( pNext0 == Aig_ObjRepr( p1, pNext1 ) );
            if ( Saig_ObjIsPi(p1, pNext1) )
                continue;
            Aig_ObjSetRepr( p0, pNext0, NULL );
            Aig_ObjSetRepr( p1, pNext1, NULL );
        }
        Vec_PtrForEachEntry( Aig_Obj_t *, vNodes1, pNext1, k )
        {
            pNext0 = Aig_ObjRepr( p1, pNext1 );
            if ( pNext0 == NULL )
                continue;
            assert( pNext1 == Aig_ObjRepr( p0, pNext0 ) );
            if ( Saig_ObjIsPi(p0, pNext0) )
                continue;
            Aig_ObjSetRepr( p0, pNext0, NULL );
            Aig_ObjSetRepr( p1, pNext1, NULL );
        }
        if ( fVerbose )
        {
            int nUnmached = Ssw_StrSimMatchingCountUnmached(p0);
            Abc_Print( 1, "%2d : Total = %6d. Unmatched = %6d.  Ratio = %6.2f %%\n",
                d+1, Aig_ManCiNum(p0) + Aig_ManNodeNum(p0),
                nUnmached, 100.0 * nUnmached/(Aig_ManCiNum(p0) + Aig_ManNodeNum(p0)) );
        }
    }
    Vec_PtrFree( vNodes0 );
    Vec_PtrFree( vNodes1 );
}


/**Function*************************************************************

  Synopsis    [Performs structural matching of two AIGs using simulation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Saig_StrSimPerformMatching( Aig_Man_t * p0, Aig_Man_t * p1, int nDist, int fVerbose, Aig_Man_t ** ppMiter )
{
    extern Aig_Man_t * Saig_ManWindowExtractMiter( Aig_Man_t * p0, Aig_Man_t * p1 );

    Vec_Int_t * vPairs;
    Aig_Man_t * pPart0, * pPart1;
    Aig_Obj_t * pObj0, * pObj1;
    int i, nMatches;
    abctime clk, clkTotal = Abc_Clock();
    Aig_ManRandom( 1 );
    // consider the case when a miter is given
    if ( p1 == NULL )
    {
        if ( fVerbose )
        {
            Aig_ManPrintStats( p0 );
        }
        // demiter the miter
        if ( !Saig_ManDemiterSimpleDiff( p0, &pPart0, &pPart1 ) )
        {
            Abc_Print( 1, "Demitering has failed.\n" );
            return NULL;
        }
    }
    else
    {
        pPart0 = Aig_ManDupSimple( p0 );
        pPart1 = Aig_ManDupSimple( p1 );
    }
    if ( fVerbose )
    {
        Aig_ManPrintStats( pPart0 );
        Aig_ManPrintStats( pPart1 );
    }
    // start simulation 
    Saig_StrSimPrepareAig( pPart0 );
    Saig_StrSimPrepareAig( pPart1 );
    Saig_StrSimSetInitMatching( pPart0, pPart1 );
    if ( fVerbose )
    {
        Abc_Print( 1, "Allocated %6.2f MB to simulate the first AIG.\n",
            1.0 * Aig_ManObjNumMax(pPart0) * SAIG_WORDS * sizeof(unsigned) / (1<<20) );
        Abc_Print( 1, "Allocated %6.2f MB to simulate the second AIG.\n",
            1.0 * Aig_ManObjNumMax(pPart1) * SAIG_WORDS * sizeof(unsigned) / (1<<20) );
    }
    // iterate matching
    nMatches = 1;
    for ( i = 0; nMatches > 0; i++ )
    {
        clk = Abc_Clock();
        Saig_StrSimulateRound( pPart0, pPart1 );
        nMatches = Saig_StrSimDetectUnique( pPart0, pPart1 );
        if ( fVerbose )
        {
            int nFlops = Saig_StrSimCountMatchedFlops(pPart0);
            int nNodes = Saig_StrSimCountMatchedNodes(pPart0);
            Abc_Print( 1, "%3d : Match =%6d.  FF =%6d. (%6.2f %%)  Node =%6d. (%6.2f %%)  ",
                i, nMatches,
                nFlops, 100.0*nFlops/Aig_ManRegNum(pPart0),
                nNodes, 100.0*nNodes/Aig_ManNodeNum(pPart0) );
            ABC_PRT( "Time", Abc_Clock() - clk );
        }
        if ( i == 20 )
            break;
    }
    // cleanup
    Vec_PtrFree( (Vec_Ptr_t *)pPart0->pData2 ); pPart0->pData2 = NULL;
    Vec_PtrFree( (Vec_Ptr_t *)pPart1->pData2 ); pPart1->pData2 = NULL;
    // extend the islands
    Aig_ManFanoutStart( pPart0 );
    Aig_ManFanoutStart( pPart1 );
    if ( nDist )
        Ssw_StrSimMatchingExtend( pPart0, pPart1, nDist, fVerbose );
    Saig_StrSimSetFinalMatching( pPart0, pPart1 );
//    Saig_StrSimSetContiguousMatching( pPart0, pPart1 );
    // copy the results into array
    vPairs = Vec_IntAlloc( 2*Aig_ManObjNumMax(pPart0) );
    Aig_ManForEachObj( pPart0, pObj0, i )
    {
        pObj1 = Aig_ObjRepr(pPart0, pObj0);
        if ( pObj1 == NULL )
            continue;
        assert( pObj0 == Aig_ObjRepr(pPart1, pObj1) );
        Vec_IntPush( vPairs, pObj0->Id );
        Vec_IntPush( vPairs, pObj1->Id );
    }
    // this procedure adds matching of PO and LI
    if ( ppMiter )
        *ppMiter = Saig_ManWindowExtractMiter( pPart0, pPart1 );
    Aig_ManFanoutStop( pPart0 );
    Aig_ManFanoutStop( pPart1 );
    Aig_ManStop( pPart0 );
    Aig_ManStop( pPart1 );
    ABC_PRT( "Total runtime", Abc_Clock() - clkTotal );
    return vPairs;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END
