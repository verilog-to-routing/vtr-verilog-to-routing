/**CFile****************************************************************

  FileName    [aigPart.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [AIG package.]

  Synopsis    [AIG partitioning package.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - April 28, 2007.]

  Revision    [$Id: aigPart.c,v 1.00 2007/04/28 00:00:00 alanmi Exp $]

***********************************************************************/

#include "aig.h"
#include "misc/tim/tim.h"
#include "proof/fra/fra.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Part_Man_t_     Part_Man_t;
struct Part_Man_t_
{
    int              nChunkSize;    // the size of one chunk of memory (~1 MB)
    int              nStepSize;     // the step size in saving memory (~64 bytes)
    char *           pFreeBuf;      // the pointer to free memory
    int              nFreeSize;     // the size of remaining free memory
    Vec_Ptr_t *      vMemory;       // the memory allocated
    Vec_Ptr_t *      vFree;         // the vector of free pieces of memory
};

typedef struct Part_One_t_     Part_One_t;
struct Part_One_t_
{
    int              nRefs;         // the number of references
    int              nOuts;         // the number of outputs
    int              nOutsAlloc;    // the array size
    int              pOuts[0];      // the array of outputs
};

static inline int    Part_SizeType( int nSize, int nStepSize )     { return nSize / nStepSize + ((nSize % nStepSize) > 0); }
static inline char * Part_OneNext( char * pPart )                  { return *((char **)pPart);                             }
static inline void   Part_OneSetNext( char * pPart, char * pNext ) { *((char **)pPart) = pNext;                            }

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Start the memory manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Part_Man_t * Part_ManStart( int nChunkSize, int nStepSize )
{
    Part_Man_t * p;
    p = ABC_ALLOC( Part_Man_t, 1 );
    memset( p, 0, sizeof(Part_Man_t) );
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
void Part_ManStop( Part_Man_t * p )
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
char * Part_ManFetch( Part_Man_t * p, int nSize )
{
    int Type, nSizeReal;
    char * pMemory;
    assert( nSize > 0 );
    Type = Part_SizeType( nSize, p->nStepSize );
    Vec_PtrFillExtra( p->vFree, Type + 1, NULL );
    if ( (pMemory = (char *)Vec_PtrEntry( p->vFree, Type )) )
    {
        Vec_PtrWriteEntry( p->vFree, Type, Part_OneNext(pMemory) );
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
void Part_ManRecycle( Part_Man_t * p, char * pMemory, int nSize )
{
    int Type;
    Type = Part_SizeType( nSize, p->nStepSize );
    Vec_PtrFillExtra( p->vFree, Type + 1, NULL );
    Part_OneSetNext( pMemory, (char *)Vec_PtrEntry(p->vFree, Type) );
    Vec_PtrWriteEntry( p->vFree, Type, pMemory );
}

/**Function*************************************************************

  Synopsis    [Fetches the memory entry of the given size.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Part_One_t * Part_ManFetchEntry( Part_Man_t * p, int nWords, int nRefs )
{
    Part_One_t * pPart;
    pPart = (Part_One_t *)Part_ManFetch( p, sizeof(Part_One_t) + sizeof(int) * nWords );
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
static inline void Part_ManRecycleEntry( Part_Man_t * p, Part_One_t * pEntry )
{
    assert( pEntry->nOuts <= pEntry->nOutsAlloc );
    assert( pEntry->nOuts >= pEntry->nOutsAlloc/2 );
    Part_ManRecycle( p, (char *)pEntry, sizeof(Part_One_t) + sizeof(int) * pEntry->nOutsAlloc );
}

/**Function*************************************************************

  Synopsis    [Merges two entries.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Part_One_t * Part_ManMergeEntry( Part_Man_t * pMan, Part_One_t * p1, Part_One_t * p2, int nRefs )
{
    Part_One_t * p = Part_ManFetchEntry( pMan, p1->nOuts + p2->nOuts, nRefs );
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
Vec_Int_t * Part_ManTransferEntry( Part_One_t * p )
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

  Description [Returns the array of integer arrays containing indices
  of the primary inputs for each primary output.]
               
  SideEffects [Adds the integer PO number at end of each array.]

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Aig_ManSupports( Aig_Man_t * pMan )
{
    Vec_Ptr_t * vSupports;
    Vec_Int_t * vSupp;
    Part_Man_t * p;
    Part_One_t * pPart0, * pPart1;
    Aig_Obj_t * pObj;
    int i, Counter = 0;
    // set the number of PIs/POs
    Aig_ManForEachCi( pMan, pObj, i )
        pObj->pNext = (Aig_Obj_t *)(long)i;
    Aig_ManForEachCo( pMan, pObj, i )
        pObj->pNext = (Aig_Obj_t *)(long)i;
    // start the support computation manager
    p = Part_ManStart( 1 << 20, 1 << 6 );
    // consider objects in the topological order
    vSupports = Vec_PtrAlloc( Aig_ManCoNum(pMan) );
    Aig_ManCleanData(pMan);
    Aig_ManForEachObj( pMan, pObj, i )
    {
        if ( Aig_ObjIsNode(pObj) )
        {
            pPart0 = (Part_One_t *)Aig_ObjFanin0(pObj)->pData;
            pPart1 = (Part_One_t *)Aig_ObjFanin1(pObj)->pData;
            pObj->pData = Part_ManMergeEntry( p, pPart0, pPart1, pObj->nRefs );
            assert( pPart0->nRefs > 0 );
            if ( --pPart0->nRefs == 0 )
                Part_ManRecycleEntry( p, pPart0 );
            assert( pPart1->nRefs > 0 );
            if ( --pPart1->nRefs == 0 )
                Part_ManRecycleEntry( p, pPart1 );
            if ( ((Part_One_t *)pObj->pData)->nOuts <= 16 )
                Counter++;
            continue;
        }
        if ( Aig_ObjIsCo(pObj) )
        {
            pPart0 = (Part_One_t *)Aig_ObjFanin0(pObj)->pData;
            vSupp = Part_ManTransferEntry(pPart0);
            Vec_IntPush( vSupp, (int)(long)pObj->pNext );
            Vec_PtrPush( vSupports, vSupp );
            assert( pPart0->nRefs > 0 );
            if ( --pPart0->nRefs == 0 )
                Part_ManRecycleEntry( p, pPart0 );
            continue;
        }
        if ( Aig_ObjIsCi(pObj) )
        {
            if ( pObj->nRefs )
            {
                pPart0 = Part_ManFetchEntry( p, 1, pObj->nRefs );
                pPart0->pOuts[ pPart0->nOuts++ ] = (int)(long)pObj->pNext;
                pObj->pData = pPart0;
            }
            continue;
        }
        if ( Aig_ObjIsConst1(pObj) )
        {
            if ( pObj->nRefs )
                pObj->pData = Part_ManFetchEntry( p, 0, pObj->nRefs );
            continue;
        }
        assert( 0 );
    }
//printf( "Memory usage = %d MB.\n", Vec_PtrSize(p->vMemory) * p->nChunkSize / (1<<20) );
    Part_ManStop( p );
    // sort supports by size
    Vec_VecSort( (Vec_Vec_t *)vSupports, 1 );
    // clear the number of PIs/POs
    Aig_ManForEachCi( pMan, pObj, i )
        pObj->pNext = NULL;
    Aig_ManForEachCo( pMan, pObj, i )
        pObj->pNext = NULL;
/*
    Aig_ManForEachCo( pMan, pObj, i )
        printf( "%d ", Vec_IntSize( Vec_VecEntryInt(vSupports, i) ) );
    printf( "\n" );
*/
//    printf( "%d \n", Counter );
    return vSupports;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ManSupportsTest( Aig_Man_t * pMan )
{
    Vec_Ptr_t * vSupps;
    vSupps = Aig_ManSupports( pMan );
    Vec_VecFree( (Vec_Vec_t *)vSupps );
}

/**Function*************************************************************

  Synopsis    [Computes the set of outputs for each input.]

  Description [Returns the array of integer arrays containing indices
  of the primary outputsf for each primary input.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Aig_ManSupportsInverse( Aig_Man_t * p )
{
    Vec_Ptr_t * vSupps, * vSuppsInv;
    Vec_Int_t * vSupp;
    int i, k, iIn, iOut;
    // get structural supports for each output
    vSupps = Aig_ManSupports( p );
    // start the inverse supports
    vSuppsInv = Vec_PtrAlloc( Aig_ManCiNum(p) );
    for ( i = 0; i < Aig_ManCiNum(p); i++ )
        Vec_PtrPush( vSuppsInv, Vec_IntAlloc(8) );
    // transforms the supports into the inverse supports
    Vec_PtrForEachEntry( Vec_Int_t *, vSupps, vSupp, i )
    {
        iOut = Vec_IntPop( vSupp );
        Vec_IntForEachEntry( vSupp, iIn, k )
            Vec_IntPush( (Vec_Int_t *)Vec_PtrEntry(vSuppsInv, iIn), iOut );
    }
    Vec_VecFree( (Vec_Vec_t *)vSupps );
    return vSuppsInv;
}

/**Function*************************************************************

  Synopsis    [Returns the register dependency matrix.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Aig_ManSupportsRegisters( Aig_Man_t * p )
{
    Vec_Ptr_t * vSupports, * vMatrix;
    Vec_Int_t * vSupp;
    int iOut, iIn, k, m, i;
    // get structural supports for each output
    vSupports = Aig_ManSupports( p );
    // transforms the supports into the latch dependency matrix
    vMatrix = Vec_PtrStart( Aig_ManRegNum(p) );
    Vec_PtrForEachEntry( Vec_Int_t *, vSupports, vSupp, i )
    {
        // skip true POs
        iOut = Vec_IntPop( vSupp );
        iOut -= Aig_ManCoNum(p) - Aig_ManRegNum(p);
        if ( iOut < 0 )
        {
            Vec_IntFree( vSupp );
            continue;
        }
        // remove PIs
        m = 0;
        Vec_IntForEachEntry( vSupp, iIn, k )
        {
            iIn -= Aig_ManCiNum(p) - Aig_ManRegNum(p);
            if ( iIn < 0 )
                continue;
            assert( iIn < Aig_ManRegNum(p) );
            Vec_IntWriteEntry( vSupp, m++, iIn );
        }
        Vec_IntShrink( vSupp, m );
        // store support in the matrix
        assert( iOut < Aig_ManRegNum(p) );
        Vec_PtrWriteEntry( vMatrix, iOut, vSupp );
    }
    Vec_PtrFree( vSupports );
    // check that all supports are used
    Vec_PtrForEachEntry( Vec_Int_t *, vMatrix, vSupp, i )
        assert( vSupp != NULL );
    return vMatrix;
}

/**Function*************************************************************

  Synopsis    [Start char-bases support representation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned * Aig_ManSuppCharStart( Vec_Int_t * vOne, int nPis )
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

  Synopsis    [Add to char-bases support representation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ManSuppCharAdd( unsigned * pBuffer, Vec_Int_t * vOne, int nPis )
{
    int i, Entry;
    Vec_IntForEachEntry( vOne, Entry, i )
    {
        assert( Entry < nPis );
        Abc_InfoSetBit( pBuffer, Entry );
    }
}

/**Function*************************************************************

  Synopsis    [Find the common variables using char-bases support representation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aig_ManSuppCharCommon( unsigned * pBuffer, Vec_Int_t * vOne )
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
int Aig_ManPartitionSmartFindPart( Vec_Ptr_t * vPartSuppsAll, Vec_Ptr_t * vPartsAll, Vec_Ptr_t * vPartSuppsBit, int nSuppSizeLimit, Vec_Int_t * vOne )
{
    Vec_Int_t * vPartSupp;//, * vPart;
    int Attract, Repulse, Value, ValueBest;
    int i, nCommon, iBest;
    iBest = -1;
    ValueBest = 0;
    Vec_PtrForEachEntry( Vec_Int_t *, vPartSuppsAll, vPartSupp, i )
    {
//        vPart = Vec_PtrEntry( vPartsAll, i );
//        if ( nSuppSizeLimit > 0 && Vec_IntSize(vPart) >= nSuppSizeLimit )
//            continue;
//        nCommon = Vec_IntTwoCountCommon( vPartSupp, vOne );
        nCommon = Aig_ManSuppCharCommon( (unsigned *)Vec_PtrEntry(vPartSuppsBit, i), vOne );
        if ( nCommon == 0 )
            continue;
        if ( nCommon == Vec_IntSize(vOne) )
            return i;
        // skip partitions whose size exceeds the limit
        if ( nSuppSizeLimit > 0 && Vec_IntSize(vPartSupp) >= 2 * nSuppSizeLimit )
            continue;
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
void Aig_ManPartitionPrint( Aig_Man_t * p, Vec_Ptr_t * vPartsAll, Vec_Ptr_t * vPartSuppsAll )
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
    assert( Counter == Aig_ManCoNum(p) );
//    printf( "\nTotal = %d. Outputs = %d.\n", Counter, Aig_ManCoNum(p) );
}

/**Function*************************************************************

  Synopsis    [Perform the smart partitioning.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ManPartitionCompact( Vec_Ptr_t * vPartsAll, Vec_Ptr_t * vPartSuppsAll, int nSuppSizeLimit )
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

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Aig_ManPartitionSmart( Aig_Man_t * p, int nSuppSizeLimit, int fVerbose, Vec_Ptr_t ** pvPartSupps )
{
    Vec_Ptr_t * vPartSuppsBit;
    Vec_Ptr_t * vSupports, * vPartsAll, * vPartsAll2, * vPartSuppsAll;//, * vPartPtr;
    Vec_Int_t * vOne, * vPart, * vPartSupp, * vTemp;
    int i, iPart, iOut;
    abctime clk;

    // compute the supports for all outputs
clk = Abc_Clock();
    vSupports = Aig_ManSupports( p );
if ( fVerbose )
{
ABC_PRT( "Supps", Abc_Clock() - clk );
}
    // start char-based support representation
    vPartSuppsBit = Vec_PtrAlloc( 1000 );

    // create partitions
clk = Abc_Clock();
    vPartsAll = Vec_PtrAlloc( 256 );
    vPartSuppsAll = Vec_PtrAlloc( 256 );
    Vec_PtrForEachEntry( Vec_Int_t *, vSupports, vOne, i )
    {
        // get the output number
        iOut = Vec_IntPop(vOne);
        // find closely matching part
        iPart = Aig_ManPartitionSmartFindPart( vPartSuppsAll, vPartsAll, vPartSuppsBit, nSuppSizeLimit, vOne );
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

            Vec_PtrPush( vPartSuppsBit, Aig_ManSuppCharStart(vOne, Aig_ManCiNum(p)) );
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

            Aig_ManSuppCharAdd( (unsigned *)Vec_PtrEntry(vPartSuppsBit, iPart), vOne, Aig_ManCiNum(p) );
        }
    }

    // stop char-based support representation
    Vec_PtrForEachEntry( Vec_Int_t *, vPartSuppsBit, vTemp, i )
        ABC_FREE( vTemp );
    Vec_PtrFree( vPartSuppsBit );

//printf( "\n" );
if ( fVerbose )
{
ABC_PRT( "Parts", Abc_Clock() - clk );
}

clk = Abc_Clock();
    // reorder partitions in the decreasing order of support sizes
    // remember partition number in each partition support
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
//    Aig_ManPartitionPrint( p, vPartsAll, vPartSuppsAll );
    Aig_ManPartitionCompact( vPartsAll, vPartSuppsAll, nSuppSizeLimit );
    if ( fVerbose )
//    Aig_ManPartitionPrint( p, vPartsAll, vPartSuppsAll );
    printf( "Created %d partitions.\n", Vec_PtrSize(vPartsAll) );

if ( fVerbose )
{
//ABC_PRT( "Comps", Abc_Clock() - clk );
}

    // cleanup
    Vec_VecFree( (Vec_Vec_t *)vSupports );
    if ( pvPartSupps == NULL )
        Vec_VecFree( (Vec_Vec_t *)vPartSuppsAll );
    else
        *pvPartSupps = vPartSuppsAll;
/*
    // converts from intergers to nodes
    Vec_PtrForEachEntry( Vec_Int_t *, vPartsAll, vPart, iPart )
    {
        vPartPtr = Vec_PtrAlloc( Vec_IntSize(vPart) );
        Vec_IntForEachEntry( vPart, iOut, i )
            Vec_PtrPush( vPartPtr, Aig_ManCo(p, iOut) );
        Vec_IntFree( vPart );
        Vec_PtrWriteEntry( vPartsAll, iPart, vPartPtr );
    }
*/
    return vPartsAll;
}

/**Function*************************************************************

  Synopsis    [Perform the smart partitioning.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Aig_ManPartitionSmartRegisters( Aig_Man_t * pAig, int nSuppSizeLimit, int fVerbose )
{
    Vec_Ptr_t * vPartSuppsBit;
    Vec_Ptr_t * vSupports, * vPartsAll, * vPartsAll2, * vPartSuppsAll;
    Vec_Int_t * vOne, * vPart, * vPartSupp, * vTemp;
    int i, iPart, iOut;
    abctime clk;

    // add output number to each
clk = Abc_Clock();
    vSupports = Aig_ManSupportsRegisters( pAig );
    assert( Vec_PtrSize(vSupports) == Aig_ManRegNum(pAig) );
    Vec_PtrForEachEntry( Vec_Int_t *, vSupports, vOne, i )
        Vec_IntPush( vOne, i );
if ( fVerbose )
{
ABC_PRT( "Supps", Abc_Clock() - clk );
}

    // start char-based support representation
    vPartSuppsBit = Vec_PtrAlloc( 1000 );

    // create partitions
clk = Abc_Clock();
    vPartsAll = Vec_PtrAlloc( 256 );
    vPartSuppsAll = Vec_PtrAlloc( 256 );
    Vec_PtrForEachEntry( Vec_Int_t *, vSupports, vOne, i )
    {
        // get the output number
        iOut = Vec_IntPop(vOne);
        // find closely matching part
        iPart = Aig_ManPartitionSmartFindPart( vPartSuppsAll, vPartsAll, vPartSuppsBit, nSuppSizeLimit, vOne );
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

            Vec_PtrPush( vPartSuppsBit, Aig_ManSuppCharStart(vOne, Vec_PtrSize(vSupports)) );
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

            Aig_ManSuppCharAdd( (unsigned *)Vec_PtrEntry(vPartSuppsBit, iPart), vOne, Vec_PtrSize(vSupports) );
        }
    }

    // stop char-based support representation
    Vec_PtrForEachEntry( Vec_Int_t *, vPartSuppsBit, vTemp, i )
        ABC_FREE( vTemp );
    Vec_PtrFree( vPartSuppsBit );

//printf( "\n" );
if ( fVerbose )
{
ABC_PRT( "Parts", Abc_Clock() - clk );
}

clk = Abc_Clock();
    // reorder partitions in the decreasing order of support sizes
    // remember partition number in each partition support
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
//    Aig_ManPartitionPrint( p, vPartsAll, vPartSuppsAll );
    Aig_ManPartitionCompact( vPartsAll, vPartSuppsAll, nSuppSizeLimit );
    if ( fVerbose )
//    Aig_ManPartitionPrint( p, vPartsAll, vPartSuppsAll );
    printf( "Created %d partitions.\n", Vec_PtrSize(vPartsAll) );

if ( fVerbose )
{
//ABC_PRT( "Comps", Abc_Clock() - clk );
}

    // cleanup
    Vec_VecFree( (Vec_Vec_t *)vSupports );
//    if ( pvPartSupps == NULL )
        Vec_VecFree( (Vec_Vec_t *)vPartSuppsAll );
//    else
//        *pvPartSupps = vPartSuppsAll;

/*
    // converts from intergers to nodes
    Vec_PtrForEachEntry( Vec_Int_t *, vPartsAll, vPart, iPart )
    {
        vPartPtr = Vec_PtrAlloc( Vec_IntSize(vPart) );
        Vec_IntForEachEntry( vPart, iOut, i )
            Vec_PtrPush( vPartPtr, Aig_ManCo(p, iOut) );
        Vec_IntFree( vPart );
        Vec_PtrWriteEntry( vPartsAll, iPart, vPartPtr );
    }
*/
    return vPartsAll;
}

/**Function*************************************************************

  Synopsis    [Perform the naive partitioning.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Aig_ManPartitionNaive( Aig_Man_t * p, int nPartSize )
{
    Vec_Ptr_t * vParts;
    Aig_Obj_t * pObj;
    int nParts, i;
    nParts = (Aig_ManCoNum(p) / nPartSize) + ((Aig_ManCoNum(p) % nPartSize) > 0);
    vParts = (Vec_Ptr_t *)Vec_VecStart( nParts );
    Aig_ManForEachCo( p, pObj, i )
        Vec_IntPush( (Vec_Int_t *)Vec_PtrEntry(vParts, i / nPartSize), i );
    return vParts;
}



/**Function*************************************************************

  Synopsis    [Adds internal nodes in the topological order.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Obj_t * Aig_ManDupPart_rec( Aig_Man_t * pNew, Aig_Man_t * pOld, Aig_Obj_t * pObj, Vec_Int_t * vSuppMap )
{
    assert( !Aig_IsComplement(pObj) );
    if ( Aig_ObjIsTravIdCurrent(pOld, pObj) )
        return (Aig_Obj_t *)pObj->pData;
    Aig_ObjSetTravIdCurrent(pOld, pObj);
    if ( Aig_ObjIsCi(pObj) )
    {
        assert( Vec_IntSize(vSuppMap) == Aig_ManCiNum(pNew) );
        Vec_IntPush( vSuppMap, (int)(long)pObj->pNext );
        return (Aig_Obj_t *)(pObj->pData = Aig_ObjCreateCi(pNew));
    }
    assert( Aig_ObjIsNode(pObj) );
    Aig_ManDupPart_rec( pNew, pOld, Aig_ObjFanin0(pObj), vSuppMap );
    Aig_ManDupPart_rec( pNew, pOld, Aig_ObjFanin1(pObj), vSuppMap );
    return (Aig_Obj_t *)(pObj->pData = Aig_And( pNew, Aig_ObjChild0Copy(pObj), Aig_ObjChild1Copy(pObj) ));
}

/**Function*************************************************************

  Synopsis    [Adds internal nodes in the topological order.]

  Description [Returns the array of new outputs.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Aig_ManDupPart( Aig_Man_t * pNew, Aig_Man_t * pOld, Vec_Int_t * vPart, Vec_Int_t * vSuppMap, int fInverse )
{
    Vec_Ptr_t * vOutsTotal;
    Aig_Obj_t * pObj;
    int Entry, i;
    // create the PIs
    Aig_ManIncrementTravId( pOld );
    Aig_ManConst1(pOld)->pData = Aig_ManConst1(pNew);
    Aig_ObjSetTravIdCurrent( pOld, Aig_ManConst1(pOld) );
    if ( !fInverse )
    {
        Vec_IntForEachEntry( vSuppMap, Entry, i )
        {
            pObj = Aig_ManCi( pOld, Entry );
            pObj->pData = Aig_ManCi( pNew, i );
            Aig_ObjSetTravIdCurrent( pOld, pObj );
        }
    }
    else
    {
        Vec_IntForEachEntry( vSuppMap, Entry, i )
        {
            pObj = Aig_ManCi( pOld, i );
            pObj->pData = Aig_ManCi( pNew, Entry );
            Aig_ObjSetTravIdCurrent( pOld, pObj );
        }
        vSuppMap = NULL; // should not be useful
    }
    // create the internal nodes
    vOutsTotal = Vec_PtrAlloc( Vec_IntSize(vPart) );
    if ( !fInverse )
    {
        Vec_IntForEachEntry( vPart, Entry, i )
        {
            pObj = Aig_ManCo( pOld, Entry );
            Aig_ManDupPart_rec( pNew, pOld, Aig_ObjFanin0(pObj), vSuppMap );
            Vec_PtrPush( vOutsTotal, Aig_ObjChild0Copy(pObj) );
        }
    }
    else
    {
        Aig_ManForEachObj( pOld, pObj, i )
        {
            if ( Aig_ObjIsCo(pObj) )
            {
                Aig_ManDupPart_rec( pNew, pOld, Aig_ObjFanin0(pObj), vSuppMap );
                Vec_PtrPush( vOutsTotal, Aig_ObjChild0Copy(pObj) );
            }
            else if ( Aig_ObjIsNode(pObj) && pObj->nRefs == 0 )
                Aig_ManDupPart_rec( pNew, pOld, pObj, vSuppMap );
            
        }
    }
    return vOutsTotal; 
}

/**Function*************************************************************

  Synopsis    [Adds internal nodes in the topological order.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ManDupPartAll_rec( Aig_Man_t * pNew, Aig_Man_t * pOld, Aig_Obj_t * pObj )
{
    Aig_Obj_t * pObjNew;
    assert( !Aig_IsComplement(pObj) );
    if ( Aig_ObjIsTravIdCurrent(pOld, pObj) )
        return;
    Aig_ObjSetTravIdCurrent(pOld, pObj);
    if ( Aig_ObjIsCi(pObj) )
        pObjNew = Aig_ObjCreateCi(pNew);
    else if ( Aig_ObjIsCo(pObj) )
    {
        Aig_ManDupPartAll_rec( pNew, pOld, Aig_ObjFanin0(pObj) );
        pObjNew = Aig_ObjCreateCo( pNew, Aig_ObjChild0Copy(pObj) );
    }
    else
    {
        assert( Aig_ObjIsNode(pObj) );
        Aig_ManDupPartAll_rec( pNew, pOld, Aig_ObjFanin0(pObj) );
        Aig_ManDupPartAll_rec( pNew, pOld, Aig_ObjFanin1(pObj) );
        pObjNew = Aig_And( pNew, Aig_ObjChild0Copy(pObj), Aig_ObjChild1Copy(pObj) );
    }
    pObj->pData = pObjNew;
    pObjNew->pData = pObj;
}

/**Function*************************************************************

  Synopsis    [Adds internal nodes in the topological order.]

  Description [Returns the array of new outputs.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Aig_ManDupPartAll( Aig_Man_t * pOld, Vec_Int_t * vPart )
{
    Aig_Man_t * pNew;
    Aig_Obj_t * pObj, * pObjNew;
    int i, Entry;
    Aig_ManIncrementTravId( pOld );
    pNew = Aig_ManStart( 5000 );
    // map constant nodes
    pObj = Aig_ManConst1(pOld);
    pObjNew = Aig_ManConst1(pNew);
    pObj->pData = pObjNew;
    pObjNew->pData = pObj;
    Aig_ObjSetTravIdCurrent(pOld, pObj);
    // map all other nodes
    Vec_IntForEachEntry( vPart, Entry, i )
    {
        pObj = Aig_ManCo( pOld, Entry );
        Aig_ManDupPartAll_rec( pNew, pOld, pObj );
    }
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Collects internal nodes in the DFS order.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ManSupportNodes_rec( Aig_Man_t * p, Aig_Obj_t * pObj, Vec_Int_t * vSupport )
{
    if ( Aig_ObjIsTravIdCurrent(p, pObj) )
        return;
    Aig_ObjSetTravIdCurrent(p, pObj);
    if ( Aig_ObjIsCi(pObj) )
    {
        Vec_IntPush( vSupport, Aig_ObjCioId(pObj) );
        return;
    }
    Aig_ManSupportNodes_rec( p, Aig_ObjFanin0(pObj), vSupport );
    Aig_ManSupportNodes_rec( p, Aig_ObjFanin1(pObj), vSupport );
}

/**Function*************************************************************

  Synopsis    [Collects internal nodes and PIs in the DFS order.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Aig_ManSupportNodes( Aig_Man_t * p, Vec_Ptr_t * vParts )
{
    Vec_Ptr_t * vPartSupps;
    Vec_Int_t * vPart, * vSupport;
    int i, k, iOut;
    Aig_ManSetCioIds( p );
    vPartSupps = Vec_PtrAlloc( Vec_PtrSize(vParts) );
    Vec_PtrForEachEntry( Vec_Int_t *, vParts, vPart, i )
    {
        vSupport = Vec_IntAlloc( 100 );
        Aig_ManIncrementTravId( p );
        Aig_ObjSetTravIdCurrent( p, Aig_ManConst1(p) );
        Vec_IntForEachEntry( vPart, iOut, k )
            Aig_ManSupportNodes_rec( p, Aig_ObjFanin0(Aig_ManCo(p, iOut)), vSupport );
//        Vec_IntSort( vSupport, 0 );
        Vec_PtrPush( vPartSupps, vSupport );
    }
    Aig_ManCleanCioIds( p );
    return vPartSupps;
}

/**Function*************************************************************

  Synopsis    [Create partitioned miter of the two AIGs.]

  Description [Assumes that each output in the second AIG cannot have 
  more supp vars than the same output in the first AIG.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Aig_ManMiterPartitioned( Aig_Man_t * p1, Aig_Man_t * p2, int nPartSize, int fSmart )
{
    Aig_Man_t * pNew;
    Aig_Obj_t * pMiter;
    Vec_Ptr_t * vMiters, * vNodes1, * vNodes2;
    Vec_Ptr_t * vParts, * vPartSupps;
    Vec_Int_t * vPart, * vPartSupp;
    int i, k;
    // partition the first manager
    if ( fSmart )
        vParts = Aig_ManPartitionSmart( p1, nPartSize, 0, &vPartSupps );
    else
    {
        vParts = Aig_ManPartitionNaive( p1, nPartSize );
        vPartSupps = Aig_ManSupportNodes( p1, vParts );
    }
    // derive miters
    vMiters = Vec_PtrAlloc( Vec_PtrSize(vParts) );
    for ( i = 0; i < Vec_PtrSize(vParts); i++ )
    {
        // get partition and its support
        vPart     = (Vec_Int_t *)Vec_PtrEntry( vParts, i );
        vPartSupp = (Vec_Int_t *)Vec_PtrEntry( vPartSupps, i );
        // create the new miter
        pNew = Aig_ManStart( 1000 );
        // create the PIs
        for ( k = 0; k < Vec_IntSize(vPartSupp); k++ )
            Aig_ObjCreateCi( pNew );
        // copy the components
        vNodes1 = Aig_ManDupPart( pNew, p1, vPart, vPartSupp, 0 );
        vNodes2 = Aig_ManDupPart( pNew, p2, vPart, vPartSupp, 0 );
        // create the miter
        pMiter = Aig_MiterTwo( pNew, vNodes1, vNodes2 );
        Vec_PtrFree( vNodes1 );
        Vec_PtrFree( vNodes2 );
        // create the output
        Aig_ObjCreateCo( pNew, pMiter );
        // clean up
        Aig_ManCleanup( pNew );
        Vec_PtrPush( vMiters, pNew );
    }
    Vec_VecFree( (Vec_Vec_t *)vParts );
    Vec_VecFree( (Vec_Vec_t *)vPartSupps );
    return vMiters;
}

/**Function*************************************************************

  Synopsis    [Performs partitioned choice computation.]

  Description [Assumes that each output in the second AIG cannot have 
  more supp vars than the same output in the first AIG.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Aig_ManChoicePartitioned( Vec_Ptr_t * vAigs, int nPartSize, int nConfMax, int nLevelMax, int fVerbose )
{
//    extern int Cmd_CommandExecute( void * pAbc, char * sCommand );
//    extern void * Abc_FrameGetGlobalFrame();
//    extern Aig_Man_t * Fra_FraigChoice( Aig_Man_t * pManAig, int nConfMax, int nLevelMax );

    Vec_Ptr_t * vPios;
    Vec_Ptr_t * vOutsTotal, * vOuts;
    Aig_Man_t * pAigTotal, * pAigPart, * pAig, * pTemp;
    Vec_Int_t * vPart, * vPartSupp;
    Vec_Ptr_t * vParts;
    Aig_Obj_t * pObj;
    void ** ppData;
    int i, k, m, nIdMax;
    assert( Vec_PtrSize(vAigs) > 1 );

    // compute the total number of IDs
    nIdMax = 0;
    Vec_PtrForEachEntry( Aig_Man_t *, vAigs, pAig, i )
        nIdMax += Aig_ManObjNumMax(pAig);

    // partition the first AIG in the array
    pAig = (Aig_Man_t *)Vec_PtrEntry( vAigs, 0 );
    vParts = Aig_ManPartitionSmart( pAig, nPartSize, 0, NULL );

    // start the total fraiged AIG
    pAigTotal = Aig_ManStartFrom( pAig );
    Aig_ManReprStart( pAigTotal, nIdMax );
    vOutsTotal = Vec_PtrStart( Aig_ManCoNum(pAig) );

    // set the PI numbers
    Vec_PtrForEachEntry( Aig_Man_t *, vAigs, pAig, i )
        Aig_ManForEachCi( pAig, pObj, k )
            pObj->pNext = (Aig_Obj_t *)(long)k;

//    Cmd_CommandExecute( Abc_FrameGetGlobalFrame(), "unset progressbar" );

    // create the total fraiged AIG
    vPartSupp = Vec_IntAlloc( 100 ); // maps part PI num into total PI num
    Vec_PtrForEachEntry( Vec_Int_t *, vParts, vPart, i )
    {
        // derive the partition AIG
        pAigPart = Aig_ManStart( 5000 );
//        pAigPart->pName = Extra_UtilStrsav( pAigPart->pName );
        Vec_IntClear( vPartSupp );
        Vec_PtrForEachEntry( Aig_Man_t *, vAigs, pAig, k )
        {
            vOuts = Aig_ManDupPart( pAigPart, pAig, vPart, vPartSupp, 0 );
            if ( k == 0 )
            {
                Vec_PtrForEachEntry( Aig_Obj_t *, vOuts, pObj, m )
                    Aig_ObjCreateCo( pAigPart, pObj );
            }
            Vec_PtrFree( vOuts );
        }
        // derive the total AIG from the partitioned AIG
        vOuts = Aig_ManDupPart( pAigTotal, pAigPart, vPart, vPartSupp, 1 );
        // add to the outputs
        Vec_PtrForEachEntry( Aig_Obj_t *, vOuts, pObj, k )
        {
            assert( Vec_PtrEntry( vOutsTotal, Vec_IntEntry(vPart,k) ) == NULL );
            Vec_PtrWriteEntry( vOutsTotal, Vec_IntEntry(vPart,k), pObj );
        }
        Vec_PtrFree( vOuts );
        // store contents of pData pointers
        ppData = ABC_ALLOC( void *, Aig_ManObjNumMax(pAigPart) );
        Aig_ManForEachObj( pAigPart, pObj, k )
            ppData[k] = pObj->pData;
        // report the process
        if ( fVerbose )
        printf( "Part %4d  (out of %4d)  PI = %5d. PO = %5d. And = %6d. Lev = %4d.\r", 
            i+1, Vec_PtrSize(vParts), Aig_ManCiNum(pAigPart), Aig_ManCoNum(pAigPart), 
            Aig_ManNodeNum(pAigPart), Aig_ManLevelNum(pAigPart) );
        // compute equivalence classes (to be stored in pNew->pReprs)
        pAig = Fra_FraigChoice( pAigPart, nConfMax, nLevelMax );
        Aig_ManStop( pAig );
        // reset the pData pointers
        Aig_ManForEachObj( pAigPart, pObj, k )
            pObj->pData = ppData[k];
        ABC_FREE( ppData );
        // transfer representatives to the total AIG
        if ( pAigPart->pReprs )
            Aig_ManTransferRepr( pAigTotal, pAigPart );
        Aig_ManStop( pAigPart );
    }
    if ( fVerbose )
    printf( "                                                                                          \r" );
    Vec_VecFree( (Vec_Vec_t *)vParts );
    Vec_IntFree( vPartSupp );

//    Cmd_CommandExecute( Abc_FrameGetGlobalFrame(), "set progressbar" );

    // clear the PI numbers
    Vec_PtrForEachEntry( Aig_Man_t *, vAigs, pAig, i )
        Aig_ManForEachCi( pAig, pObj, k )
            pObj->pNext = NULL;

    // add the outputs in the same order
    Vec_PtrForEachEntry( Aig_Obj_t *, vOutsTotal, pObj, i )
        Aig_ObjCreateCo( pAigTotal, pObj );
    Vec_PtrFree( vOutsTotal );

    // derive the result of choicing
    pAig = Aig_ManRehash( pAigTotal );
    // create the equivalent nodes lists
    Aig_ManMarkValidChoices( pAig );
    // reconstruct the network
    vPios = Aig_ManOrderPios( pAig, (Aig_Man_t *)Vec_PtrEntry(vAigs,0) );
    pAig = Aig_ManDupDfsGuided( pTemp = pAig, vPios );
    Aig_ManStop( pTemp );
    Vec_PtrFree( vPios );
    // duplicate the timing manager
    pTemp = (Aig_Man_t *)Vec_PtrEntry( vAigs, 0 );
    if ( pTemp->pManTime )
        pAig->pManTime = Tim_ManDup( (Tim_Man_t *)pTemp->pManTime, 0 );
    // reset levels
    Aig_ManChoiceLevel( pAig );
    return pAig;
}

/**Function*************************************************************

  Synopsis    [Performs partitioned choice computation.]

  Description [Assumes that each output in the second AIG cannot have 
  more supp vars than the same output in the first AIG.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Aig_ManFraigPartitioned( Aig_Man_t * pAig, int nPartSize, int nConfMax, int nLevelMax, int fVerbose )
{
//    extern Aig_Man_t * Fra_FraigChoice( Aig_Man_t * pManAig, int nConfMax, int nLevelMax );

    Aig_Man_t * pAigPart, * pAigTemp;
    Vec_Int_t * vPart;
    Vec_Ptr_t * vParts;
    Aig_Obj_t * pObj;
    void ** ppData;
    int i, k;

    // partition the outputs of the AIG
    vParts = Aig_ManPartitionNaive( pAig, nPartSize );

    // start the equivalence classes
    Aig_ManReprStart( pAig, Aig_ManObjNumMax(pAig) );

    // set the PI numbers
    Aig_ManSetCioIds( pAig );

    // create the total fraiged AIG
    Vec_PtrForEachEntry( Vec_Int_t *, vParts, vPart, i )
    {
        // derive the partition AIG
        pAigPart = Aig_ManDupPartAll( pAig, vPart );
        // store contents of pData pointers
        ppData = ABC_ALLOC( void *, Aig_ManObjNumMax(pAigPart) );
        Aig_ManForEachObj( pAigPart, pObj, k )
            ppData[k] = pObj->pData;
        // report the process
        if ( fVerbose )
        printf( "Part %4d  (out of %4d)  PI = %5d. PO = %5d. And = %6d. Lev = %4d.\r", 
            i+1, Vec_PtrSize(vParts), Aig_ManCiNum(pAigPart), Aig_ManCoNum(pAigPart), 
            Aig_ManNodeNum(pAigPart), Aig_ManLevelNum(pAigPart) );
        // compute equivalence classes (to be stored in pNew->pReprs)
        pAigTemp = Fra_FraigChoice( pAigPart, nConfMax, nLevelMax );
        Aig_ManStop( pAigTemp );
        // reset the pData pointers
        Aig_ManForEachObj( pAigPart, pObj, k )
            pObj->pData = ppData[k];
        ABC_FREE( ppData );
        // transfer representatives to the total AIG
        if ( pAigPart->pReprs )
            Aig_ManTransferRepr( pAig, pAigPart );
        Aig_ManStop( pAigPart );
    }
    if ( fVerbose )
    printf( "                                                                                          \r" );
    Vec_VecFree( (Vec_Vec_t *)vParts );

    // clear the PI numbers
    Aig_ManCleanCioIds( pAig );

    // derive the result of choicing
    return Aig_ManDupRepr( pAig, 0 );
}




/**Function*************************************************************

  Synopsis    [Set the representative.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Aig_ObjSetRepr_( Aig_Man_t * p, Aig_Obj_t * pNode1, Aig_Obj_t * pNode2 )
{
    assert( p->pReprs != NULL );
    assert( !Aig_IsComplement(pNode1) );
    assert( !Aig_IsComplement(pNode2) );
    assert( pNode1->Id < p->nReprsAlloc );
    assert( pNode2->Id < p->nReprsAlloc );
    if ( pNode1 == pNode2 )
        return;
    if ( pNode1->Id < pNode2->Id )
        p->pReprs[pNode2->Id] = pNode1;
    else
        p->pReprs[pNode1->Id] = pNode2;
}

/**Function*************************************************************

  Synopsis    [Constructively accumulates choices.]

  Description [pNew is a new AIG with choices under construction.
  pPrev is the AIG preceding pThis in the order of deriving choices.
  pThis is the current AIG to be added to pNew while creating choices.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ManChoiceConstructiveOne( Aig_Man_t * pNew, Aig_Man_t * pPrev, Aig_Man_t * pThis )
{
    Aig_Obj_t * pObj, * pObjNew;
    int i;
    assert( Aig_ManCiNum(pNew) == Aig_ManCiNum(pPrev) );
    assert( Aig_ManCiNum(pNew) == Aig_ManCiNum(pThis) );
    assert( Aig_ManCoNum(pNew) == Aig_ManCoNum(pPrev) );
    assert( Aig_ManCoNum(pNew) == Aig_ManCoNum(pThis) );
    // make sure the nodes of pPrev point to pNew
    Aig_ManForEachObj( pNew, pObj, i )
        pObj->fMarkB = 1;
    Aig_ManForEachObj( pPrev, pObj, i )
        assert( Aig_Regular((Aig_Obj_t *)pObj->pData)->fMarkB );
    Aig_ManForEachObj( pNew, pObj, i )
        pObj->fMarkB = 0;
    // make sure the nodes of pThis point to pPrev
    Aig_ManForEachObj( pPrev, pObj, i )
        pObj->fMarkB = 1;
    Aig_ManForEachObj( pPrev, pObj, i )
        pObj->fMarkB = 0;
    // remap nodes of pThis on top of pNew using pPrev
    pObj = Aig_ManConst1(pThis);
    pObj->pData = Aig_ManConst1(pNew);
    Aig_ManForEachCi( pThis, pObj, i )
        pObj->pData = Aig_ManCi(pNew, i);
    Aig_ManForEachCo( pThis, pObj, i )
        pObj->pData = Aig_ManCo(pNew, i);
    // go through the nodes in the topological order
    Aig_ManForEachNode( pThis, pObj, i )
        pObj->pData = Aig_And( pNew, Aig_ObjChild0Copy(pObj), Aig_ObjChild1Copy(pObj) );
    // set the inputs of POs as equivalent
    Aig_ManForEachCo( pThis, pObj, i )
    {
        pObjNew = Aig_ObjFanin0( Aig_ManCo(pNew,i) );
        // pObjNew and Aig_ObjFanin0(pObj)->pData are equivalent
        Aig_ObjSetRepr_( pNew, pObjNew, Aig_Regular((Aig_Obj_t *)Aig_ObjFanin0(pObj)->pData) );
    }
}

/**Function*************************************************************

  Synopsis    [Constructively accumulates choices.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ManChoiceEval( Aig_Man_t * p )
{
    Vec_Ptr_t * vSupp;
    Aig_Obj_t * pNode, * pTemp;
    int i, Counter;

    vSupp = Vec_PtrAlloc( 100 );
    Aig_ManForEachNode( p, pNode, i )
    {
        if ( !Aig_ObjIsChoice(p, pNode) )
            continue; 
        Counter = 0;
        for ( pTemp = pNode; pTemp; pTemp = Aig_ObjEquiv(p, pTemp) )
            Counter++;
        printf( "Choice node = %5d. Level = %2d. Choices = %d. { ", pNode->Id, pNode->Level, Counter );
        for ( pTemp = pNode; pTemp; pTemp = Aig_ObjEquiv(p, pTemp) )
        {
            Counter = Aig_NodeMffcSupp( p, pTemp, 0, vSupp );
            printf( "S=%d N=%d L=%d  ", Vec_PtrSize(vSupp), Counter, pTemp->Level );
        }
        printf( "}\n" );
    }
    Vec_PtrFree( vSupp );
}

/**Function*************************************************************

  Synopsis    [Constructively accumulates choices.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Aig_ManChoiceConstructive( Vec_Ptr_t * vAigs, int fVerbose )
{
    Vec_Ptr_t * vPios;
    Aig_Man_t * pNew, * pThis, * pPrev, * pTemp;
    int i;
    // start AIG with choices
    pPrev = (Aig_Man_t *)Vec_PtrEntry( vAigs, 0 );
    pNew = Aig_ManDupOrdered( pPrev );
    // create room for equivalent nodes and representatives
    assert( pNew->pReprs == NULL );
    pNew->nReprsAlloc = Vec_PtrSize(vAigs) * Aig_ManObjNumMax(pNew);
    pNew->pReprs = ABC_ALLOC( Aig_Obj_t *, pNew->nReprsAlloc );
    memset( pNew->pReprs, 0, sizeof(Aig_Obj_t *) * pNew->nReprsAlloc );
    // add other AIGs one by one
    Vec_PtrForEachEntryStart( Aig_Man_t *, vAigs, pThis, i, 1 )
    {
        Aig_ManChoiceConstructiveOne( pNew, pPrev, pThis );
        pPrev = pThis;
    }
    // derive the result of choicing
    pNew = Aig_ManRehash( pNew );
    // create the equivalent nodes lists
    Aig_ManMarkValidChoices( pNew );
    // reconstruct the network
    vPios = Aig_ManOrderPios( pNew, (Aig_Man_t *)Vec_PtrEntry( vAigs, 0 ) );
    pNew = Aig_ManDupDfsGuided( pTemp = pNew, vPios );
    Aig_ManStop( pTemp );
    Vec_PtrFree( vPios );
    // duplicate the timing manager
    pTemp = (Aig_Man_t *)Vec_PtrEntry( vAigs, 0 );
    if ( pTemp->pManTime )
        pNew->pManTime = Tim_ManDup( (Tim_Man_t *)pTemp->pManTime, 0 );
    // reset levels
    Aig_ManChoiceLevel( pNew );
    return pNew;
}

/*
    Vec_Ptr_t * vPios;
    vPios = Aig_ManOrderPios( pMan, pAig );
    Vec_PtrFree( vPios );
*/

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

