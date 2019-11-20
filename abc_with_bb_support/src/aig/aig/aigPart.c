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

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Part_Man_t_     Part_Man_t;
struct Part_Man_t_
{
    int              nChunkSize;    // the size of one chunk of memory (~1 Mb)
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
    p = ALLOC( Part_Man_t, 1 );
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
    Vec_PtrForEachEntry( p->vMemory, pMemory, i )
        free( pMemory );
    Vec_PtrFree( p->vMemory );
    Vec_PtrFree( p->vFree );
    free( p );
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
    if ( pMemory = Vec_PtrEntry( p->vFree, Type ) )
    {
        Vec_PtrWriteEntry( p->vFree, Type, Part_OneNext(pMemory) );
        return pMemory;
    }
    nSizeReal = p->nStepSize * Type;
    if ( p->nFreeSize < nSizeReal )
    {
        p->pFreeBuf = ALLOC( char, p->nChunkSize );
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
    Part_OneSetNext( pMemory, Vec_PtrEntry(p->vFree, Type) );
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

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Vec_t * Aig_ManSupports( Aig_Man_t * pMan )
{
    Vec_Vec_t * vSupps;
    Vec_Int_t * vSupp;
    Part_Man_t * p;
    Part_One_t * pPart0, * pPart1;
    Aig_Obj_t * pObj;
    int i, iIns = 0, iOut = 0;
    // start the support computation manager
    p = Part_ManStart( 1 << 20, 1 << 6 );
    // consider objects in the topological order
    vSupps = Vec_VecAlloc( Aig_ManPoNum(pMan) );
    Aig_ManCleanData(pMan);
    Aig_ManForEachObj( pMan, pObj, i )
    {
        if ( Aig_ObjIsNode(pObj) )
        {
            pPart0 = Aig_ObjFanin0(pObj)->pData;
            pPart1 = Aig_ObjFanin1(pObj)->pData;
            pObj->pData = Part_ManMergeEntry( p, pPart0, pPart1, pObj->nRefs );
            assert( pPart0->nRefs > 0 );
            if ( --pPart0->nRefs == 0 )
                Part_ManRecycleEntry( p, pPart0 );
            assert( pPart1->nRefs > 0 );
            if ( --pPart1->nRefs == 0 )
                Part_ManRecycleEntry( p, pPart1 );
            continue;
        }
        if ( Aig_ObjIsPo(pObj) )
        {
            pPart0 = Aig_ObjFanin0(pObj)->pData;
            vSupp = Part_ManTransferEntry(pPart0);
            Vec_IntPush( vSupp, iOut++ );
            Vec_PtrPush( (Vec_Ptr_t *)vSupps, vSupp );
            assert( pPart0->nRefs > 0 );
            if ( --pPart0->nRefs == 0 )
                Part_ManRecycleEntry( p, pPart0 );
            continue;
        }
        if ( Aig_ObjIsPi(pObj) )
        {
            if ( pObj->nRefs )
            {
                pPart0 = Part_ManFetchEntry( p, 1, pObj->nRefs );
                pPart0->pOuts[ pPart0->nOuts++ ] = iIns++;
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
printf( "Memory usage = %d Mb.\n", Vec_PtrSize(p->vMemory) * p->nChunkSize / (1<<20) );
    Part_ManStop( p );
    // sort supports by size
    Vec_VecSort( vSupps, 1 );
/*
    Aig_ManForEachPo( pMan, pObj, i )
        printf( "%d ", Vec_IntSize( (Vec_Int_t *)Vec_VecEntry(vSupps, i) ) );
    printf( "\n" );
*/
    return vSupps;
}

/**Function*************************************************************

  Synopsis    [Start char-bases support representation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Aig_ManSuppCharStart( Vec_Int_t * vOne, int nPis )
{
    char * pBuffer;
    int i, Entry;
    pBuffer = ALLOC( char, nPis );
    memset( pBuffer, 0, sizeof(char) * nPis );
    Vec_IntForEachEntry( vOne, Entry, i )
    {
        assert( Entry < nPis );
        pBuffer[Entry] = 1;
    }
    return pBuffer;
}

/**Function*************************************************************

  Synopsis    [Add to char-bases support representation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ManSuppCharAdd( char * pBuffer, Vec_Int_t * vOne, int nPis )
{
    int i, Entry;
    Vec_IntForEachEntry( vOne, Entry, i )
    {
        assert( Entry < nPis );
        pBuffer[Entry] = 1;
    }
}

/**Function*************************************************************

  Synopsis    [Find the common variables using char-bases support representation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aig_ManSuppCharCommon( char * pBuffer, Vec_Int_t * vOne )
{
    int i, Entry, nCommon = 0;
    Vec_IntForEachEntry( vOne, Entry, i )
        nCommon += pBuffer[Entry];
    return nCommon;
}

/**Function*************************************************************

  Synopsis    [Find the best partition.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aig_ManPartitionSmartFindPart( Vec_Ptr_t * vPartSuppsAll, Vec_Ptr_t * vPartsAll, Vec_Ptr_t * vPartSuppsChar, int nPartSizeLimit, Vec_Int_t * vOne )
{
    Vec_Int_t * vPartSupp, * vPart;
    int Attract, Repulse, Value, ValueBest;
    int i, nCommon, iBest;
    iBest = -1;
    ValueBest = 0;
    Vec_PtrForEachEntry( vPartSuppsAll, vPartSupp, i )
    {
        vPart = Vec_PtrEntry( vPartsAll, i );
        if ( nPartSizeLimit > 0 && Vec_IntSize(vPart) >= nPartSizeLimit )
            continue;
//        nCommon = Vec_IntTwoCountCommon( vPartSupp, vOne );
        nCommon = Aig_ManSuppCharCommon( Vec_PtrEntry(vPartSuppsChar, i), vOne );
        if ( nCommon == 0 )
            continue;
        if ( nCommon == Vec_IntSize(vOne) )
            return i;
        Attract = 1000 * nCommon / Vec_IntSize(vOne);
        if ( Vec_IntSize(vPartSupp) < 100 )
            Repulse = 1;
        else
            Repulse = 1+Extra_Base2Log(Vec_IntSize(vPartSupp)-100);
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
    Vec_PtrForEachEntry( vPartSuppsAll, vOne, i )
    {
        nOutputs = Vec_IntSize(Vec_PtrEntry(vPartsAll, i));
        printf( "%d=(%d,%d) ", i, Vec_IntSize(vOne), nOutputs );
        Counter += nOutputs;
        if ( i == Vec_PtrSize(vPartsAll) - 1 )
            break;
    }
    assert( Counter == Aig_ManPoNum(p) );
    printf( "\nTotal = %d. Outputs = %d.\n", Counter, Aig_ManPoNum(p) );
}

/**Function*************************************************************

  Synopsis    [Perform the smart partitioning.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ManPartitionCompact( Vec_Ptr_t * vPartsAll, Vec_Ptr_t * vPartSuppsAll, int nPartSizeLimit )
{
    Vec_Int_t * vOne, * vPart, * vPartSupp, * vTemp;
    int i, iPart;

    if ( nPartSizeLimit == 0 )
        nPartSizeLimit = 200;

    // pack smaller partitions into larger blocks
    iPart = 0;
    vPart = vPartSupp = NULL;
    Vec_PtrForEachEntry( vPartSuppsAll, vOne, i )
    {
        if ( Vec_IntSize(vOne) < nPartSizeLimit )
        {
            if ( vPartSupp == NULL )
            {
                assert( vPart == NULL );
                vPartSupp = Vec_IntDup(vOne);
                vPart = Vec_PtrEntry(vPartsAll, i);
            }
            else
            {
                vPartSupp = Vec_IntTwoMerge( vTemp = vPartSupp, vOne );
                Vec_IntFree( vTemp );
                vPart = Vec_IntTwoMerge( vTemp = vPart, Vec_PtrEntry(vPartsAll, i) );
                Vec_IntFree( vTemp );
                Vec_IntFree( Vec_PtrEntry(vPartsAll, i) );
            }
            if ( Vec_IntSize(vPartSupp) < nPartSizeLimit )
                continue;
        }
        else
            vPart = Vec_PtrEntry(vPartsAll, i);
        // add the partition 
        Vec_PtrWriteEntry( vPartsAll, iPart, vPart );  
        vPart = NULL;
        if ( vPartSupp ) 
        {
            Vec_IntFree( Vec_PtrEntry(vPartSuppsAll, iPart) );
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
        Vec_IntFree( Vec_PtrEntry(vPartSuppsAll, iPart) );
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
Vec_Vec_t * Aig_ManPartitionSmart( Aig_Man_t * p, int nPartSizeLimit, int fVerbose, Vec_Vec_t ** pvPartSupps )
{
    Vec_Ptr_t * vPartSuppsChar;
    Vec_Ptr_t * vSupps, * vPartsAll, * vPartsAll2, * vPartSuppsAll;//, * vPartPtr;
    Vec_Int_t * vOne, * vPart, * vPartSupp, * vTemp;
    int i, iPart, iOut, clk;

    // compute the supports for all outputs
clk = clock();
    vSupps = (Vec_Ptr_t *)Aig_ManSupports( p );
if ( fVerbose )
{
PRT( "Supps", clock() - clk );
}
    // start char-based support representation
    vPartSuppsChar = Vec_PtrAlloc( 1000 );

    // create partitions
clk = clock();
    vPartsAll = Vec_PtrAlloc( 256 );
    vPartSuppsAll = Vec_PtrAlloc( 256 );
    Vec_PtrForEachEntry( vSupps, vOne, i )
    {
        // get the output number
        iOut = Vec_IntPop(vOne);
        // find closely matching part
        iPart = Aig_ManPartitionSmartFindPart( vPartSuppsAll, vPartsAll, vPartSuppsChar, nPartSizeLimit, vOne );
//printf( "%4d->%4d  ", i, iPart );
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

            Vec_PtrPush( vPartSuppsChar, Aig_ManSuppCharStart(vOne, Aig_ManPiNum(p)) );
        }
        else
        {
            // add output to this partition
            vPart = Vec_PtrEntry( vPartsAll, iPart );
            Vec_IntPush( vPart, iOut );
            // merge supports
            vPartSupp = Vec_PtrEntry( vPartSuppsAll, iPart );
            vPartSupp = Vec_IntTwoMerge( vTemp = vPartSupp, vOne );
            Vec_IntFree( vTemp );
            // reinsert new support
            Vec_PtrWriteEntry( vPartSuppsAll, iPart, vPartSupp );

            Aig_ManSuppCharAdd( Vec_PtrEntry(vPartSuppsChar, iPart), vOne, Aig_ManPiNum(p) );
        }
    }

    // stop char-based support representation
    Vec_PtrForEachEntry( vPartSuppsChar, vTemp, i )
        free( vTemp );
    Vec_PtrFree( vPartSuppsChar );

//printf( "\n" );
if ( fVerbose )
{
PRT( "Parts", clock() - clk );
}

clk = clock();
    // reorder partitions in the decreasing order of support sizes
    // remember partition number in each partition support
    Vec_PtrForEachEntry( vPartSuppsAll, vOne, i )
        Vec_IntPush( vOne, i );
    // sort the supports in the decreasing order
    Vec_VecSort( (Vec_Vec_t *)vPartSuppsAll, 1 );
    // reproduce partitions
    vPartsAll2 = Vec_PtrAlloc( 256 );
    Vec_PtrForEachEntry( vPartSuppsAll, vOne, i )
        Vec_PtrPush( vPartsAll2, Vec_PtrEntry(vPartsAll, Vec_IntPop(vOne)) );
    Vec_PtrFree( vPartsAll );
    vPartsAll = vPartsAll2;

    // compact small partitions
//    Aig_ManPartitionPrint( p, vPartsAll, vPartSuppsAll );
    Aig_ManPartitionCompact( vPartsAll, vPartSuppsAll, nPartSizeLimit );
    if ( fVerbose )
//    Aig_ManPartitionPrint( p, vPartsAll, vPartSuppsAll );
    printf( "Created %d partitions.\n", Vec_PtrSize(vPartsAll) );

if ( fVerbose )
{
//PRT( "Comps", clock() - clk );
}

    // cleanup
    Vec_VecFree( (Vec_Vec_t *)vSupps );
    if ( pvPartSupps == NULL )
        Vec_VecFree( (Vec_Vec_t *)vPartSuppsAll );
    else
        *pvPartSupps = (Vec_Vec_t *)vPartSuppsAll;
/*
    // converts from intergers to nodes
    Vec_PtrForEachEntry( vPartsAll, vPart, iPart )
    {
        vPartPtr = Vec_PtrAlloc( Vec_IntSize(vPart) );
        Vec_IntForEachEntry( vPart, iOut, i )
            Vec_PtrPush( vPartPtr, Aig_ManPo(p, iOut) );
        Vec_IntFree( vPart );
        Vec_PtrWriteEntry( vPartsAll, iPart, vPartPtr );
    }
*/
    return (Vec_Vec_t *)vPartsAll;
}

/**Function*************************************************************

  Synopsis    [Perform the naive partitioning.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Vec_t * Aig_ManPartitionNaive( Aig_Man_t * p, int nPartSize )
{
    Vec_Vec_t * vParts;
    Aig_Obj_t * pObj;
    int nParts, i;
    nParts = (Aig_ManPoNum(p) / nPartSize) + ((Aig_ManPoNum(p) % nPartSize) > 0);
    vParts = Vec_VecStart( nParts );
    Aig_ManForEachPo( p, pObj, i )
        Vec_VecPush( vParts, i / nPartSize, pObj );
    return vParts;
}



/**Function*************************************************************

  Synopsis    [Adds internal nodes in the topological order.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Obj_t * Aig_ManDupPart_rec( Aig_Man_t * pNew, Aig_Man_t * p, Aig_Obj_t * pObj )
{
    assert( !Aig_IsComplement(pObj) );
    if ( Aig_ObjIsTravIdCurrent(p, pObj) )
        return pObj->pData;
    assert( Aig_ObjIsNode(pObj) );
    Aig_ManDupPart_rec( pNew, p, Aig_ObjFanin0(pObj) );
    Aig_ManDupPart_rec( pNew, p, Aig_ObjFanin1(pObj) );
    pObj->pData = Aig_And( pNew, Aig_ObjChild0Copy(pObj), Aig_ObjChild1Copy(pObj) );
    assert( !Aig_ObjIsTravIdCurrent(p, pObj) ); // loop detection
    Aig_ObjSetTravIdCurrent(p, pObj);
    return pObj->pData;
}

/**Function*************************************************************

  Synopsis    [Adds internal nodes in the topological order.]

  Description [Returns the array of new outputs.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Aig_ManDupPart( Aig_Man_t * pNew, Aig_Man_t * p, Vec_Int_t * vPart, Vec_Int_t * vPartSupp, int fInverse )
{
    Vec_Ptr_t * vOutputs;
    Aig_Obj_t * pObj, * pObjNew;
    int Entry, k;
    // create the PIs
    Aig_ManIncrementTravId( p );
    Aig_ObjSetTravIdCurrent( p, Aig_ManConst1(p) );
    Aig_ManConst1(p)->pData = Aig_ManConst1(pNew);
    if ( !fInverse )
    {
        Vec_IntForEachEntry( vPartSupp, Entry, k )
        {
            pObj = Aig_ManPi( p, Entry );
            pObj->pData = Aig_ManPi( pNew, k );
            Aig_ObjSetTravIdCurrent( p, pObj );
        }
    }
    else
    {
        Vec_IntForEachEntry( vPartSupp, Entry, k )
        {
            pObj = Aig_ManPi( p, k );
            pObj->pData = Aig_ManPi( pNew, Entry );
            Aig_ObjSetTravIdCurrent( p, pObj );
        }
    }
    // create the internal nodes
    vOutputs = Vec_PtrAlloc( Vec_IntSize(vPart) );
    Vec_IntForEachEntry( vPart, Entry, k )
    {
        pObj = Aig_ManPo( p, Entry );
        pObjNew = Aig_ManDupPart_rec( pNew, p, Aig_ObjFanin0(pObj) );
        pObjNew = Aig_NotCond( pObjNew, Aig_ObjFaninC0(pObj) );
        Vec_PtrPush( vOutputs, pObjNew );
    }
    return vOutputs; 
}

/**Function*************************************************************

  Synopsis    [Create partitioned miter of the two AIGs.]

  Description [Assumes that each output in the second AIG cannot have 
  more supp vars than the same output in the first AIG.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Aig_ManMiterPartitioned( Aig_Man_t * p1, Aig_Man_t * p2, int nPartSize )
{
    Aig_Man_t * pNew;
    Aig_Obj_t * pMiter;
    Vec_Ptr_t * vMiters, * vNodes1, * vNodes2;
    Vec_Vec_t * vParts, * vPartSupps;
    Vec_Int_t * vPart, * vPartSupp;
    int i, k;
    // partition the first manager
    vParts = Aig_ManPartitionSmart( p1, nPartSize, 0, &vPartSupps );
    // derive miters
    vMiters = Vec_PtrAlloc( Vec_VecSize(vParts) );
    for ( i = 0; i < Vec_VecSize(vParts); i++ )
    {
        // get partitions and their support
        vPart     = Vec_PtrEntry( (Vec_Ptr_t *)vParts, i );
        vPartSupp = Vec_PtrEntry( (Vec_Ptr_t *)vPartSupps, i );
        // create the new miter
        pNew = Aig_ManStart( 1000 );
        // create the PIs
        for ( k = 0; k < Vec_IntSize(vPartSupp); k++ )
            Aig_ObjCreatePi( pNew );
        // copy the components
        vNodes1 = Aig_ManDupPart( pNew, p1, vPart, vPartSupp, 0 );
        vNodes2 = Aig_ManDupPart( pNew, p2, vPart, vPartSupp, 0 );
        // create the miter
        pMiter = Aig_MiterTwo( pNew, vNodes1, vNodes2 );
        // create the output
        Aig_ObjCreatePo( pNew, pMiter );
        // clean up
        Aig_ManCleanup( pNew );
        Vec_PtrPush( vMiters, pNew );
    }
    Vec_VecFree( vParts );
    Vec_VecFree( vPartSupps );
    return vMiters;
}

/**Function*************************************************************

  Synopsis    [Performs partitioned choice computation.]

  Description [Assumes that each output in the second AIG cannot have 
  more supp vars than the same output in the first AIG.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Aig_ManChoicePartitioned( Vec_Ptr_t * vAigs, int nPartSize )
{
    Aig_Man_t * pChoice, * pNew, * pAig, * p;
    Aig_Obj_t * pObj;
    Vec_Ptr_t * vMiters, * vNodes;
    Vec_Vec_t * vParts, * vPartSupps;
    Vec_Int_t * vPart, * vPartSupp;
    int i, k, m;

    // partition the first AIG
    assert( Vec_PtrSize(vAigs) > 1 );
    pAig = Vec_PtrEntry( vAigs, 0 );
    vParts = Aig_ManPartitionSmart( pAig, nPartSize, 0, &vPartSupps );

    // derive the AIG partitions
    vMiters = Vec_PtrAlloc( Vec_VecSize(vParts) );
    for ( i = 0; i < Vec_VecSize(vParts); i++ )
    {
        // get partitions and their support
        vPart     = Vec_PtrEntry( (Vec_Ptr_t *)vParts, i );
        vPartSupp = Vec_PtrEntry( (Vec_Ptr_t *)vPartSupps, i );
        // create the new miter
        pNew = Aig_ManStart( 1000 );
        // create the PIs
        for ( k = 0; k < Vec_IntSize(vPartSupp); k++ )
            Aig_ObjCreatePi( pNew );
        // copy the components
        Vec_PtrForEachEntry( vAigs, p, k )
        {
            vNodes = Aig_ManDupPart( pNew, p, vPart, vPartSupp, 0 );
            Vec_PtrForEachEntry( vNodes, pObj, m )
                Aig_ObjCreatePo( pNew, pObj );
            Vec_PtrFree( vNodes );
        }
        // add to the partitions
        Vec_PtrPush( vMiters, pNew );
    }

    // perform choicing for each derived AIG
    Vec_PtrForEachEntry( vMiters, pNew, i )
    {
        extern Aig_Man_t * Fra_FraigChoice( Aig_Man_t * pManAig );
        pNew = Fra_FraigChoice( p = pNew );
        Vec_PtrWriteEntry( vMiters, i, pNew );
        Aig_ManStop( p );
    }

    // combine the resulting AIGs
    pNew = Aig_ManStartFrom( pAig );
    Vec_PtrForEachEntry( vMiters, p, i )
    {
        // get partitions and their support
        vPart     = Vec_PtrEntry( (Vec_Ptr_t *)vParts, i );
        vPartSupp = Vec_PtrEntry( (Vec_Ptr_t *)vPartSupps, i );
        // copy the component
        vNodes = Aig_ManDupPart( pNew, p, vPart, vPartSupp, 1 );
        assert( Vec_PtrSize(vNodes) == Vec_VecSize(vParts) * Vec_PtrSize(vAigs) );
        // create choice node
        for ( k = 0; k < Vec_VecSize(vParts); k++ )
        {
            for ( m = 0; m < Vec_PtrSize(vAigs); m++ )
            {
                pObj = Aig_ObjChild0( Vec_PtrEntry(vNodes, k * Vec_PtrSize(vAigs) + m) );
                break;
            }
            Aig_ObjCreatePo( pNew, pObj );
        }
        Vec_PtrFree( vNodes );
    }
    Vec_VecFree( vParts );
    Vec_VecFree( vPartSupps );

    // trasfer representatives
    Aig_ManReprStart( pNew, Aig_ManObjIdMax(pNew)+1 );
    Vec_PtrForEachEntry( vMiters, p, i )
    {
        Aig_ManTransferRepr( pNew, p );
        Aig_ManStop( p );
    }
    Vec_PtrFree( vMiters );

    // derive the result of choicing
    pChoice = Aig_ManRehash( pNew );
    if ( pChoice != pNew )
        Aig_ManStop( pNew );
    return pChoice;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


