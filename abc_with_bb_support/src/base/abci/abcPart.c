/**CFile****************************************************************

  FileName    [abcPart.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Network and node package.]

  Synopsis    [Output partitioning package.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: abcPart.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "base/abc/abc.h"
#include "base/main/main.h"
#include "base/cmd/cmd.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Supp_Man_t_     Supp_Man_t;
struct Supp_Man_t_
{
    int              nChunkSize;    // the size of one chunk of memory (~1 MB)
    int              nStepSize;     // the step size in saving memory (~64 bytes)
    char *           pFreeBuf;      // the pointer to free memory
    int              nFreeSize;     // the size of remaining free memory
    Vec_Ptr_t *      vMemory;       // the memory allocated
    Vec_Ptr_t *      vFree;         // the vector of free pieces of memory
};

typedef struct Supp_One_t_     Supp_One_t;
struct Supp_One_t_
{
    int              nRefs;         // the number of references
    int              nOuts;         // the number of outputs
    int              nOutsAlloc;    // the array size
    int              pOuts[0];      // the array of outputs
};

static inline int    Supp_SizeType( int nSize, int nStepSize )     { return nSize / nStepSize + ((nSize % nStepSize) > 0); }
static inline char * Supp_OneNext( char * pPart )                  { return *((char **)pPart);                             }
static inline void   Supp_OneSetNext( char * pPart, char * pNext ) { *((char **)pPart) = pNext;                            }

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Start the memory manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Supp_Man_t * Supp_ManStart( int nChunkSize, int nStepSize )
{
    Supp_Man_t * p;
    p = ABC_ALLOC( Supp_Man_t, 1 );
    memset( p, 0, sizeof(Supp_Man_t) );
    p->nChunkSize = nChunkSize;
    p->nStepSize  = nStepSize;
    p->vMemory    = Vec_PtrAlloc( 1000 );
    p->vFree      = Vec_PtrAlloc( 1000 );
    return p;
}

/**Function*************************************************************

  Synopsis    [Stops the memory manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Supp_ManStop( Supp_Man_t * p )
{
    void * pMemory;
    int i;
    Vec_PtrForEachEntry( void *, p->vMemory, pMemory, i )
        ABC_FREE( pMemory );
    Vec_PtrFree( p->vMemory );
    Vec_PtrFree( p->vFree );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    [Fetches the memory entry of the given size.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Supp_ManFetch( Supp_Man_t * p, int nSize )
{
    int Type, nSizeReal;
    char * pMemory;
    assert( nSize > 0 );
    Type = Supp_SizeType( nSize, p->nStepSize );
    Vec_PtrFillExtra( p->vFree, Type + 1, NULL );
    if ( (pMemory = (char *)Vec_PtrEntry( p->vFree, Type )) )
    {
        Vec_PtrWriteEntry( p->vFree, Type, Supp_OneNext(pMemory) );
        return pMemory;
    }
    nSizeReal = p->nStepSize * Type;
    if ( p->nFreeSize < nSizeReal )
    {
        p->pFreeBuf = ABC_ALLOC( char, p->nChunkSize );
        p->nFreeSize = p->nChunkSize;
        Vec_PtrPush( p->vMemory, p->pFreeBuf );
    }
    assert( p->nFreeSize >= nSizeReal );
    pMemory = p->pFreeBuf;
    p->pFreeBuf  += nSizeReal;
    p->nFreeSize -= nSizeReal;
    return pMemory;
}

/**Function*************************************************************

  Synopsis    [Recycles the memory entry of the given size.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Supp_ManRecycle( Supp_Man_t * p, char * pMemory, int nSize )
{
    int Type;
    Type = Supp_SizeType( nSize, p->nStepSize );
    Vec_PtrFillExtra( p->vFree, Type + 1, NULL );
    Supp_OneSetNext( pMemory, (char *)Vec_PtrEntry(p->vFree, Type) );
    Vec_PtrWriteEntry( p->vFree, Type, pMemory );
}

/**Function*************************************************************

  Synopsis    [Fetches the memory entry of the given size.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Supp_One_t * Supp_ManFetchEntry( Supp_Man_t * p, int nWords, int nRefs )
{
    Supp_One_t * pPart;
    pPart = (Supp_One_t *)Supp_ManFetch( p, sizeof(Supp_One_t) + sizeof(int) * nWords );
    pPart->nRefs = nRefs;
    pPart->nOuts = 0;
    pPart->nOutsAlloc = nWords;
    return pPart;
}

/**Function*************************************************************

  Synopsis    [Recycles the memory entry of the given size.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Supp_ManRecycleEntry( Supp_Man_t * p, Supp_One_t * pEntry )
{
    assert( pEntry->nOuts <= pEntry->nOutsAlloc );
    assert( pEntry->nOuts >= pEntry->nOutsAlloc/2 );
    Supp_ManRecycle( p, (char *)pEntry, sizeof(Supp_One_t) + sizeof(int) * pEntry->nOutsAlloc );
}

/**Function*************************************************************

  Synopsis    [Merges two entries.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Supp_One_t * Supp_ManMergeEntry( Supp_Man_t * pMan, Supp_One_t * p1, Supp_One_t * p2, int nRefs )
{
    Supp_One_t * p = Supp_ManFetchEntry( pMan, p1->nOuts + p2->nOuts, nRefs );
    int * pBeg1 = p1->pOuts;
    int * pBeg2 = p2->pOuts;
    int * pBeg  = p->pOuts;
    int * pEnd1 = p1->pOuts + p1->nOuts;
    int * pEnd2 = p2->pOuts + p2->nOuts;
    while ( pBeg1 < pEnd1 && pBeg2 < pEnd2 )
    {
        if ( *pBeg1 == *pBeg2 )
            *pBeg++ = *pBeg1++, pBeg2++;
        else if ( *pBeg1 < *pBeg2 )
            *pBeg++ = *pBeg1++;
        else 
            *pBeg++ = *pBeg2++;
    }
    while ( pBeg1 < pEnd1 )
        *pBeg++ = *pBeg1++;
    while ( pBeg2 < pEnd2 )
        *pBeg++ = *pBeg2++;
    p->nOuts = pBeg - p->pOuts;
    assert( p->nOuts <= p->nOutsAlloc );
    assert( p->nOuts >= p1->nOuts );
    assert( p->nOuts >= p2->nOuts );
    return p;
}

/**Function*************************************************************

  Synopsis    [Tranfers the entry.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Supp_ManTransferEntry( Supp_One_t * p )
{
    Vec_Int_t * vSupp;
    int i;
    vSupp = Vec_IntAlloc( p->nOuts );
    for ( i = 0; i < p->nOuts; i++ )
        Vec_IntPush( vSupp, p->pOuts[i] );
    return vSupp;
}

/**Function*************************************************************

  Synopsis    [Computes supports of the POs in the multi-output AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Abc_NtkDfsNatural( Abc_Ntk_t * pNtk )
{
    Vec_Ptr_t * vNodes;
    Abc_Obj_t * pObj, * pNext;
    int i, k;
    assert( Abc_NtkIsStrash(pNtk) );
    vNodes = Vec_PtrAlloc( Abc_NtkObjNum(pNtk) );
    Abc_NtkIncrementTravId( pNtk );
    // add the constant-1 nodes
    pObj = Abc_AigConst1(pNtk);
    Abc_NodeSetTravIdCurrent( pObj );
    Vec_PtrPush( vNodes, pObj );
    // add the CIs/nodes/COs in the topological order
    Abc_NtkForEachNode( pNtk, pObj, i )
    {
        // check the fanins and add CIs
        Abc_ObjForEachFanin( pObj, pNext, k )
            if ( Abc_ObjIsCi(pNext) && !Abc_NodeIsTravIdCurrent(pNext) )
            {
                Abc_NodeSetTravIdCurrent( pNext );
                Vec_PtrPush( vNodes, pNext );
            }
        // add the node
        Vec_PtrPush( vNodes, pObj );
        // check the fanouts and add COs
        Abc_ObjForEachFanout( pObj, pNext, k )
            if ( Abc_ObjIsCo(pNext) && !Abc_NodeIsTravIdCurrent(pNext) )
            {
                Abc_NodeSetTravIdCurrent( pNext );
                Vec_PtrPush( vNodes, pNext );
            }
    }
    return vNodes;
}

/**Function*************************************************************

  Synopsis    [Computes supports of the POs.]

  Description [Returns the ptr-vector of int-vectors.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Abc_NtkComputeSupportsSmart( Abc_Ntk_t * pNtk )
{
    Vec_Ptr_t * vSupports;
    Vec_Ptr_t * vNodes;
    Vec_Int_t * vSupp;
    Supp_Man_t * p;
    Supp_One_t * pPart0, * pPart1;
    Abc_Obj_t * pObj;
    int i;
    // set the number of PIs/POs
    Abc_NtkForEachCi( pNtk, pObj, i )
        pObj->pNext = (Abc_Obj_t *)(ABC_PTRINT_T)i;
    Abc_NtkForEachCo( pNtk, pObj, i )
        pObj->pNext = (Abc_Obj_t *)(ABC_PTRINT_T)i;
    // start the support computation manager
    p = Supp_ManStart( 1 << 20, 1 << 6 );
    // consider objects in the topological order
    vSupports = Vec_PtrAlloc( Abc_NtkCoNum(pNtk) );
    Abc_NtkCleanCopy(pNtk);
    // order the nodes so that the PIs and POs follow naturally
    vNodes = Abc_NtkDfsNatural( pNtk );
    Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pObj, i )
    {
        if ( Abc_ObjIsNode(pObj) )
        {
            pPart0 = (Supp_One_t *)Abc_ObjFanin0(pObj)->pCopy;
            pPart1 = (Supp_One_t *)Abc_ObjFanin1(pObj)->pCopy;
            pObj->pCopy = (Abc_Obj_t *)Supp_ManMergeEntry( p, pPart0, pPart1, Abc_ObjFanoutNum(pObj) );
            assert( pPart0->nRefs > 0 );
            if ( --pPart0->nRefs == 0 )
                Supp_ManRecycleEntry( p, pPart0 );
            assert( pPart1->nRefs > 0 );
            if ( --pPart1->nRefs == 0 )
                Supp_ManRecycleEntry( p, pPart1 );
            continue;
        }
        if ( Abc_ObjIsCo(pObj) )
        {
            pPart0 = (Supp_One_t *)Abc_ObjFanin0(pObj)->pCopy;
            // only save the CO if it is non-trivial
            if ( Abc_ObjIsNode(Abc_ObjFanin0(pObj)) )
            {
                vSupp = Supp_ManTransferEntry(pPart0);
                Vec_IntPush( vSupp, (int)(ABC_PTRINT_T)pObj->pNext );
                Vec_PtrPush( vSupports, vSupp );
            }
            assert( pPart0->nRefs > 0 );
            if ( --pPart0->nRefs == 0 )
                Supp_ManRecycleEntry( p, pPart0 );
            continue;
        }
        if ( Abc_ObjIsCi(pObj) )
        {
            if ( Abc_ObjFanoutNum(pObj) )
            {
                pPart0 = (Supp_One_t *)Supp_ManFetchEntry( p, 1, Abc_ObjFanoutNum(pObj) );
                pPart0->pOuts[ pPart0->nOuts++ ] = (int)(ABC_PTRINT_T)pObj->pNext;
                pObj->pCopy = (Abc_Obj_t *)pPart0;
            }
            continue;
        }
        if ( pObj == Abc_AigConst1(pNtk) )
        {
            if ( Abc_ObjFanoutNum(pObj) )
                pObj->pCopy = (Abc_Obj_t *)Supp_ManFetchEntry( p, 0, Abc_ObjFanoutNum(pObj) );
            continue;
        }
        assert( 0 );
    }
    Vec_PtrFree( vNodes );
//printf( "Memory usage = %d MB.\n", Vec_PtrSize(p->vMemory) * p->nChunkSize / (1<<20) );
    Supp_ManStop( p );
    // sort supports by size
    Vec_VecSort( (Vec_Vec_t *)vSupports, 1 );
    // clear the number of PIs/POs
    Abc_NtkForEachCi( pNtk, pObj, i )
        pObj->pNext = NULL;
    Abc_NtkForEachCo( pNtk, pObj, i )
        pObj->pNext = NULL;
/*
    Vec_PtrForEachEntry( Vec_Int_t *, vSupports, vSupp, i )
        printf( "%d ", Vec_IntSize(vSupp) );
    printf( "\n" );
*/
    return vSupports;
}


/**Function*************************************************************

  Synopsis    [Computes supports of the POs using naive method.]

  Description [Returns the ptr-vector of int-vectors.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Abc_NtkComputeSupportsNaive( Abc_Ntk_t * pNtk )
{
    Vec_Ptr_t * vSupp, * vSupports;
    Vec_Int_t * vSuppI;
    Abc_Obj_t * pObj, * pTemp;
    int i, k;
    // set the PI numbers
    Abc_NtkForEachCi( pNtk, pObj, i )
        pObj->pNext = (Abc_Obj_t *)(ABC_PTRINT_T)i;
    // save the CI numbers
    vSupports = Vec_PtrAlloc( Abc_NtkCoNum(pNtk) );
    Abc_NtkForEachCo( pNtk, pObj, i )
    {
        if ( !Abc_ObjIsNode(Abc_ObjFanin0(pObj)) )
            continue;
        vSupp = Abc_NtkNodeSupport( pNtk, &pObj, 1 );
        vSuppI = (Vec_Int_t *)vSupp;
        Vec_PtrForEachEntry( Abc_Obj_t *, vSupp, pTemp, k )
            Vec_IntWriteEntry( vSuppI, k, (int)(ABC_PTRINT_T)pTemp->pNext );
        Vec_IntSort( vSuppI, 0 );
        // append the number of this output
        Vec_IntPush( vSuppI, i );
        // save the support in the vector
        Vec_PtrPush( vSupports, vSuppI );
    }
    // clean the CI numbers
    Abc_NtkForEachCi( pNtk, pObj, i )
        pObj->pNext = NULL;
    // sort supports by size
    Vec_VecSort( (Vec_Vec_t *)vSupports, 1 );
/*
    Vec_PtrForEachEntry( Vec_Int_t *, vSupports, vSuppI, i )
        printf( "%d ", Vec_IntSize(vSuppI) );
    printf( "\n" );
*/
    return vSupports;
}

/**Function*************************************************************

  Synopsis    [Start bitwise support representation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned * Abc_NtkSuppCharStart( Vec_Int_t * vOne, int nPis )
{
    unsigned * pBuffer;
    int i, Entry;
    int nWords = Abc_BitWordNum(nPis);
    pBuffer = ABC_ALLOC( unsigned, nWords );
    memset( pBuffer, 0, sizeof(unsigned) * nWords );
    Vec_IntForEachEntry( vOne, Entry, i )
    {
        assert( Entry < nPis );
        Abc_InfoSetBit( pBuffer, Entry );
    }
    return pBuffer;
}

/**Function*************************************************************

  Synopsis    [Add to bitwise support representation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkSuppCharAdd( unsigned * pBuffer, Vec_Int_t * vOne, int nPis )
{
    int i, Entry;
    Vec_IntForEachEntry( vOne, Entry, i )
    {
        assert( Entry < nPis );
        Abc_InfoSetBit( pBuffer, Entry );
    }
}

/**Function*************************************************************

  Synopsis    [Find the common variables using bitwise support representation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkSuppCharCommon( unsigned * pBuffer, Vec_Int_t * vOne )
{
    int i, Entry, nCommon = 0;
    Vec_IntForEachEntry( vOne, Entry, i )
        nCommon += Abc_InfoHasBit(pBuffer, Entry);
    return nCommon;
}

/**Function*************************************************************

  Synopsis    [Find the best partition.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkPartitionSmartFindPart( Vec_Ptr_t * vPartSuppsAll, Vec_Ptr_t * vPartsAll, Vec_Ptr_t * vPartSuppsChar, int nSuppSizeLimit, Vec_Int_t * vOne )
{
/*
    Vec_Int_t * vPartSupp, * vPart;
    double Attract, Repulse, Cost, CostBest;
    int i, nCommon, iBest;
    iBest = -1;
    CostBest = 0.0;
    Vec_PtrForEachEntry( Vec_Int_t *, vPartSuppsAll, vPartSupp, i )
    {
        vPart = Vec_PtrEntry( vPartsAll, i );
        if ( nPartSizeLimit > 0 && Vec_IntSize(vPart) >= nPartSizeLimit )
            continue;
        nCommon = Vec_IntTwoCountCommon( vPartSupp, vOne );
        if ( nCommon == 0 )
            continue;
        if ( nCommon == Vec_IntSize(vOne) )
            return i;
        Attract = 1.0 * nCommon / Vec_IntSize(vOne);
        if ( Vec_IntSize(vPartSupp) < 100 )
            Repulse = 1.0;
        else
            Repulse = log10( Vec_IntSize(vPartSupp) / 10.0 );
        Cost = pow( Attract, pow(Repulse, 5.0) );
        if ( CostBest < Cost )
        {
            CostBest = Cost;
            iBest = i;
        }
    }
    if ( CostBest < 0.6 )
        return -1;
    return iBest;
*/

    Vec_Int_t * vPartSupp;//, * vPart;
    int Attract, Repulse, Value, ValueBest;
    int i, nCommon, iBest;
//    int nCommon2;
    iBest = -1;
    ValueBest = 0;
    Vec_PtrForEachEntry( Vec_Int_t *, vPartSuppsAll, vPartSupp, i )
    {
        // skip partitions with too many outputs
//        vPart = Vec_PtrEntry( vPartsAll, i );
//        if ( nSuppSizeLimit > 0 && Vec_IntSize(vPart) >= nSuppSizeLimit )
//            continue;
        // find the number of common variables between this output and the partitions
//        nCommon2 = Vec_IntTwoCountCommon( vPartSupp, vOne );
        nCommon = Abc_NtkSuppCharCommon( (unsigned *)Vec_PtrEntry(vPartSuppsChar, i), vOne );
//        assert( nCommon2 == nCommon );
        // if no common variables, continue searching
        if ( nCommon == 0 )
            continue;
        // if all variables are common, the best partition if found
        if ( nCommon == Vec_IntSize(vOne) )
            return i;
        // skip partitions whose size exceeds the limit
        if ( nSuppSizeLimit > 0 && Vec_IntSize(vPartSupp) >= 2 * nSuppSizeLimit )
            continue;
        // figure out might be the good partition for this one
        Attract = 1000 * nCommon / Vec_IntSize(vOne);
        if ( Vec_IntSize(vPartSupp) < 100 )
            Repulse = 1;
        else
            Repulse = 1+Abc_Base2Log(Vec_IntSize(vPartSupp)-100);
        Value = Attract/Repulse;
        if ( ValueBest < Value )
        {
            ValueBest = Value;
            iBest = i;
        }
    }
    if ( ValueBest < 75 )
        return -1;
    return iBest;
}

/**Function*************************************************************

  Synopsis    [Perform the smart partitioning.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkPartitionPrint( Abc_Ntk_t * pNtk, Vec_Ptr_t * vPartsAll, Vec_Ptr_t * vPartSuppsAll )
{
    Vec_Int_t * vOne;
    int i, nOutputs, Counter;

    Counter = 0;
    Vec_PtrForEachEntry( Vec_Int_t *, vPartSuppsAll, vOne, i )
    {
        nOutputs = Vec_IntSize((Vec_Int_t *)Vec_PtrEntry(vPartsAll, i));
        printf( "%d=(%d,%d) ", i, Vec_IntSize(vOne), nOutputs );
        Counter += nOutputs;
        if ( i == Vec_PtrSize(vPartsAll) - 1 )
            break;
    }
//    assert( Counter == Abc_NtkCoNum(pNtk) );
    printf( "\nTotal = %d. Outputs = %d.\n", Counter, Abc_NtkCoNum(pNtk) );
}

/**Function*************************************************************

  Synopsis    [Perform the smart partitioning.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkPartitionCompact( Vec_Ptr_t * vPartsAll, Vec_Ptr_t * vPartSuppsAll, int nSuppSizeLimit )
{
    Vec_Int_t * vOne, * vPart, * vPartSupp, * vTemp;
    int i, iPart;

    if ( nSuppSizeLimit == 0 )
        nSuppSizeLimit = 200;

    // pack smaller partitions into larger blocks
    iPart = 0;
    vPart = vPartSupp = NULL;
    Vec_PtrForEachEntry( Vec_Int_t *, vPartSuppsAll, vOne, i )
    {
        if ( Vec_IntSize(vOne) < nSuppSizeLimit )
        {
            if ( vPartSupp == NULL )
            {
                assert( vPart == NULL );
                vPartSupp = Vec_IntDup(vOne);
                vPart = (Vec_Int_t *)Vec_PtrEntry(vPartsAll, i);
            }
            else
            {
                vPartSupp = Vec_IntTwoMerge( vTemp = vPartSupp, vOne );
                Vec_IntFree( vTemp );
                vPart = Vec_IntTwoMerge( vTemp = vPart, (Vec_Int_t *)Vec_PtrEntry(vPartsAll, i) );
                Vec_IntFree( vTemp );
                Vec_IntFree( (Vec_Int_t *)Vec_PtrEntry(vPartsAll, i) );
            }
            if ( Vec_IntSize(vPartSupp) < nSuppSizeLimit )
                continue;
        }
        else
            vPart = (Vec_Int_t *)Vec_PtrEntry(vPartsAll, i);
        // add the partition 
        Vec_PtrWriteEntry( vPartsAll, iPart, vPart );  
        vPart = NULL;
        if ( vPartSupp ) 
        {
            Vec_IntFree( (Vec_Int_t *)Vec_PtrEntry(vPartSuppsAll, iPart) );
            Vec_PtrWriteEntry( vPartSuppsAll, iPart, vPartSupp );  
            vPartSupp = NULL;
        }
        iPart++;
    }
    // add the last one
    if ( vPart )
    {
        Vec_PtrWriteEntry( vPartsAll, iPart, vPart );  
        vPart = NULL;

        assert( vPartSupp != NULL );
        Vec_IntFree( (Vec_Int_t *)Vec_PtrEntry(vPartSuppsAll, iPart) );
        Vec_PtrWriteEntry( vPartSuppsAll, iPart, vPartSupp );  
        vPartSupp = NULL;
        iPart++;
    }
    Vec_PtrShrink( vPartsAll, iPart );
    Vec_PtrShrink( vPartsAll, iPart );
}

/**Function*************************************************************

  Synopsis    [Perform the smart partitioning.]

  Description [Returns the ptr-vector of int-vectors.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Abc_NtkPartitionSmart( Abc_Ntk_t * pNtk, int nSuppSizeLimit, int fVerbose )
{
    ProgressBar * pProgress;
    Vec_Ptr_t * vPartSuppsChar;
    Vec_Ptr_t * vSupps, * vPartsAll, * vPartsAll2, * vPartSuppsAll;
    Vec_Int_t * vOne, * vPart, * vPartSupp, * vTemp;
    int i, iPart, iOut, timeFind = 0;
    abctime clk, clk2;

    // compute the supports for all outputs
clk = Abc_Clock();
//    vSupps = Abc_NtkComputeSupportsNaive( pNtk );
    vSupps = Abc_NtkComputeSupportsSmart( pNtk );
if ( fVerbose )
{
ABC_PRT( "Supps", Abc_Clock() - clk );
}
    // start char-based support representation
    vPartSuppsChar = Vec_PtrAlloc( 1000 );

    // create partitions
clk = Abc_Clock();
    vPartsAll = Vec_PtrAlloc( 256 );
    vPartSuppsAll = Vec_PtrAlloc( 256 );
    pProgress = Extra_ProgressBarStart( stdout, Vec_PtrSize(vSupps) );
    Vec_PtrForEachEntry( Vec_Int_t *, vSupps, vOne, i )
    {
        Extra_ProgressBarUpdate( pProgress, i, NULL );
//        if ( i % 1000 == 0 )
//            printf( "CIs = %6d. COs = %6d. Processed = %6d (out of %6d). Parts = %6d.\r", 
//                Abc_NtkCiNum(pNtk), Abc_NtkCoNum(pNtk), i, Vec_PtrSize(vSupps), Vec_PtrSize(vPartsAll) ); 
        // get the output number
        iOut = Vec_IntPop(vOne);
        // find closely matching part
clk2 = Abc_Clock();
        iPart = Abc_NtkPartitionSmartFindPart( vPartSuppsAll, vPartsAll, vPartSuppsChar, nSuppSizeLimit, vOne );
timeFind += Abc_Clock() - clk2;
        if ( iPart == -1 )
        {
            // create new partition
            vPart = Vec_IntAlloc( 32 );
            Vec_IntPush( vPart, iOut );
            // create new partition support
            vPartSupp = Vec_IntDup( vOne );
            // add this partition and its support
            Vec_PtrPush( vPartsAll, vPart );
            Vec_PtrPush( vPartSuppsAll, vPartSupp );

            Vec_PtrPush( vPartSuppsChar, Abc_NtkSuppCharStart(vOne, Abc_NtkCiNum(pNtk)) );
        }
        else
        {
            // add output to this partition
            vPart = (Vec_Int_t *)Vec_PtrEntry( vPartsAll, iPart );
            Vec_IntPush( vPart, iOut );
            // merge supports
            vPartSupp = (Vec_Int_t *)Vec_PtrEntry( vPartSuppsAll, iPart );
            vPartSupp = Vec_IntTwoMerge( vTemp = vPartSupp, vOne );
            Vec_IntFree( vTemp );
            // reinsert new support
            Vec_PtrWriteEntry( vPartSuppsAll, iPart, vPartSupp );

            Abc_NtkSuppCharAdd( (unsigned *)Vec_PtrEntry(vPartSuppsChar, iPart), vOne, Abc_NtkCiNum(pNtk) );
        }
    }
    Extra_ProgressBarStop( pProgress );

    // stop char-based support representation
    Vec_PtrForEachEntry( Vec_Int_t *, vPartSuppsChar, vTemp, i )
        ABC_FREE( vTemp );
    Vec_PtrFree( vPartSuppsChar );

//printf( "\n" );
if ( fVerbose )
{
ABC_PRT( "Parts", Abc_Clock() - clk );
//ABC_PRT( "Find ", timeFind );
}

clk = Abc_Clock();
    // remember number of supports
    Vec_PtrForEachEntry( Vec_Int_t *, vPartSuppsAll, vOne, i )
        Vec_IntPush( vOne, i );
    // sort the supports in the decreasing order
    Vec_VecSort( (Vec_Vec_t *)vPartSuppsAll, 1 );
    // reproduce partitions
    vPartsAll2 = Vec_PtrAlloc( 256 );
    Vec_PtrForEachEntry( Vec_Int_t *, vPartSuppsAll, vOne, i )
        Vec_PtrPush( vPartsAll2, Vec_PtrEntry(vPartsAll, Vec_IntPop(vOne)) );
    Vec_PtrFree( vPartsAll );
    vPartsAll = vPartsAll2;

    // compact small partitions
//    Abc_NtkPartitionPrint( pNtk, vPartsAll, vPartSuppsAll );
    Abc_NtkPartitionCompact( vPartsAll, vPartSuppsAll, nSuppSizeLimit );

if ( fVerbose )
{
ABC_PRT( "Comps", Abc_Clock() - clk );
}
    if ( fVerbose )
    printf( "Created %d partitions.\n", Vec_PtrSize(vPartsAll) );
//    Abc_NtkPartitionPrint( pNtk, vPartsAll, vPartSuppsAll );

    // cleanup
    Vec_VecFree( (Vec_Vec_t *)vSupps );
    Vec_VecFree( (Vec_Vec_t *)vPartSuppsAll );
/*
    // converts from intergers to nodes
    Vec_PtrForEachEntry( Vec_Int_t *, vPartsAll, vPart, iPart )
    {
        vPartPtr = Vec_PtrAlloc( Vec_IntSize(vPart) );
        Vec_IntForEachEntry( vPart, iOut, i )
            Vec_PtrPush( vPartPtr, Abc_NtkCo(pNtk, iOut) );
        Vec_IntFree( vPart );
        Vec_PtrWriteEntry( vPartsAll, iPart, vPartPtr );
    }
*/
    return vPartsAll;
}

/**Function*************************************************************

  Synopsis    [Perform the naive partitioning.]

  Description [Returns the ptr-vector of int-vectors.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Abc_NtkPartitionNaive( Abc_Ntk_t * pNtk, int nPartSize )
{
    Vec_Ptr_t * vParts;
    Abc_Obj_t * pObj;
    int nParts, i;
    nParts = (Abc_NtkCoNum(pNtk) / nPartSize) + ((Abc_NtkCoNum(pNtk) % nPartSize) > 0);
    vParts = (Vec_Ptr_t *)Vec_VecStart( nParts );
    Abc_NtkForEachCo( pNtk, pObj, i )
        Vec_IntPush( (Vec_Int_t *)Vec_PtrEntry(vParts, i / nPartSize), i );
    return vParts;
}

/**Function*************************************************************

  Synopsis    [Converts from intergers to pointers for the given network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkConvertCos( Abc_Ntk_t * pNtk, Vec_Int_t * vOuts, Vec_Ptr_t * vOutsPtr )
{
    int Out, i;
    Vec_PtrClear( vOutsPtr );
    Vec_IntForEachEntry( vOuts, Out, i )
        Vec_PtrPush( vOutsPtr, Abc_NtkCo(pNtk, Out) );
}

/**Function*************************************************************

  Synopsis    [Returns representative of the given node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Obj_t * Abc_NtkPartStitchFindRepr_rec( Vec_Ptr_t * vEquiv, Abc_Obj_t * pObj )
{
    Abc_Obj_t * pRepr;
    pRepr = (Abc_Obj_t *)Vec_PtrEntry( vEquiv, pObj->Id );
    if ( pRepr == NULL || pRepr == pObj )
        return pObj;
    return Abc_NtkPartStitchFindRepr_rec( vEquiv, pRepr );
}

/**Function*************************************************************

  Synopsis    [Returns the representative of the fanin.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Abc_Obj_t * Abc_NtkPartStitchCopy0( Vec_Ptr_t * vEquiv, Abc_Obj_t * pObj )
{
    Abc_Obj_t * pFan = Abc_ObjFanin0( pObj );
    Abc_Obj_t * pRepr = Abc_NtkPartStitchFindRepr_rec( vEquiv, pFan );
    return Abc_ObjNotCond( pRepr->pCopy, pRepr->fPhase ^ pFan->fPhase ^ (int)Abc_ObjFaninC1(pObj) );
}
static inline Abc_Obj_t * Abc_NtkPartStitchCopy1( Vec_Ptr_t * vEquiv, Abc_Obj_t * pObj )
{
    Abc_Obj_t * pFan = Abc_ObjFanin1( pObj );
    Abc_Obj_t * pRepr = Abc_NtkPartStitchFindRepr_rec( vEquiv, pFan );
    return Abc_ObjNotCond( pRepr->pCopy, pRepr->fPhase ^ pFan->fPhase ^ (int)Abc_ObjFaninC1(pObj) );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Hop_Obj_t * Hop_ObjChild0Next( Abc_Obj_t * pObj ) { return Hop_NotCond( (Hop_Obj_t *)Abc_ObjFanin0(pObj)->pNext, Abc_ObjFaninC0(pObj) ); }
static inline Hop_Obj_t * Hop_ObjChild1Next( Abc_Obj_t * pObj ) { return Hop_NotCond( (Hop_Obj_t *)Abc_ObjFanin1(pObj)->pNext, Abc_ObjFaninC1(pObj) ); }


/**Function*************************************************************

  Synopsis    [Stitches together several networks with choice nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Hop_Man_t * Abc_NtkPartStartHop( Abc_Ntk_t * pNtk )
{
    Hop_Man_t * pMan;
    Abc_Obj_t * pObj;
    int i;
    // start the HOP package
    pMan = Hop_ManStart();
    pMan->vObjs = Vec_PtrAlloc( Abc_NtkObjNumMax(pNtk) + 1  );
    Vec_PtrPush( pMan->vObjs, Hop_ManConst1(pMan) );
    // map constant node and PIs
    Abc_AigConst1(pNtk)->pNext = (Abc_Obj_t *)Hop_ManConst1(pMan);
    Abc_NtkForEachCi( pNtk, pObj, i )
        pObj->pNext = (Abc_Obj_t *)Hop_ObjCreatePi(pMan);
    // map the internal nodes
    Abc_AigForEachAnd( pNtk, pObj, i )
    {
        pObj->pNext = (Abc_Obj_t *)Hop_And( pMan, Hop_ObjChild0Next(pObj), Hop_ObjChild1Next(pObj) );
        assert( !Abc_ObjIsComplement(pObj->pNext) );
    }
    // set the choice nodes
    Abc_AigForEachAnd( pNtk, pObj, i )
    {
        if ( pObj->pCopy )
            ((Hop_Obj_t *)pObj->pNext)->pData = pObj->pCopy->pNext;
    }
    // transfer the POs
    Abc_NtkForEachCo( pNtk, pObj, i )
        Hop_ObjCreatePo( pMan, Hop_ObjChild0Next(pObj) );
    // check the new manager
    if ( !Hop_ManCheck(pMan) )
        printf( "Abc_NtkPartStartHop: HOP manager check has failed.\n" );
    return pMan;
}

/**Function*************************************************************

  Synopsis    [Stitches together several networks with choice nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkPartStitchChoices( Abc_Ntk_t * pNtk, Vec_Ptr_t * vParts )
{
    extern Abc_Ntk_t * Abc_NtkHopRemoveLoops( Abc_Ntk_t * pNtk, Hop_Man_t * pMan );
    Hop_Man_t * pMan;
    Vec_Ptr_t * vNodes;
    Abc_Ntk_t * pNtkNew, * pNtkTemp;
    Abc_Obj_t * pObj, * pFanin;
    int i, k, iNodeId;

    // start a new network similar to the original one
    assert( Abc_NtkIsStrash(pNtk) );
    pNtkNew = Abc_NtkStartFrom( pNtk, ABC_NTK_STRASH, ABC_FUNC_AIG );

    // annotate parts to point to the new network
    Vec_PtrForEachEntry( Abc_Ntk_t *, vParts, pNtkTemp, i )
    {
        assert( Abc_NtkIsStrash(pNtkTemp) );
        Abc_NtkCleanCopy( pNtkTemp );

        // map the CI nodes
        Abc_AigConst1(pNtkTemp)->pCopy = Abc_AigConst1(pNtkNew);
        Abc_NtkForEachCi( pNtkTemp, pObj, k )
        {
            iNodeId = Nm_ManFindIdByNameTwoTypes( pNtkNew->pManName, Abc_ObjName(pObj), ABC_OBJ_PI, ABC_OBJ_BO );
            if ( iNodeId == -1 )
            {
                printf( "Cannot find CI node %s in the original network.\n", Abc_ObjName(pObj) );
                return NULL;
            }
            pObj->pCopy = Abc_NtkObj( pNtkNew, iNodeId );
        }

        // add the internal nodes while saving representatives
        vNodes = Abc_AigDfs( pNtkTemp, 1, 0 );
        Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pObj, k )
        {
            pObj->pCopy = Abc_AigAnd( (Abc_Aig_t *)pNtkNew->pManFunc, Abc_ObjChild0Copy(pObj), Abc_ObjChild1Copy(pObj) );
            assert( !Abc_ObjIsComplement(pObj->pCopy) );
            if ( Abc_AigNodeIsChoice(pObj) )
                for ( pFanin = (Abc_Obj_t *)pObj->pData; pFanin; pFanin = (Abc_Obj_t *)pFanin->pData )
                    pFanin->pCopy->pCopy = pObj->pCopy;
        }
        Vec_PtrFree( vNodes );

        // map the CO nodes
        Abc_NtkForEachCo( pNtkTemp, pObj, k )
        {
            iNodeId = Nm_ManFindIdByNameTwoTypes( pNtkNew->pManName, Abc_ObjName(pObj), ABC_OBJ_PO, ABC_OBJ_BI );
            if ( iNodeId == -1 )
            {
                printf( "Cannot find CO node %s in the original network.\n", Abc_ObjName(pObj) );
                return NULL;
            }
            pObj->pCopy = Abc_NtkObj( pNtkNew, iNodeId );
            Abc_ObjAddFanin( pObj->pCopy, Abc_ObjChild0Copy(pObj) );
        }
    }

    // connect the remaining POs
/*
    Abc_AigConst1(pNtk)->pCopy = Abc_AigConst1(pNtkNew);
    Abc_NtkForEachCi( pNtk, pObj, i )
        pObj->pCopy = Abc_NtkCi( pNtkNew, i );
    Abc_NtkForEachCo( pNtk, pObj, i )
        pObj->pCopy = Abc_NtkCo( pNtkNew, i );
*/
    Abc_NtkForEachCo( pNtk, pObj, i )
    {
        if ( Abc_ObjFaninNum(pObj->pCopy) == 0 )
            Abc_ObjAddFanin( pObj->pCopy, Abc_ObjChild0Copy(pObj) );
    }

    // transform into the HOP manager
    pMan = Abc_NtkPartStartHop( pNtkNew );
    pNtkNew = Abc_NtkHopRemoveLoops( pNtkTemp = pNtkNew, pMan );
    Abc_NtkDelete( pNtkTemp );

    // check correctness of the new network
    if ( !Abc_NtkCheck( pNtkNew ) )
    {
        printf( "Abc_NtkPartStitchChoices: The network check has failed.\n" );
        Abc_NtkDelete( pNtkNew );
        return NULL;
    }
    return pNtkNew;
}

/**Function*************************************************************

  Synopsis    [Stitches together several networks with choice nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkFraigPartitioned( Vec_Ptr_t * vStore, void * pParams )
{
    Vec_Ptr_t * vParts, * vFraigs, * vOnePtr;
    Vec_Int_t * vOne;
    Abc_Ntk_t * pNtk, * pNtk2, * pNtkAig, * pNtkFraig;
    int i, k;

    // perform partitioning
    pNtk = (Abc_Ntk_t *)Vec_PtrEntry( vStore, 0 );
    assert( Abc_NtkIsStrash(pNtk) );
//    vParts = Abc_NtkPartitionNaive( pNtk, 20 );
    vParts = Abc_NtkPartitionSmart( pNtk, 300, 0 );

    Cmd_CommandExecute( Abc_FrameGetGlobalFrame(), "unset progressbar" );

    // fraig each partition
    vOnePtr = Vec_PtrAlloc( 1000 );
    vFraigs = Vec_PtrAlloc( Vec_PtrSize(vParts) );
    Vec_PtrForEachEntry( Vec_Int_t *, vParts, vOne, i )
    {
        // start the partition 
        Abc_NtkConvertCos( pNtk, vOne, vOnePtr );
        pNtkAig = Abc_NtkCreateConeArray( pNtk, vOnePtr, 0 );
        // add nodes to the partition
        Vec_PtrForEachEntryStart( Abc_Ntk_t *, vStore, pNtk2, k, 1 )
        {
            Abc_NtkConvertCos( pNtk2, vOne, vOnePtr );
            Abc_NtkAppendToCone( pNtkAig, pNtk2, vOnePtr );
        }
        printf( "Fraiging part %4d  (out of %4d)  PI = %5d. PO = %5d. And = %6d. Lev = %4d.\r", 
            i+1, Vec_PtrSize(vParts), Abc_NtkPiNum(pNtkAig), Abc_NtkPoNum(pNtkAig), 
            Abc_NtkNodeNum(pNtkAig), Abc_AigLevel(pNtkAig) );
        // fraig the partition
        pNtkFraig = Abc_NtkFraig( pNtkAig, pParams, 1, 0 );
        Vec_PtrPush( vFraigs, pNtkFraig );
        Abc_NtkDelete( pNtkAig );
    }
    printf( "                                                                                          \r" );
    Vec_VecFree( (Vec_Vec_t *)vParts );

    Cmd_CommandExecute( Abc_FrameGetGlobalFrame(), "set progressbar" );

    // derive the final network
    pNtkFraig = Abc_NtkPartStitchChoices( pNtk, vFraigs );
    Vec_PtrForEachEntry( Abc_Ntk_t *, vFraigs, pNtkAig, i )
        Abc_NtkDelete( pNtkAig );
    Vec_PtrFree( vFraigs );
    Vec_PtrFree( vOnePtr );
    return pNtkFraig;
}

/**Function*************************************************************

  Synopsis    [Stitches together several networks with choice nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkFraigPartitionedTime( Abc_Ntk_t * pNtk, void * pParams )
{
    Vec_Ptr_t * vParts, * vFraigs, * vOnePtr;
    Vec_Int_t * vOne;
    Abc_Ntk_t * pNtkAig, * pNtkFraig;
    int i;
    abctime clk = Abc_Clock();

    // perform partitioning
    assert( Abc_NtkIsStrash(pNtk) );
//    vParts = Abc_NtkPartitionNaive( pNtk, 20 );
    vParts = Abc_NtkPartitionSmart( pNtk, 300, 0 );

    Cmd_CommandExecute( Abc_FrameGetGlobalFrame(), "unset progressbar" );

    // fraig each partition
    vOnePtr = Vec_PtrAlloc( 1000 );
    vFraigs = Vec_PtrAlloc( Vec_PtrSize(vParts) );
    Vec_PtrForEachEntry( Vec_Int_t *, vParts, vOne, i )
    {
        Abc_NtkConvertCos( pNtk, vOne, vOnePtr );
        pNtkAig = Abc_NtkCreateConeArray( pNtk, vOnePtr, 0 );
        pNtkFraig = Abc_NtkFraig( pNtkAig, pParams, 0, 0 );
        Vec_PtrPush( vFraigs, pNtkFraig );
        Abc_NtkDelete( pNtkAig );

        printf( "Finished part %5d (out of %5d)\r", i+1, Vec_PtrSize(vParts) );
    }
    Vec_VecFree( (Vec_Vec_t *)vParts );

    Cmd_CommandExecute( Abc_FrameGetGlobalFrame(), "set progressbar" );

    // derive the final network
    Vec_PtrForEachEntry( Abc_Ntk_t *, vFraigs, pNtkAig, i )
        Abc_NtkDelete( pNtkAig );
    Vec_PtrFree( vFraigs );
    Vec_PtrFree( vOnePtr );
    ABC_PRT( "Partitioned fraiging time", Abc_Clock() - clk );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

