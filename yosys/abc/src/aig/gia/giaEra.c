/**CFile****************************************************************

  FileName    [giaEra.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Scalable AIG package.]

  Synopsis    [Explicit reachability analysis.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: giaEra.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "gia.h"
#include "misc/mem/mem.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

// explicit state representation
typedef struct Gia_ObjEra_t_ Gia_ObjEra_t;
struct Gia_ObjEra_t_
{
    int            Num;          // ID of this state
    int            Cond;         // input condition
    int            iPrev;        // previous state
    int            iNext;        // next state in the hash table
    unsigned       pData[0];     // state bits
};

// explicit state reachability
typedef struct Gia_ManEra_t_ Gia_ManEra_t;
struct Gia_ManEra_t_
{
    Gia_Man_t *    pAig;         // user's AIG manager
    int            nWordsSim;       // 2^(PInum)
    int            nWordsDat;   // Abc_BitWordNum
    unsigned *     pDataSim;     // simulation data
    Mem_Fixed_t *  pMemory;      // memory manager
    Vec_Ptr_t *    vStates;      // reached states
    Gia_ObjEra_t * pStateNew;    // temporary state
    int            iCurState;    // the current state
    Vec_Int_t *    vBugTrace;    // the sequence of transitions
    Vec_Int_t *    vStgDump;     // STG written into a file
    // hash table for states
    int            nBins;         
    unsigned *     pBins;
};

static inline unsigned *     Gia_ManEraData( Gia_ManEra_t * p, int i )    { return p->pDataSim + i * p->nWordsSim;  }
static inline Gia_ObjEra_t * Gia_ManEraState( Gia_ManEra_t * p, int i )   { return (Gia_ObjEra_t *)Vec_PtrEntry(p->vStates, i);  }

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Creates reachability manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_ManEra_t * Gia_ManEraCreate( Gia_Man_t * pAig )
{
    Vec_Ptr_t * vTruths;
    Gia_ManEra_t * p;
    unsigned * pTruth, * pSimInfo;
    int i;
    p = ABC_CALLOC( Gia_ManEra_t, 1 );
    p->pAig      = pAig;
    p->nWordsSim = Abc_TruthWordNum( Gia_ManPiNum(pAig) );
    p->nWordsDat = Abc_BitWordNum( Gia_ManRegNum(pAig) );
    p->pDataSim  = ABC_ALLOC( unsigned, p->nWordsSim*Gia_ManObjNum(pAig) );
    p->pMemory   = Mem_FixedStart( sizeof(Gia_ObjEra_t) + sizeof(unsigned) * p->nWordsDat );
    p->vStates   = Vec_PtrAlloc( 100000 );
    p->nBins     = Abc_PrimeCudd( 100000 );
    p->pBins     = ABC_CALLOC( unsigned, p->nBins );
    Vec_PtrPush( p->vStates, NULL );
    // assign primary input values
    vTruths = Vec_PtrAllocTruthTables( Gia_ManPiNum(pAig) );
    Vec_PtrForEachEntry( unsigned *, vTruths, pTruth, i )
    {
        pSimInfo = Gia_ManEraData( p, Gia_ObjId(pAig, Gia_ManPi(pAig, i)) );
        memcpy( pSimInfo, pTruth, sizeof(unsigned) * p->nWordsSim );
    }
    Vec_PtrFree( vTruths );
    // assign constant zero node
    pSimInfo = Gia_ManEraData( p, 0 );
    memset( pSimInfo, 0, sizeof(unsigned) * p->nWordsSim );
    p->vStgDump = Vec_IntAlloc( 1000 );
    return p;
}

/**Function*************************************************************

  Synopsis    [Deletes reachability manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManEraFree( Gia_ManEra_t * p )
{
    Mem_FixedStop( p->pMemory, 0 );
    Vec_IntFree( p->vStgDump );
    Vec_PtrFree( p->vStates );
    if ( p->vBugTrace ) Vec_IntFree( p->vBugTrace );
    ABC_FREE( p->pDataSim );
    ABC_FREE( p->pBins );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    [Creates new state.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_ObjEra_t * Gia_ManEraCreateState( Gia_ManEra_t * p )
{
    Gia_ObjEra_t * pNew;
    pNew = (Gia_ObjEra_t *)Mem_FixedEntryFetch( p->pMemory );
    pNew->Num = Vec_PtrSize( p->vStates );
    pNew->iPrev = 0;
    Vec_PtrPush( p->vStates, pNew );
    return pNew;
}


/**Function*************************************************************

  Synopsis    [Computes hash value of the node using its simulation info.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManEraStateHash( unsigned * pState, int nWordsSim, int nTableSize )
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
    for ( i = 0; i < nWordsSim; i++ )
        uHash ^= pState[i] * s_FPrimes[i & 0x7F];
    return uHash % nTableSize;
}

/**Function*************************************************************

  Synopsis    [Returns the place of this state in the table or NULL if it exists.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline unsigned * Gia_ManEraHashFind( Gia_ManEra_t * p, Gia_ObjEra_t * pState, int * pStateNum )
{
    Gia_ObjEra_t * pThis;
    unsigned * pPlace = p->pBins + Gia_ManEraStateHash( pState->pData, p->nWordsDat, p->nBins );
    for ( pThis = (*pPlace)? Gia_ManEraState(p, *pPlace) : NULL; pThis; 
          pPlace = (unsigned *)&pThis->iNext, pThis = (*pPlace)? Gia_ManEraState(p, *pPlace) : NULL )
              if ( !memcmp( pState->pData, pThis->pData, sizeof(unsigned) * p->nWordsDat ) )
              {
                  if ( pStateNum )
                      *pStateNum = pThis->Num;
                  return NULL;
              }
    if ( pStateNum )
      *pStateNum = -1;
    return pPlace;
}

/**Function*************************************************************

  Synopsis    [Resizes the hash table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManEraHashResize( Gia_ManEra_t * p )
{
    Gia_ObjEra_t * pThis;
    unsigned * pBinsOld, * piPlace;
    int nBinsOld, iNext, Counter, i;
    assert( p->pBins != NULL );
    // replace the table
    pBinsOld = p->pBins;
    nBinsOld = p->nBins;
    p->nBins = Abc_PrimeCudd( 3 * p->nBins ); 
    p->pBins = ABC_CALLOC( unsigned, p->nBins );
    // rehash the entries from the old table
    Counter = 0;
    for ( i = 0; i < nBinsOld; i++ )
    for ( pThis = (pBinsOld[i]? Gia_ManEraState(p, pBinsOld[i]) : NULL),
          iNext = (pThis? pThis->iNext : 0);  
          pThis;  pThis = (iNext? Gia_ManEraState(p, iNext) : NULL),   
          iNext = (pThis? pThis->iNext : 0)  )
    {
        assert( pThis->Num );
        pThis->iNext = 0;
        piPlace = Gia_ManEraHashFind( p, pThis, NULL );
        assert( *piPlace == 0 ); // should not be there
        *piPlace = pThis->Num;
        Counter++;
    }
    assert( Counter == Vec_PtrSize( p->vStates ) - 1 );
    ABC_FREE( pBinsOld );
}

/**Function*************************************************************

  Synopsis    [Initialize register output to the given state.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManInsertState( Gia_ManEra_t * p, Gia_ObjEra_t * pState )
{
    Gia_Obj_t * pObj;
    unsigned * pSimInfo;
    int i;
    Gia_ManForEachRo( p->pAig, pObj, i )
    {
        pSimInfo = Gia_ManEraData( p, Gia_ObjId(p->pAig, pObj) );
        if ( Abc_InfoHasBit(pState->pData, i) )
            memset( pSimInfo, 0xff, sizeof(unsigned) * p->nWordsSim );
        else
            memset( pSimInfo, 0, sizeof(unsigned) * p->nWordsSim );
    }
}

/**Function*************************************************************

  Synopsis    [Returns -1 if outputs are not asserted.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Gia_ManOutputAsserted( Gia_ManEra_t * p, Gia_Obj_t * pObj )
{
    unsigned * pInfo  = Gia_ManEraData( p, Gia_ObjId(p->pAig, pObj) );
    int w;
    for ( w = 0; w < p->nWordsSim; w++ )
        if ( pInfo[w] )
            return 32*w + Gia_WordFindFirstBit( pInfo[w] );
    return -1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Gia_ManSimulateCo( Gia_ManEra_t * p, Gia_Obj_t * pObj )
{
    int Id = Gia_ObjId(p->pAig, pObj);
    unsigned * pInfo  = Gia_ManEraData( p, Id );
    unsigned * pInfo0 = Gia_ManEraData( p, Gia_ObjFaninId0(pObj, Id) );
    int w;
    if ( Gia_ObjFaninC0(pObj) )
        for ( w = p->nWordsSim-1; w >= 0; w-- )
            pInfo[w] = ~pInfo0[w];
    else 
        for ( w = p->nWordsSim-1; w >= 0; w-- )
            pInfo[w] = pInfo0[w];
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Gia_ManSimulateNode( Gia_ManEra_t * p, Gia_Obj_t * pObj )
{
    int Id = Gia_ObjId(p->pAig, pObj);
    unsigned * pInfo  = Gia_ManEraData( p, Id );
    unsigned * pInfo0 = Gia_ManEraData( p, Gia_ObjFaninId0(pObj, Id) );
    unsigned * pInfo1 = Gia_ManEraData( p, Gia_ObjFaninId1(pObj, Id) );
    int w;
    if ( Gia_ObjFaninC0(pObj) )
    {
        if (  Gia_ObjFaninC1(pObj) )
            for ( w = p->nWordsSim-1; w >= 0; w-- )
                pInfo[w] = ~(pInfo0[w] | pInfo1[w]);
        else 
            for ( w = p->nWordsSim-1; w >= 0; w-- )
                pInfo[w] = ~pInfo0[w] & pInfo1[w];
    }
    else 
    {
        if (  Gia_ObjFaninC1(pObj) )
            for ( w = p->nWordsSim-1; w >= 0; w-- )
                pInfo[w] = pInfo0[w] & ~pInfo1[w];
        else 
            for ( w = p->nWordsSim-1; w >= 0; w-- )
                pInfo[w] = pInfo0[w] & pInfo1[w];
    }
}

/**Function*************************************************************

  Synopsis    [Performs one iteration of reachability analysis.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManPerformOneIter( Gia_ManEra_t * p )
{
    Gia_Obj_t * pObj;
    int i;
    Gia_ManForEachObj1( p->pAig, pObj, i )
    {
        if ( Gia_ObjIsAnd(pObj) )
            Gia_ManSimulateNode( p, pObj );
        else if ( Gia_ObjIsCo(pObj) )
            Gia_ManSimulateCo( p, pObj );
    }
}

/**Function*************************************************************

  Synopsis    [Performs one iteration of reachability analysis.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Gia_ManCollectBugTrace( Gia_ManEra_t * p, Gia_ObjEra_t * pState, int iCond )
{
    Vec_Int_t * vTrace;
    vTrace = Vec_IntAlloc( 10 );
    Vec_IntPush( vTrace, iCond );
    for ( ; pState; pState = pState->iPrev ? Gia_ManEraState(p, pState->iPrev) : NULL )
        Vec_IntPush( vTrace, pState->Cond );
    Vec_IntReverseOrder( vTrace );
    return vTrace;
}

/**Function*************************************************************

  Synopsis    [Counts the depth of state transitions leading ot this state.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManCountDepth( Gia_ManEra_t * p )
{
    Gia_ObjEra_t * pState;
    int Counter = 0;
    pState = (Gia_ObjEra_t *)Vec_PtrEntryLast( p->vStates );
    if ( pState->iPrev == 0 && Vec_PtrSize(p->vStates) > 3 )
        pState = (Gia_ObjEra_t *)Vec_PtrEntry( p->vStates, Vec_PtrSize(p->vStates) - 2 );
    for ( ; pState; pState = pState->iPrev ? Gia_ManEraState(p, pState->iPrev) : NULL )
        Counter++;
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Analized reached states.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManAnalyzeResult( Gia_ManEra_t * p, Gia_ObjEra_t * pState, int fMiter, int fStgDump )
{
    Gia_Obj_t * pObj;
    unsigned * pSimInfo, * piPlace, uOutput = 0;
    int i, k, iCond, nMints, iNextState;
    // check if the miter is asserted
    if ( fMiter )
    {
        Gia_ManForEachPo( p->pAig, pObj, i )
        {
            iCond = Gia_ManOutputAsserted( p, pObj );
            if ( iCond >= 0 )
            {
                p->vBugTrace = Gia_ManCollectBugTrace( p, pState, iCond );
                return 1;
            }
        }
    }
    // collect reached states 
    nMints = (1 << Gia_ManPiNum(p->pAig));
    for ( k = 0; k < nMints; k++ )
    {
        if ( p->pStateNew == NULL )
            p->pStateNew = Gia_ManEraCreateState( p );
        p->pStateNew->pData[p->nWordsDat-1] = 0;
        Gia_ManForEachRi( p->pAig, pObj, i )
        {
            pSimInfo = Gia_ManEraData( p, Gia_ObjId(p->pAig, pObj) );
            if ( Abc_InfoHasBit(p->pStateNew->pData, i) != Abc_InfoHasBit(pSimInfo, k) )
                Abc_InfoXorBit( p->pStateNew->pData, i );
        }
        if ( fStgDump )
        {
            uOutput = 0;
            Gia_ManForEachPo( p->pAig, pObj, i )
            {
                pSimInfo = Gia_ManEraData( p, Gia_ObjId(p->pAig, pObj) );
                if ( Abc_InfoHasBit(pSimInfo, k) && i < 32 )
                    Abc_InfoXorBit( &uOutput, i );
            }
        }
        piPlace = Gia_ManEraHashFind( p, p->pStateNew, &iNextState );
        if ( fStgDump ) Vec_IntPush( p->vStgDump, k );
        if ( fStgDump ) Vec_IntPush( p->vStgDump, pState->Num );
        if ( piPlace == NULL )
        {
            if ( fStgDump ) Vec_IntPush( p->vStgDump, iNextState );
            if ( fStgDump ) Vec_IntPush( p->vStgDump, uOutput );
            continue;
        }
        if ( fStgDump ) Vec_IntPush( p->vStgDump, p->pStateNew->Num );
        if ( fStgDump ) Vec_IntPush( p->vStgDump, uOutput );
//printf( "Inserting %d ", Vec_PtrSize(p->vStates) );
//Extra_PrintBinary( stdout, p->pStateNew->pData, Gia_ManRegNum(p->pAig) );  printf( "\n" );
        assert( *piPlace == 0 );
        *piPlace = p->pStateNew->Num;
        p->pStateNew->Cond = k;
        p->pStateNew->iPrev = pState->Num;
        p->pStateNew->iNext = 0;
        p->pStateNew = NULL;
        // expand hash table if needed
        if ( Vec_PtrSize(p->vStates) > 2 * p->nBins )
            Gia_ManEraHashResize( p );
    }
    return 0;
}

/**Function*************************************************************

  Synopsis    [Resizes the hash table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManCollectReachable( Gia_Man_t * pAig, int nStatesMax, int fMiter, int fDumpFile, int fVerbose )
{ 
    Gia_ManEra_t * p;
    Gia_ObjEra_t * pState;
    int Hash;
    abctime clk = Abc_Clock();
    int RetValue = 1;
    assert( Gia_ManPiNum(pAig) <= 12 );
    assert( Gia_ManRegNum(pAig) > 0 );
    p = Gia_ManEraCreate( pAig );
    // create init state
    pState = Gia_ManEraCreateState( p );
    pState->Cond  = 0; 
    pState->iPrev = 0;
    pState->iNext = 0; 
    memset( pState->pData, 0, sizeof(unsigned) * p->nWordsDat );
    Hash = Gia_ManEraStateHash(pState->pData, p->nWordsDat, p->nBins);
    p->pBins[ Hash ] = pState->Num;
    // process reachable states
    while ( p->iCurState < Vec_PtrSize( p->vStates ) - 1 )
    {
        if ( Vec_PtrSize(p->vStates) >= nStatesMax )
        {
            printf( "Reached the limit on states traversed (%d).  ", nStatesMax );
            RetValue = -1;
            break;
        }
        pState = Gia_ManEraState( p, ++p->iCurState );
        if ( p->iCurState > 1 && pState->iPrev == 0 )
            continue;
//printf( "Extracting %d  ", p->iCurState );
//Extra_PrintBinary( stdout, p->pStateNew->pData, Gia_ManRegNum(p->pAig) );  printf( "\n" );
        Gia_ManInsertState( p, pState );
        Gia_ManPerformOneIter( p );
        if ( Gia_ManAnalyzeResult( p, pState, fMiter, fDumpFile ) && fMiter )
        {
            RetValue = 0;
            printf( "Miter failed in state %d after %d transitions.  ", 
                p->iCurState, Vec_IntSize(p->vBugTrace)-1 );
            break;
        }
        if ( fVerbose && p->iCurState % 5000 == 0 )
        {
            printf( "States =%10d. Reached =%10d. R = %5.3f. Depth =%6d. Mem =%9.2f MB.  ", 
                p->iCurState, Vec_PtrSize(p->vStates), 1.0*p->iCurState/Vec_PtrSize(p->vStates), Gia_ManCountDepth(p), 
                (1.0/(1<<20))*(1.0*Vec_PtrSize(p->vStates)*(sizeof(Gia_ObjEra_t) + sizeof(unsigned) * p->nWordsDat) + 
                   1.0*p->nBins*sizeof(unsigned) + 1.0*p->vStates->nCap * sizeof(void*)) );
            ABC_PRT( "Time", Abc_Clock() - clk );
        }
    }
    printf( "Reachability analysis traversed %d states with depth %d.  ", p->iCurState-1, Gia_ManCountDepth(p) );
    ABC_PRT( "Time", Abc_Clock() - clk );
    if ( fDumpFile )
    {
        char * pFileName = "test.stg";
        FILE * pFile = fopen( pFileName, "wb" );
        if ( pFile == NULL )
            printf( "Cannot open file \"%s\" for writing.\n", pFileName );
        else
        {
            Gia_ManStgPrint( pFile, p->vStgDump, Gia_ManPiNum(pAig), Gia_ManPoNum(pAig), p->iCurState-1 );
            fclose( pFile );
            printf( "Extracted STG was written into file \"%s\".\n", pFileName );
        }
    }
    Gia_ManEraFree( p );
    return RetValue;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

