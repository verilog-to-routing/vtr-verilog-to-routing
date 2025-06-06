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
#include "map/mio/mio.h"

#ifdef WIN32
#include <process.h> 
#define unlink _unlink
#else
#include <unistd.h>
#endif

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


/**Function*************************************************************

  Synopsis    [Optimization.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Abc_NtkStochSynthesis( Vec_Ptr_t * vWins, char * pScript )
{
    Vec_Int_t * vGains = Vec_IntStart( Vec_PtrSize(vWins) );
    Abc_Ntk_t * pNtk, * pNew; int i;
    Vec_PtrForEachEntry( Abc_Ntk_t *, vWins, pNtk, i )
    {
        Abc_FrameReplaceCurrentNetwork( Abc_FrameGetGlobalFrame(), Abc_NtkDupDfs(pNtk) );
        if ( Abc_FrameIsBatchMode() )
        {
            if ( Cmd_CommandExecute(Abc_FrameGetGlobalFrame(), pScript) )
            {
                Abc_Print( 1, "Something did not work out with the command \"%s\".\n", pScript );
                return vGains;
            }
        }
        else
        {
            Abc_FrameSetBatchMode( 1 );
            if ( Cmd_CommandExecute(Abc_FrameGetGlobalFrame(), pScript) )
            {
                Abc_Print( 1, "Something did not work out with the command \"%s\".\n", pScript );
                Abc_FrameSetBatchMode( 0 );
                return vGains;
            }
            Abc_FrameSetBatchMode( 0 );
        }
        pNew = Abc_FrameReadNtk(Abc_FrameGetGlobalFrame());
        if ( Abc_NtkIsMappedLogic(pNew) && Abc_NtkIsMappedLogic(pNtk) )
        {
            double Before = Abc_NtkGetMappedArea(pNtk);
            double After  = Abc_NtkGetMappedArea(pNew);
            if ( After < Before )
            {
                Vec_IntWriteEntry( vGains, i, (int)(Before - After) );
                Abc_NtkDelete( pNtk );
                pNtk = Abc_NtkDupDfs( pNew );
            }
        }
        else
        {
            if ( Abc_NtkNodeNum(pNew) < Abc_NtkNodeNum(pNtk) )
            {
                Vec_IntWriteEntry( vGains, i, Abc_NtkNodeNum(pNtk) - Abc_NtkNodeNum(pNew) );
                Abc_NtkDelete( pNtk );
                pNtk = Abc_NtkDupDfs( pNew );
            }
        }
        Vec_PtrWriteEntry( vWins, i, pNtk );
    }
    return vGains;
}


/**Function*************************************************************

  Synopsis    [Generic concurrent processing.]

  Description [User-defined problem-specific data and the way to process it.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/

typedef struct StochSynData_t_
{
    Abc_Ntk_t *  pIn;
    Abc_Ntk_t *  pOut;
    char *       pScript;
    int          Rand;
    int          TimeOut;
} StochSynData_t;

Abc_Ntk_t * Abc_NtkStochProcessOne( Abc_Ntk_t * p, char * pScript0, int Rand, int TimeSecs )
{
    extern int Abc_NtkWriteToFile( char * pFileName, Abc_Ntk_t * pNtk );
    extern Abc_Ntk_t * Abc_NtkReadFromFile( char * pFileName );
    Abc_Ntk_t * pNew, * pTemp;
    char FileName[100], Command[1000], PreCommand[500] = {0};
    char * pLibFileName = Abc_NtkIsMappedLogic(p) ? Mio_LibraryReadFileName((Mio_Library_t *)p->pManFunc) : NULL;
    if ( pLibFileName ) sprintf( PreCommand, "read_genlib %s; ", pLibFileName );
    sprintf( FileName, "%06x.mm", Rand );
    Abc_NtkWriteToFile( FileName, p );    
    char * pScript = Abc_UtilStrsav( pScript0 );
    sprintf( Command, "./abc -q \"%sread_mm %s; %s; write_mm %s\"", PreCommand[0] ? PreCommand : "", FileName, pScript, FileName );    
#if defined(__wasm)
    if ( 1 )
#else
    if ( system( (char *)Command ) )    
#endif
    {
        fprintf( stderr, "The following command has returned non-zero exit status:\n" );
        fprintf( stderr, "\"%s\"\n", (char *)Command );
        fprintf( stderr, "Sorry for the inconvenience.\n" );
        fflush( stdout );
        unlink( FileName );
        ABC_FREE( pScript );
        return Abc_NtkDupDfs(p);
    }
    ABC_FREE( pScript );
    pNew = Abc_NtkReadFromFile( FileName );
    unlink( FileName );
    if ( pNew && Abc_NtkGetMappedArea(pNew) < Abc_NtkGetMappedArea(p) ) {
        pNew = Abc_NtkDupDfs( pTemp = pNew );
        Abc_NtkDelete( pTemp );
        return pNew;
    }
    if ( pNew ) Abc_NtkDelete( pNew );
    return Abc_NtkDupDfs(p);
}

int Abc_NtkStochProcess1( void * p )
{
    StochSynData_t * pData = (StochSynData_t *)p;
    assert( pData->pIn != NULL );
    assert( pData->pOut == NULL );
    pData->pOut = Abc_NtkStochProcessOne( pData->pIn, pData->pScript, pData->Rand, pData->TimeOut );
    return 1;
}

Vec_Int_t * Abc_NtkStochProcess( Vec_Ptr_t * vWins, char * pScript, int nProcs, int TimeSecs, int fVerbose )
{
    if ( nProcs <= 2 ) {
        return Abc_NtkStochSynthesis( vWins, pScript );
    }
    Vec_Int_t * vGains = Vec_IntStart( Vec_PtrSize(vWins) );
    StochSynData_t * pData = ABC_CALLOC( StochSynData_t, Vec_PtrSize(vWins) );
    Vec_Ptr_t * vData = Vec_PtrAlloc( Vec_PtrSize(vWins) ); 
    Abc_Ntk_t * pNtk; int i;
    //Abc_Random(1);
    Vec_PtrForEachEntry( Abc_Ntk_t *, vWins, pNtk, i ) {
        pData[i].pIn     = pNtk;
        pData[i].pOut    = NULL;
        pData[i].pScript = pScript;
        pData[i].Rand    = Abc_Random(0) % 0x1000000;
        pData[i].TimeOut = TimeSecs;
        Vec_PtrPush( vData, pData+i );
    }
    Util_ProcessThreads( Abc_NtkStochProcess1, vData, nProcs, TimeSecs, fVerbose );
    // replace old AIGs by new AIGs
    Vec_PtrForEachEntry( Abc_Ntk_t *, vWins, pNtk, i ) {
        if ( Abc_NtkIsMappedLogic(pNtk) )
            Vec_IntWriteEntry( vGains, i, (int)(Abc_NtkGetMappedArea(pNtk) - Abc_NtkGetMappedArea(pData[i].pOut)) );
        else
            Vec_IntWriteEntry( vGains, i, Abc_NtkNodeNum(pNtk) - Abc_NtkNodeNum(pData[i].pOut) );
        Abc_NtkDelete( pNtk );
        Vec_PtrWriteEntry( vWins, i, pData[i].pOut );
    }
    Vec_PtrFree( vData );
    ABC_FREE( pData );
    return vGains;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if this window has a topo error (forward path from an output to an input).]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkWindowCheckTopoError_rec( Abc_Obj_t * pObj )
{
    if ( !Abc_ObjIsNode(pObj) )
        return 0;
    if ( Abc_NodeIsTravIdPrevious(pObj) )
        return 1; // there is an error
    if ( Abc_NodeIsTravIdCurrent(pObj) )
        return 0; // there is no error; visited this node before
    Abc_NodeSetTravIdPrevious(pObj);
    Abc_Obj_t * pFanin; int i;
    Abc_ObjForEachFanin( pObj, pFanin, i )
    if ( Abc_NtkWindowCheckTopoError_rec(pFanin) )
        return 1;
    Abc_NodeSetTravIdCurrent(pObj);
    return 0;
}
int Abc_NtkWindowCheckTopoError( Abc_Ntk_t * p, Vec_Int_t * vIns, Vec_Int_t * vOuts )
{
    Abc_Obj_t * pObj; int i, fError = 0;
    // outputs should be internal nodes
    Abc_NtkForEachObjVec( vOuts, p, pObj, i )
        assert(Abc_ObjIsNode(pObj));
    // mark outputs
    Abc_NtkIncrementTravId( p );
    Abc_NtkForEachObjVec( vOuts, p, pObj, i )
        Abc_NodeSetTravIdCurrent(pObj);
    // start from inputs and make sure we do not reach any of the outputs
    Abc_NtkIncrementTravId( p );
    Abc_NtkForEachObjVec( vIns, p, pObj, i )
        fError |= Abc_NtkWindowCheckTopoError_rec(pObj);
    return fError;
}

/**Function*************************************************************

  Synopsis    [Updates the AIG after multiple windows have been optimized.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkCreateNodeMapped( Abc_Ntk_t * pNew, Abc_Obj_t * pObj )
{
    Abc_Obj_t * pObjNew = Abc_NtkDupObj( pNew, pObj, 0 );
    Abc_Obj_t * pFanin; int i;
    Abc_ObjForEachFanin( pObj, pFanin, i )
        Abc_ObjAddFanin( pObjNew, pFanin->pCopy );
}
void Abc_NtkInsertPartitions_rec( Abc_Ntk_t * pNew, Abc_Obj_t * pObj, Vec_Int_t * vMap, Vec_Ptr_t * vvIns, Vec_Ptr_t * vvOuts, Vec_Ptr_t * vWins )
{
    if ( pObj->pCopy )
        return;
    assert( Abc_ObjIsNode(pObj) );
    if ( Vec_IntEntry(vMap, Abc_ObjId(pObj)) == -1 ) // this is a regular node
    {
        Abc_Obj_t * pFanin; int i;
        Abc_ObjForEachFanin( pObj, pFanin, i )
            Abc_NtkInsertPartitions_rec( pNew, pFanin, vMap, vvIns, vvOuts, vWins );
        Abc_NtkCreateNodeMapped( pNew, pObj );
        return;
    }
    // this node is an output of a window
    int iWin = Vec_IntEntry(vMap, Abc_ObjId(pObj));
    Vec_Int_t * vIns  = (Vec_Int_t *)Vec_PtrEntry(vvIns, iWin);
    Vec_Int_t * vOuts = (Vec_Int_t *)Vec_PtrEntry(vvOuts, iWin);
    Abc_Ntk_t * pWin  = (Abc_Ntk_t *)Vec_PtrEntry(vWins, iWin); 
    // build transinvite fanins of window inputs
    Abc_Obj_t * pNode; int i;
    Abc_NtkForEachObjVec( vIns, pObj->pNtk, pNode, i ) {
        Abc_NtkInsertPartitions_rec( pNew, pNode, vMap, vvIns, vvOuts, vWins );
        Abc_NtkPi(pWin, i)->pCopy = pNode->pCopy;
    }
    // add window nodes
    Vec_Ptr_t * vNodes = Abc_NtkDfs( pWin, 0 );
    Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pNode, i )
        Abc_NtkCreateNodeMapped( pNew, pNode );
    Vec_PtrFree( vNodes );
    // transfer to window outputs
    Abc_NtkForEachObjVec( vOuts, pObj->pNtk, pNode, i )
        pNode->pCopy = Abc_ObjFanin0(Abc_NtkPo(pWin, i))->pCopy;
    assert( pObj->pCopy );
}
Abc_Ntk_t * Abc_NtkInsertPartitions( Abc_Ntk_t * p, Vec_Ptr_t * vvIns, Vec_Ptr_t * vvOuts, Vec_Ptr_t * vWins, int fOverlap, Vec_Int_t * vGains )
{
    if ( vvIns == NULL ) {
        assert( vvOuts == NULL );
        assert( Vec_PtrSize(vWins) == 1 );
        return Abc_NtkDupDfs( (Abc_Ntk_t *)Vec_PtrEntry(vWins, 0) );        
    }
    if ( fOverlap ) {
        Vec_Ptr_t * vvInsNew  = Vec_PtrAlloc( 10 );
        Vec_Ptr_t * vvOutsNew = Vec_PtrAlloc( 10 );
        Vec_Ptr_t * vvWinsNew = Vec_PtrAlloc( 10 );
        Abc_NtkIncrementTravId( p );
        while ( 1 ) {
            int i, Gain, iEntry = Vec_IntArgMax(vGains);
            if ( Vec_IntEntry(vGains, iEntry) <= 0 )
                break;
            //printf( "Selecting partition %d with gain %d.\n", iEntry, Vec_IntEntry(vGains, iEntry) );
            Vec_IntWriteEntry( vGains, iEntry, -1 );
            Vec_PtrPush( vvInsNew,  Vec_IntDup((Vec_Int_t *)Vec_PtrEntry(vvIns,  iEntry)) );
            Vec_PtrPush( vvOutsNew, Vec_IntDup((Vec_Int_t *)Vec_PtrEntry(vvOuts, iEntry)) );
            Vec_PtrPush( vvWinsNew, Abc_NtkDupDfs((Abc_Ntk_t *)Vec_PtrEntry(vWins, iEntry)) );
            extern void Abc_NtKMarkTfiTfo( Vec_Int_t * vOne, Abc_Ntk_t * pNtk );
            Abc_NtKMarkTfiTfo( (Vec_Int_t *)Vec_PtrEntryLast(vvInsNew), p );
            Vec_IntForEachEntry( vGains, Gain, i ) {
                if ( Gain <= 0 )
                    continue;
                Vec_Int_t * vIns  = (Vec_Int_t *)Vec_PtrEntry(vvIns,  i);
                Vec_Int_t * vOuts = (Vec_Int_t *)Vec_PtrEntry(vvOuts, i);
                Abc_Obj_t * pNode; int j, k;
                Abc_NtkForEachObjVec( vIns, p, pNode, j )
                    if ( Abc_NodeIsTravIdCurrent(pNode) )
                        break;
                Abc_NtkForEachObjVec( vOuts, p, pNode, k )
                    if ( Abc_NodeIsTravIdCurrent(pNode) )
                        break;
                if ( j < Vec_IntSize(vIns) || k < Vec_IntSize(vOuts) )
                    Vec_IntWriteEntry( vGains, i, -1 );
            }         
        }
        ABC_SWAP( Vec_Ptr_t, *vvInsNew,  *vvIns  );
        ABC_SWAP( Vec_Ptr_t, *vvOutsNew, *vvOuts );
        ABC_SWAP( Vec_Ptr_t, *vvWinsNew, *vWins  );
        Vec_PtrFreeFunc( vvInsNew,   (void (*)(void *)) Vec_IntFree );
        Vec_PtrFreeFunc( vvOutsNew,  (void (*)(void *)) Vec_IntFree );           
        Vec_PtrFreeFunc( vvWinsNew,  (void (*)(void *)) Abc_NtkDelete );
    }
    // check consistency of input data
    Abc_Ntk_t * pNew, * pTemp; Abc_Obj_t * pObj; int i, k, iNode;
    Vec_PtrForEachEntry( Abc_Ntk_t *, vWins, pTemp, i ) {
        Vec_Int_t * vIns  = (Vec_Int_t *)Vec_PtrEntry(vvIns, i);
        Vec_Int_t * vOuts = (Vec_Int_t *)Vec_PtrEntry(vvOuts, i);
        assert( Vec_IntSize(vIns)  == Abc_NtkPiNum(pTemp) );
        assert( Vec_IntSize(vOuts) == Abc_NtkPoNum(pTemp) );
        assert( !Abc_NtkWindowCheckTopoError(p, vIns, vOuts) );        
    }
    // create mapping of window outputs into window IDs
    Vec_Int_t * vMap = Vec_IntStartFull( Abc_NtkObjNumMax(p) ), * vOuts;
    Vec_PtrForEachEntry( Vec_Int_t *, vvOuts, vOuts, i )
        Vec_IntForEachEntry( vOuts, iNode, k ) {
            assert( Vec_IntEntry(vMap, iNode) == -1 );
            Vec_IntWriteEntry( vMap, iNode, i );
        }
    Abc_NtkCleanCopy( p );
    pNew = Abc_NtkStartFrom( p, p->ntkType, p->ntkFunc );
    pNew->pManFunc = p->pManFunc;
    Abc_NtkForEachCo( p, pObj, i )
        Abc_NtkInsertPartitions_rec( pNew, Abc_ObjFanin0(pObj), vMap, vvIns, vvOuts, vWins );
    Abc_NtkForEachCo( p, pObj, i )
        Abc_ObjAddFanin( Abc_NtkCo(pNew, i), Abc_ObjFanin0(pObj)->pCopy );
    Vec_IntFree( vMap );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Partitioning.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_ObjDfsMark_rec( Abc_Obj_t * p )
{
    Abc_Obj_t * pFanin; int i;
    assert( !p->fMarkA );
    if ( Abc_NodeIsTravIdCurrent( p ) )
        return;
    Abc_NodeSetTravIdCurrent( p );
    Abc_ObjForEachFanin( p, pFanin, i )
        Abc_ObjDfsMark_rec( pFanin );
}
void Abc_ObjDfsMark2_rec( Abc_Obj_t * p )
{
    Abc_Obj_t * pFanout; int i;
    assert( !p->fMarkA );
    if ( Abc_NodeIsTravIdCurrent( p ) )
        return;
    Abc_NodeSetTravIdCurrent( p );
    Abc_ObjForEachFanout( p, pFanout, i )
        Abc_ObjDfsMark2_rec( pFanout );
}
Vec_Int_t * Abc_NtkDeriveWinNodes( Abc_Ntk_t * pNtk, Vec_Int_t * vIns, Vec_Wec_t * vStore )
{
    Vec_Int_t * vLevel, * vNodes = Vec_IntAlloc( 100 );
    Abc_Obj_t * pObj, * pNext; int i, k, iLevel;
    Vec_WecForEachLevel( vStore, vLevel, i )
        Vec_IntClear( vLevel );
    // mark the TFI cones of the inputs
    Abc_NtkIncrementTravId( pNtk );
    Abc_NtkForEachObjVec( vIns, pNtk, pObj, i )
        Abc_ObjDfsMark_rec( pObj );
    // add unrelated fanouts of the inputs to storage
    Abc_NtkForEachObjVec( vIns, pNtk, pObj, i )
        Abc_ObjForEachFanout( pObj, pNext, k )
            if ( Abc_ObjIsNode(pNext) && !Abc_NodeIsTravIdCurrent(pNext) && !pNext->fMarkA ) {
                pNext->fMarkA = 1;
                Vec_WecPush( vStore, Abc_ObjLevel(pNext), Abc_ObjId(pNext) );
            }
    // mark the inputs
    Abc_NtkIncrementTravId( pNtk );
    Abc_NtkForEachObjVec( vIns, pNtk, pObj, i )
        Abc_NodeSetTravIdCurrent(pObj);
    // collect those fanouts that are completely supported by the inputs
    Vec_WecForEachLevel( vStore, vLevel, iLevel )
        Abc_NtkForEachObjVec( vLevel, pNtk, pObj, i ) {
            assert( !Abc_NodeIsTravIdCurrent(pObj) );
            assert( pObj->fMarkA );
            pObj->fMarkA = 0;
            Abc_ObjForEachFanin( pObj, pNext, k )
                if ( !Abc_NodeIsTravIdCurrent(pNext) )
                    break;
            if ( k < Abc_ObjFaninNum(pObj) )
                continue;
            Abc_NodeSetTravIdCurrent(pObj);
            Vec_IntPush( vNodes, Abc_ObjId(pObj) );
            assert( Abc_ObjIsNode(pObj) );
            // add fanouts of this node to storage
            Abc_ObjForEachFanout( pObj, pNext, k ) 
                if ( Abc_ObjIsNode(pNext) && !Abc_NodeIsTravIdCurrent(pNext) && !pNext->fMarkA ) {
                    pNext->fMarkA = 1;
                    assert( Abc_ObjLevel(pNext) > iLevel );
                    Vec_WecPush( vStore, Abc_ObjLevel(pNext), Abc_ObjId(pNext) );
                }
        }
    Vec_IntSort( vNodes, 0 );
    return vNodes;
}
Vec_Ptr_t * Abc_NtkDeriveWinNodesAll( Abc_Ntk_t * pNtk, Vec_Ptr_t * vvIns, Vec_Wec_t * vStore )
{
    Vec_Int_t * vIns; int i;
    Vec_Ptr_t * vvNodes = Vec_PtrAlloc( Vec_PtrSize(vvIns) );
    Vec_PtrForEachEntry( Vec_Int_t *, vvIns, vIns, i ) 
        Vec_PtrPush( vvNodes, Abc_NtkDeriveWinNodes(pNtk, vIns, vStore) );
    return vvNodes;
}
Vec_Int_t * Abc_NtkDeriveWinOuts( Abc_Ntk_t * pNtk, Vec_Int_t * vNodes )
{
    Vec_Int_t * vOuts = Vec_IntAlloc( 100 );
    Abc_Obj_t * pObj, * pNext; int i, k;
    // mark the nodes in the window
    Abc_NtkIncrementTravId( pNtk );
    Abc_NtkForEachObjVec( vNodes, pNtk, pObj, i )
        Abc_NodeSetTravIdCurrent(pObj);
    // collect nodes that have unmarked fanouts
    Abc_NtkForEachObjVec( vNodes, pNtk, pObj, i ) {
        Abc_ObjForEachFanout( pObj, pNext, k )
            if ( !Abc_NodeIsTravIdCurrent(pNext) )
                break;
        if ( k < Abc_ObjFanoutNum(pObj) )
            Vec_IntPush( vOuts, Abc_ObjId(pObj) );
    }
    if ( Vec_IntSize(vOuts) == 0 )
        printf( "Window with %d internal nodes has no outputs (are these dangling nodes?).\n", Vec_IntSize(vNodes) );
    return vOuts;
}
Vec_Ptr_t * Abc_NtkDeriveWinOutsAll( Abc_Ntk_t * pNtk, Vec_Ptr_t * vvNodes )
{
    Vec_Int_t * vNodes; int i;
    Vec_Ptr_t * vvOuts = Vec_PtrAlloc( Vec_PtrSize(vvNodes) );
    Vec_PtrForEachEntry( Vec_Int_t *, vvNodes, vNodes, i ) 
        Vec_PtrPush( vvOuts, Abc_NtkDeriveWinOuts(pNtk, vNodes) );
    return vvOuts;
}
void Abc_NtkPermuteLevel( Abc_Ntk_t * pNtk, int Level )
{
    Abc_Obj_t * pObj, * pNext; int i, k;
    Abc_NtkForEachNode( pNtk, pObj, i ) {
        int LevelMin = Abc_ObjLevel(pObj), LevelMax = Level + 1;
        Abc_ObjForEachFanout( pObj, pNext, k )
            if ( Abc_ObjIsNode(pNext) )
                LevelMax = Abc_MinInt( LevelMax, Abc_ObjLevel(pNext) );
        if ( LevelMin == LevelMax ) continue;
        assert( LevelMin < LevelMax );
        // randomly set level between LevelMin and LevelMax-1
        pObj->Level = LevelMin + (Abc_Random(0) % (LevelMax - LevelMin));
        assert( pObj->Level < LevelMax );
    }
}
Vec_Int_t * Abc_NtkCollectObjectsPointedTo( Abc_Ntk_t * pNtk, int Level )
{
    Vec_Int_t * vRes = Vec_IntAlloc( 100 ); 
    Abc_Obj_t * pObj, * pFanin; int i, k;
    Abc_NtkIncrementTravId( pNtk );
    Abc_NtkForEachNode( pNtk, pObj, i ) {
        if ( Abc_ObjLevel(pObj) <= Level )
            continue;
        Abc_ObjForEachFanin( pObj, pFanin, k )
            if ( Abc_ObjLevel(pFanin) <= Level && !Abc_NodeIsTravIdCurrent(pFanin) ) {
                Abc_NodeSetTravIdCurrent(pFanin);
                Vec_IntPush( vRes, Abc_ObjId(pFanin) );
            }
    }
    Abc_NtkForEachCo( pNtk, pObj, i ) {
        pFanin = Abc_ObjFanin0(pObj);
        if ( Abc_ObjLevel(pFanin) <= Level && !Abc_NodeIsTravIdCurrent(pFanin) && Abc_ObjFaninNum(pFanin) > 0 ) {
            Abc_NodeSetTravIdCurrent(pFanin);
            Vec_IntPush( vRes, Abc_ObjId(pFanin) );
        }
    }
    Vec_IntSort( vRes, 0 );
    return vRes;
}
Vec_Wec_t * Abc_NtkCollectObjectsWithSuppLimit( Abc_Ntk_t * pNtk, int Level, int nSuppMax )
{
    Vec_Wec_t * vResSupps = NULL;
    Vec_Int_t * vBelow   = Abc_NtkCollectObjectsPointedTo( pNtk, Level );
    Vec_Wec_t * vSupps   = Vec_WecStart( Vec_IntSize(vBelow) );
    Vec_Int_t * vSuppIds = Vec_IntStartFull( Abc_NtkObjNumMax(pNtk)+1 );
    Vec_Int_t * vTemp[2] = { Vec_IntAlloc(100), Vec_IntAlloc(100) };
    Abc_Obj_t * pObj, * pFanin; int i, k, Count = 0;
    Abc_NtkForEachObjVec( vBelow, pNtk, pObj, i ) {
        Vec_IntWriteEntry( vSuppIds, Abc_ObjId(pObj), i );
        Vec_IntPush( Vec_WecEntry(vSupps, i), Abc_ObjId(pObj) );
    }
    Abc_NtkForEachNode( pNtk, pObj, i ) {
        if ( Abc_ObjLevel(pObj) <= Level )
            continue;
        Vec_IntClear( vTemp[0] );
        Abc_ObjForEachFanin( pObj, pFanin, k ) {
            int iSuppId = Vec_IntEntry( vSuppIds, Abc_ObjId(pFanin) );
            if ( iSuppId == -1 )
                break;
            Vec_IntTwoMerge2( Vec_WecEntry(vSupps, iSuppId), vTemp[0], vTemp[1] );
            ABC_SWAP( Vec_Int_t *, vTemp[0], vTemp[1] );
        }
        if ( k < Abc_ObjFaninNum(pObj) || Vec_IntSize(vTemp[0]) > nSuppMax ) {
            Count++;
            continue;
        }
        Vec_IntWriteEntry( vSuppIds, Abc_ObjId(pObj), Vec_WecSize(vSupps) );
        Vec_IntAppend( Vec_WecPushLevel(vSupps), vTemp[0] );
    }
    // remove those supported nodes that are in the TFI cones of others
    Abc_NtkIncrementTravId( pNtk );
    Abc_NtkForEachNode( pNtk, pObj, i )
        if ( Abc_ObjLevel(pObj) > Level && Vec_IntEntry(vSuppIds, i) >= 0 && !Abc_NodeIsTravIdCurrent(pObj) ) {
            Abc_ObjDfsMark_rec(pObj);
            Abc_NodeSetTravIdPrevious(pObj);
        }
    // create the result
    vResSupps = Vec_WecAlloc( 100 );
    Abc_NtkForEachNode( pNtk, pObj, i )
        if ( Abc_ObjLevel(pObj) > Level && Vec_IntEntry(vSuppIds, i) >= 0 && !Abc_NodeIsTravIdCurrent(pObj) ) {
            Vec_Int_t * vSupp = Vec_WecEntry( vSupps, Vec_IntEntry(vSuppIds, i) );
            if ( Vec_IntSize(vSupp) < 4 )
                continue;
            Vec_Int_t * vThis = Vec_WecPushLevel( vResSupps );
            Vec_IntGrow( vThis, Vec_IntSize(vSupp) + 1 );
            Vec_IntAppend( vThis, vSupp );
            //Vec_IntPush( vThis, Abc_ObjId(pObj) );
        }
    //printf( "Inputs = %d. Nodes with %d-support = %d. Nodes with larger support = %d. Selected outputs = %d.\n", 
    //    Vec_IntSize(vBelow), nSuppMax, Vec_WecSize(vSupps), Count, Vec_WecSize(vResSupps) );
    Vec_WecFree( vSupps );
    Vec_IntFree( vSuppIds );
    Vec_IntFree( vBelow );
    Vec_IntFree( vTemp[0] );
    Vec_IntFree( vTemp[1] );
    return vResSupps;
}
// removes all supports that overlap with this one
void Abc_NtKSelectRemove( Vec_Wec_t * vSupps, Vec_Int_t * vOne )
{
    Vec_Int_t * vLevel; int i;
    Vec_WecForEachLevel( vSupps, vLevel, i ) 
        if ( Vec_IntTwoCountCommon(vLevel, vOne) > 0 )
            Vec_IntClear( vLevel );
    Vec_WecRemoveEmpty( vSupps );
}
// marks TFI/TFO of this one
void Abc_NtKMarkTfiTfo( Vec_Int_t * vOne, Abc_Ntk_t * pNtk )
{
    int i; Abc_Obj_t * pObj;
    Abc_NtkForEachObjVec( vOne, pNtk, pObj, i ) {
        Abc_NodeSetTravIdPrevious(pObj);
        Abc_ObjDfsMark_rec( pObj );
        Abc_NodeSetTravIdPrevious(pObj);
        Abc_ObjDfsMark2_rec( pObj );
    }
}
// removes all supports that overlap with the TFI/TFO cones of this one
void Abc_NtKSelectRemove2( Vec_Wec_t * vSupps, Vec_Int_t * vOne, Abc_Ntk_t * pNtk )
{
    Vec_Int_t * vLevel; int i, k; Abc_Obj_t * pObj;
    Abc_NtkForEachObjVec( vOne, pNtk, pObj, i ) {
        Abc_NodeSetTravIdPrevious(pObj);
        Abc_ObjDfsMark_rec( pObj );
        Abc_NodeSetTravIdPrevious(pObj);
        Abc_ObjDfsMark2_rec( pObj );
    }
    Vec_WecForEachLevel( vSupps, vLevel, i ) {
        Abc_NtkForEachObjVec( vLevel, pNtk, pObj, k ) 
            if ( Abc_NodeIsTravIdCurrent(pObj) )
                break;
        if ( k < Vec_IntSize(vLevel) )
            Vec_IntClear( vLevel );
    }
    Vec_WecRemoveEmpty( vSupps );
}
// removes all supports that are contained in this one
void Abc_NtKSelectRemove3( Vec_Wec_t * vSupps, Vec_Int_t * vOne )
{
    Vec_Int_t * vLevel; int i;
    Vec_WecForEachLevel( vSupps, vLevel, i )
        if ( Vec_IntTwoCountCommon(vLevel, vOne) == Vec_IntSize(vLevel) )
            Vec_IntClear( vLevel );
    Vec_WecRemoveEmpty( vSupps );
}
Vec_Ptr_t * Abc_NtkDeriveWinInsAll( Vec_Wec_t * vSupps, int nSuppMax, Abc_Ntk_t * pNtk, int fOverlap )
{
    Vec_Ptr_t * vRes = Vec_PtrAlloc( 100 );
    Abc_NtkIncrementTravId( pNtk );
    while ( Vec_WecSize(vSupps) > 0 ) {
        int i, Item, iRand = Abc_Random(0) % Vec_WecSize(vSupps);
        Vec_Int_t * vLevel, * vLevel2 = Vec_WecEntry( vSupps, iRand );
        Vec_Int_t * vCopy = Vec_IntDup( vLevel2 );
        if ( Vec_IntSize(vLevel2) == nSuppMax ) {
            Vec_PtrPush( vRes, vCopy );
            if ( fOverlap )
                Abc_NtKSelectRemove3( vSupps, vCopy );
            else
                Abc_NtKSelectRemove2( vSupps, vCopy, pNtk );
            continue;
        }
        // find another support, which maximizes the union but does not exceed nSuppMax
        int iBest = iRand, nUnion = Vec_IntSize(vCopy);
        Vec_WecForEachLevel( vSupps, vLevel, i ) {
            if ( i == iRand ) continue;
            int nCommon = Vec_IntTwoCountCommon(vLevel, vCopy);
            int nUnionCur = Vec_IntSize(vLevel) + Vec_IntSize(vCopy) - nCommon;
            if ( nUnionCur <= nSuppMax && nUnion < nUnionCur ) {
                nUnion = nUnionCur;
                iBest = i;
            }
        }
        vLevel = Vec_WecEntry( vSupps, iBest );
        Vec_IntForEachEntry( vLevel, Item, i )
            Vec_IntPushUniqueOrder( vCopy, Item );
        Vec_PtrPush( vRes, vCopy );
        if ( fOverlap )
            Abc_NtKSelectRemove3( vSupps, vCopy );
        else
            Abc_NtKSelectRemove2( vSupps, vCopy, pNtk );
    }
    return vRes;
}
Abc_Ntk_t * Abc_NtkDupWindow( Abc_Ntk_t * p, Vec_Int_t * vIns, Vec_Int_t * vNodes, Vec_Int_t * vOuts )
{
    Abc_Ntk_t * pNew = Abc_NtkAlloc( ABC_NTK_LOGIC, ABC_FUNC_MAP, 0 );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    pNew->pManFunc = p->pManFunc;
    Abc_Obj_t * pObj; int i;
    Abc_NtkForEachObjVec( vIns, p, pObj, i )
        pObj->pCopy = Abc_NtkCreatePi(pNew);
    Abc_NtkForEachObjVec( vOuts, p, pObj, i )
        Abc_NtkCreatePo( pNew );
    Abc_NtkForEachObjVec( vNodes, p, pObj, i )
        Abc_NtkCreateNodeMapped( pNew, pObj );
    Abc_NtkForEachObjVec( vOuts, p, pObj, i )
        Abc_ObjAddFanin( Abc_NtkCo(pNew, i), pObj->pCopy );
    Abc_NtkForEachObjVec( vIns, p, pObj, i )
        pObj->pCopy = NULL;
    Abc_NtkForEachObjVec( vNodes, p, pObj, i )
        pObj->pCopy = NULL;
    Abc_NtkAddDummyPiNames( pNew );
    Abc_NtkAddDummyPoNames( pNew );
    return pNew;
}
Vec_Ptr_t * Abc_NtkDupWindows( Abc_Ntk_t * pNtk, Vec_Ptr_t * vvIns, Vec_Ptr_t * vvNodes, Vec_Ptr_t * vvOuts )
{
    Vec_Int_t * vNodes; int i;
    Vec_Ptr_t * vWins = Vec_PtrAlloc( Vec_PtrSize(vvIns) );
    assert( Vec_PtrSize(vvIns)  == Vec_PtrSize(vvNodes) );
    assert( Vec_PtrSize(vvOuts) == Vec_PtrSize(vvNodes) );
    Abc_NtkCleanCopy( pNtk );
    Abc_NtkCleanMarkABC( pNtk );
    Vec_PtrForEachEntry( Vec_Int_t *, vvNodes, vNodes, i ) {
        Vec_Int_t * vIns  = (Vec_Int_t *)Vec_PtrEntry(vvIns, i);
        Vec_Int_t * vOuts = (Vec_Int_t *)Vec_PtrEntry(vvOuts, i);
        Abc_Ntk_t * pNew  = Abc_NtkDupWindow( pNtk, vIns, vNodes, vOuts );
        Vec_PtrPush( vWins, pNew );
    }
    return vWins;
}
Vec_Ptr_t * Abc_NtkExtractPartitions( Abc_Ntk_t * pNtk, int Iter, int nSuppMax, Vec_Ptr_t ** pvIns, Vec_Ptr_t ** pvOuts, Vec_Ptr_t ** pvNodes, int fOverlap )
{
    // if ( Abc_NtkCiNum(pNtk) <= nSuppMax ) {
    //     Vec_Ptr_t * vWins = Vec_PtrAlloc( 1 );
    //     Vec_PtrPush( vWins, Abc_NtkDupDfs(pNtk) );
    //     *pvIns = *pvOuts = *pvNodes = NULL;
    //     return vWins;
    // }
    // int iUseRevL = Iter % 3 == 0 ? 0 : Abc_Random(0) & 1;
    int iUseRevL = Abc_Random(0) & 1;
    int LevelMax = iUseRevL ? Abc_NtkLevelR(pNtk) : Abc_NtkLevel(pNtk);
    // int LevelCut = Iter % 3 == 0 ? 0 : LevelMax > 8 ? 2 + (Abc_Random(0) % (LevelMax - 4)) : 0;
    int LevelCut = LevelMax > 8 ? (Abc_Random(0) % (LevelMax - 4)) : 0;
    // printf( "Using %s cut level %d (out of %d)\n", iUseRevL ? "reverse": "direct", LevelCut, LevelMax );
    // Abc_NtkPermuteLevel( pNtk, LevelMax );
    Vec_Wec_t * vStore = Vec_WecStart( LevelMax+1 );
    Vec_Wec_t * vSupps = Abc_NtkCollectObjectsWithSuppLimit( pNtk, LevelCut, nSuppMax );
    Vec_Ptr_t * vIns   = Abc_NtkDeriveWinInsAll( vSupps, nSuppMax, pNtk, fOverlap );
    Vec_Ptr_t * vNodes = Abc_NtkDeriveWinNodesAll( pNtk, vIns, vStore );
    Vec_Ptr_t * vOuts  = Abc_NtkDeriveWinOutsAll( pNtk, vNodes );
    Vec_Ptr_t * vWins  = Abc_NtkDupWindows( pNtk, vIns, vNodes, vOuts );
    Vec_WecFree( vSupps );
    Vec_WecFree( vStore );
    *pvIns = vIns;
    *pvOuts = vOuts;
    *pvNodes = vNodes;
    return vWins;
}

/**Function*************************************************************

  Synopsis    [Performs stochastic mapping.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkStochMap( int nSuppMax, int nIters, int TimeOut, int Seed, int fOverlap, int fVerbose, char * pScript, int nProcs )
{
    abctime clkStart    = Abc_Clock(); int i;
    abctime nTimeToStop = TimeOut ? Abc_Clock() + TimeOut * CLOCKS_PER_SEC : 0;
    float aEnd, aBeg    = Abc_NtkGetMappedArea(Abc_FrameReadNtk(Abc_FrameGetGlobalFrame()));
    assert( Abc_NtkIsMappedLogic(Abc_FrameReadNtk(Abc_FrameGetGlobalFrame())) );
    Abc_Random(1);
    for ( i = 0; i < 10+Seed; i++ )
        Abc_Random(0);
    if ( fVerbose ) {
        printf( "Running %d iterations of the script \"%s\"", nIters, pScript );
        if ( nProcs > 2 )
            printf( " using %d concurrent threads.\n", nProcs-1 );
        else
            printf( " without concurrency.\n" );
        fflush(stdout);
    }
    Vec_Ptr_t * vIns = NULL, * vOuts = NULL, * vNodes = NULL;
    for ( i = 0; i < nIters; i++ )
    {
        abctime clk = Abc_Clock();
        Abc_Ntk_t * pNtk   = Abc_NtkDupDfs(Abc_FrameReadNtk(Abc_FrameGetGlobalFrame()));
        Vec_Ptr_t * vWins  = Abc_NtkExtractPartitions( pNtk, i, nSuppMax, &vIns, &vOuts, &vNodes, fOverlap );
        Vec_Int_t * vGains = Abc_NtkStochProcess( vWins, pScript, nProcs, 0, 0 ); int nPartsInit = Vec_PtrSize(vWins);
        Abc_Ntk_t * pNew   = Abc_NtkInsertPartitions( pNtk, vIns, vOuts, vWins, fOverlap, vGains );
        Abc_FrameReplaceCurrentNetwork( Abc_FrameGetGlobalFrame(), pNew );
        if ( fVerbose )
        printf( "Iteration %3d : Using %3d -> %3d partitions. Reducing area from %.2f to %.2f.  ", 
            i, nPartsInit, Vec_PtrSize(vWins), Abc_NtkGetMappedArea(pNtk), Abc_NtkGetMappedArea(pNew) ); 
        if ( fVerbose )
        Abc_PrintTime( 0, "Time", Abc_Clock() - clk );
        // cleanup
        Abc_NtkDelete( pNtk );
        Vec_PtrFreeFunc( vWins,  (void (*)(void *)) Abc_NtkDelete );
        Vec_IntFreeP( &vGains );
        if ( vIns )   Vec_PtrFreeFunc( vIns,   (void (*)(void *)) Vec_IntFree );
        if ( vOuts )  Vec_PtrFreeFunc( vOuts,  (void (*)(void *)) Vec_IntFree );                
        if ( vNodes ) Vec_PtrFreeFunc( vNodes, (void (*)(void *)) Vec_IntFree );                
        if ( nTimeToStop && Abc_Clock() > nTimeToStop )
        {
            printf( "Runtime limit (%d sec) is reached after %d iterations.\n", TimeOut, i );
            break;
        }
    }
    aEnd = Abc_NtkGetMappedArea(Abc_FrameReadNtk(Abc_FrameGetGlobalFrame()));
    if ( fVerbose )
    printf( "Cumulatively reduced area by %.2f %% after %d iterations.  ", 100.0*(aBeg - aEnd)/aBeg, nIters );
    if ( fVerbose )
    Abc_PrintTime( 0, "Total time", Abc_Clock() - clkStart );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

