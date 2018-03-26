/**CFile****************************************************************

  FileName    [nwkMerge.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Netlist representation.]

  Synopsis    [LUT merging algorithm.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: nwkMerge.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "nwk.h"
#include "nwkMerge.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Allocates the graph.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Nwk_Grf_t * Nwk_ManGraphAlloc( int nVertsMax )
{
    Nwk_Grf_t * p;
    p = ABC_ALLOC( Nwk_Grf_t, 1 );
    memset( p, 0, sizeof(Nwk_Grf_t) );
    p->nVertsMax = nVertsMax;
    p->nEdgeHash = Abc_PrimeCudd( 3 * nVertsMax );
    p->pEdgeHash = ABC_CALLOC( Nwk_Edg_t *, p->nEdgeHash );
    p->pMemEdges = Aig_MmFixedStart( sizeof(Nwk_Edg_t), p->nEdgeHash );
    p->vPairs    = Vec_IntAlloc( 1000 );
    return p;
}

/**Function*************************************************************

  Synopsis    [Deallocates the graph.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Nwk_ManGraphFree( Nwk_Grf_t * p )
{
    if ( p->vPairs )    Vec_IntFree( p->vPairs );
    if ( p->pMemEdges ) Aig_MmFixedStop( p->pMemEdges, 0 );
    if ( p->pMemVerts ) Aig_MmFlexStop( p->pMemVerts, 0 );
    ABC_FREE( p->pVerts );
    ABC_FREE( p->pEdgeHash );
    ABC_FREE( p->pMapLut2Id );
    ABC_FREE( p->pMapId2Lut );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    [Prepares the graph for solving the problem.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Nwk_ManGraphReportMemoryUsage( Nwk_Grf_t * p )
{
    p->nMemBytes1 = 
        sizeof(Nwk_Grf_t) +
        sizeof(void *) * p->nEdgeHash + 
        sizeof(int) * (p->nObjs + p->nVertsMax) +
        sizeof(Nwk_Edg_t) * p->nEdges;
    p->nMemBytes2 = 
        sizeof(Nwk_Vrt_t) * p->nVerts + 
        sizeof(int) * 2 * p->nEdges;
    printf( "Memory usage stats:  Preprocessing = %.2f MB.  Solving = %.2f MB.\n",
        1.0 * p->nMemBytes1 / (1<<20), 1.0 * p->nMemBytes2 / (1<<20) );
}


/**Function*************************************************************

  Synopsis    [Finds or adds the edge to the graph.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Nwk_ManGraphHashEdge( Nwk_Grf_t * p, int iLut1, int iLut2 )
{
    Nwk_Edg_t * pEntry;
    unsigned Key;
    if ( iLut1 == iLut2 )
        return;
    if ( iLut1 > iLut2 )
    {
        Key = iLut1;
        iLut1 = iLut2;
        iLut2 = Key;
    }
    assert( iLut1 < iLut2 );
    if ( p->nObjs < iLut2 )
        p->nObjs = iLut2;
    Key = (unsigned)(741457 * iLut1 + 4256249 * iLut2) % p->nEdgeHash;
    for ( pEntry = p->pEdgeHash[Key]; pEntry; pEntry = pEntry->pNext )
        if ( pEntry->iNode1 == iLut1 && pEntry->iNode2 == iLut2 )
            return;
    pEntry = (Nwk_Edg_t *)Aig_MmFixedEntryFetch( p->pMemEdges );
    pEntry->iNode1 = iLut1;
    pEntry->iNode2 = iLut2;
    pEntry->pNext = p->pEdgeHash[Key];
    p->pEdgeHash[Key] = pEntry;
    p->nEdges++;
}

/**Function*************************************************************

  Synopsis    [Adds one entry to the list.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Nwk_ManGraphListAdd( Nwk_Grf_t * p, int * pList, Nwk_Vrt_t * pVertex )
{
    if ( *pList )
    {
        Nwk_Vrt_t * pHead;
        pHead = p->pVerts[*pList];
        pVertex->iPrev = 0;
        pVertex->iNext = pHead->Id;
        pHead->iPrev = pVertex->Id;
    }
    *pList = pVertex->Id;
}

/**Function*************************************************************

  Synopsis    [Deletes one entry from the list.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Nwk_ManGraphListDelete( Nwk_Grf_t * p, int * pList, Nwk_Vrt_t * pVertex )
{
    assert( *pList );
    if ( pVertex->iPrev )
    {
//        assert( p->pVerts[pVertex->iPrev]->iNext == pVertex->Id );
        p->pVerts[pVertex->iPrev]->iNext = pVertex->iNext;
    }
    if ( pVertex->iNext )
    {
//        assert( p->pVerts[pVertex->iNext]->iPrev == pVertex->Id );
        p->pVerts[pVertex->iNext]->iPrev = pVertex->iPrev;
    }
    if ( *pList == pVertex->Id )
        *pList = pVertex->iNext;
    pVertex->iPrev = pVertex->iNext = 0;
}

/**Function*************************************************************

  Synopsis    [Inserts the edge into one of the linked lists.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Nwk_ManGraphListInsert( Nwk_Grf_t * p, Nwk_Vrt_t * pVertex )
{
    Nwk_Vrt_t * pNext;
    assert( pVertex->nEdges > 0 );

    if ( pVertex->nEdges == 1 )
    {
        pNext = p->pVerts[ pVertex->pEdges[0] ];
        if ( pNext->nEdges >= NWK_MAX_LIST )
            Nwk_ManGraphListAdd( p, p->pLists1 + NWK_MAX_LIST, pVertex );
        else
            Nwk_ManGraphListAdd( p, p->pLists1 + pNext->nEdges, pVertex );
    }
    else
    {
        if ( pVertex->nEdges >= NWK_MAX_LIST )
            Nwk_ManGraphListAdd( p, p->pLists2 + NWK_MAX_LIST, pVertex );
        else
            Nwk_ManGraphListAdd( p, p->pLists2 + pVertex->nEdges, pVertex );
    }
}

/**Function*************************************************************

  Synopsis    [Extracts the edge from one of the linked lists.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Nwk_ManGraphListExtract( Nwk_Grf_t * p, Nwk_Vrt_t * pVertex )
{
    Nwk_Vrt_t * pNext;
    assert( pVertex->nEdges > 0 );

    if ( pVertex->nEdges == 1 )
    {
        pNext = p->pVerts[ pVertex->pEdges[0] ];
        if ( pNext->nEdges >= NWK_MAX_LIST )
            Nwk_ManGraphListDelete( p, p->pLists1 + NWK_MAX_LIST, pVertex );
        else
            Nwk_ManGraphListDelete( p, p->pLists1 + pNext->nEdges, pVertex );
    }
    else
    {
        if ( pVertex->nEdges >= NWK_MAX_LIST )
            Nwk_ManGraphListDelete( p, p->pLists2 + NWK_MAX_LIST, pVertex );
        else
            Nwk_ManGraphListDelete( p, p->pLists2 + pVertex->nEdges, pVertex );
    }
}

/**Function*************************************************************

  Synopsis    [Prepares the graph for solving the problem.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Nwk_ManGraphPrepare( Nwk_Grf_t * p )
{
    Nwk_Edg_t * pEntry;
    Nwk_Vrt_t * pVertex;
    int * pnEdges, nBytes, i;
    // allocate memory for the present objects
    p->pMapLut2Id = ABC_ALLOC( int, p->nObjs+1 );
    p->pMapId2Lut = ABC_ALLOC( int, p->nVertsMax+1 );
    memset( p->pMapLut2Id, 0xff, sizeof(int) * (p->nObjs+1) );
    memset( p->pMapId2Lut, 0xff, sizeof(int) * (p->nVertsMax+1) );
    // mark present objects
    Nwk_GraphForEachEdge( p, pEntry, i )
    {
        assert( pEntry->iNode1 <= p->nObjs );
        assert( pEntry->iNode2 <= p->nObjs );
        p->pMapLut2Id[ pEntry->iNode1 ] = 0;
        p->pMapLut2Id[ pEntry->iNode2 ] = 0;
    }
    // map objects
    p->nVerts = 0;
    for ( i = 0; i <= p->nObjs; i++ )
    {
        if ( p->pMapLut2Id[i] == 0 )
        {
            p->pMapLut2Id[i] = ++p->nVerts;
            p->pMapId2Lut[p->nVerts] = i;
        }
    }
    // count the edges and mark present objects
    pnEdges = ABC_CALLOC( int, p->nVerts+1 );
    Nwk_GraphForEachEdge( p, pEntry, i )
    {
        // translate into vertices
        assert( pEntry->iNode1 <= p->nObjs );
        assert( pEntry->iNode2 <= p->nObjs );
        pEntry->iNode1 = p->pMapLut2Id[pEntry->iNode1];
        pEntry->iNode2 = p->pMapLut2Id[pEntry->iNode2];
        // count the edges
        assert( pEntry->iNode1 <= p->nVerts );
        assert( pEntry->iNode2 <= p->nVerts );
        pnEdges[pEntry->iNode1]++;
        pnEdges[pEntry->iNode2]++;
    }
    // allocate the real graph
    p->pMemVerts  = Aig_MmFlexStart();
    p->pVerts = ABC_ALLOC( Nwk_Vrt_t *, p->nVerts + 1 );
    p->pVerts[0] = NULL;
    for ( i = 1; i <= p->nVerts; i++ )
    {
        assert( pnEdges[i] > 0 );
        nBytes = sizeof(Nwk_Vrt_t) + sizeof(int) * pnEdges[i];
        p->pVerts[i] = (Nwk_Vrt_t *)Aig_MmFlexEntryFetch( p->pMemVerts, nBytes );
        memset( p->pVerts[i], 0, nBytes );
        p->pVerts[i]->Id = i;
    }
    // add edges to the real graph
    Nwk_GraphForEachEdge( p, pEntry, i )
    {
        pVertex = p->pVerts[pEntry->iNode1];
        pVertex->pEdges[ pVertex->nEdges++ ] = pEntry->iNode2;
        pVertex = p->pVerts[pEntry->iNode2];
        pVertex->pEdges[ pVertex->nEdges++ ] = pEntry->iNode1;
    }
    // put vertices into the data structure
    for ( i = 1; i <= p->nVerts; i++ )
    {
        assert( p->pVerts[i]->nEdges == pnEdges[i] );
        Nwk_ManGraphListInsert( p, p->pVerts[i] );
    }
    // clean up
    Aig_MmFixedStop( p->pMemEdges, 0 ); p->pMemEdges = NULL;
    ABC_FREE( p->pEdgeHash );
//    p->nEdgeHash = 0;
    ABC_FREE( pnEdges );
}

/**Function*************************************************************

  Synopsis    [Sort pairs by the first vertex in the topological order.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Nwk_ManGraphSortPairs( Nwk_Grf_t * p )
{
    int nSize = Vec_IntSize(p->vPairs);
    int * pIdToPair, i;
    // allocate storage
    pIdToPair = ABC_ALLOC( int, p->nObjs+1 );
    for ( i = 0; i <= p->nObjs; i++ )
        pIdToPair[i] = -1;
    // create mapping
    for ( i = 0; i < p->vPairs->nSize; i += 2 )
    {
        assert( pIdToPair[ p->vPairs->pArray[i] ] == -1 );
        pIdToPair[ p->vPairs->pArray[i] ] = p->vPairs->pArray[i+1];
    }
    // recreate pairs
    Vec_IntClear( p->vPairs );
    for ( i = 0; i <= p->nObjs; i++ )
        if ( pIdToPair[i] >= 0 )
        {
            assert( i < pIdToPair[i] );
            Vec_IntPush( p->vPairs, i );
            Vec_IntPush( p->vPairs, pIdToPair[i] );
        }
    assert( nSize == Vec_IntSize(p->vPairs) );
    ABC_FREE( pIdToPair );
}


/**Function*************************************************************

  Synopsis    [Updates the problem after pulling out one edge.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Nwk_ManGraphCheckLists( Nwk_Grf_t * p )
{
    Nwk_Vrt_t * pVertex, * pNext;
    int i, j;
    assert( p->pLists1[0] == 0 );
    for ( i = 1; i <= NWK_MAX_LIST; i++ )
        if ( p->pLists1[i] )
        {
            pVertex = p->pVerts[ p->pLists1[i] ];
            assert( pVertex->nEdges == 1 );
            pNext = p->pVerts[ pVertex->pEdges[0] ];
            assert( pNext->nEdges == i || pNext->nEdges > NWK_MAX_LIST );
        }
    // find the next vertext to extract
    assert( p->pLists2[0] == 0 );
    assert( p->pLists2[1] == 0 );
    for ( j = 2; j <= NWK_MAX_LIST; j++ )
        if ( p->pLists2[j] )
        {
            pVertex = p->pVerts[ p->pLists2[j] ];
            assert( pVertex->nEdges == j || pVertex->nEdges > NWK_MAX_LIST );
        }
}

/**Function*************************************************************

  Synopsis    [Extracts the edge from one of the linked lists.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Nwk_ManGraphVertexRemoveEdge( Nwk_Vrt_t * pThis, Nwk_Vrt_t * pNext )
{
    int k;
    for ( k = 0; k < pThis->nEdges; k++ )
        if ( pThis->pEdges[k] == pNext->Id )
            break;
    assert( k < pThis->nEdges );
    pThis->nEdges--;
    for ( ; k < pThis->nEdges; k++ )
        pThis->pEdges[k] = pThis->pEdges[k+1];
}

/**Function*************************************************************

  Synopsis    [Updates the problem after pulling out one edge.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Nwk_ManGraphUpdate( Nwk_Grf_t * p, Nwk_Vrt_t * pVertex, Nwk_Vrt_t * pNext )
{
    Nwk_Vrt_t * pChanged, * pOther;
    int i, k;
//    Nwk_ManGraphCheckLists( p );
    Nwk_ManGraphListExtract( p, pVertex );
    Nwk_ManGraphListExtract( p, pNext );
    // update neihbors of pVertex
    Nwk_VertexForEachAdjacent( p, pVertex, pChanged, i )
    {
        if ( pChanged == pNext )
            continue;
        Nwk_ManGraphListExtract( p, pChanged );
        // move those that use this one
        if ( pChanged->nEdges > 1 )
        Nwk_VertexForEachAdjacent( p, pChanged, pOther, k )
        {
            if ( pOther == pVertex || pOther->nEdges > 1 )
                continue;
            assert( pOther->nEdges == 1 );
            Nwk_ManGraphListExtract( p, pOther );
            pChanged->nEdges--;
            Nwk_ManGraphListInsert( p, pOther );
            pChanged->nEdges++;
        }
        // remove the edge
        Nwk_ManGraphVertexRemoveEdge( pChanged, pVertex );
        // add the changed vertex back
        if ( pChanged->nEdges > 0 )
            Nwk_ManGraphListInsert( p, pChanged );
    }
    // update neihbors of pNext
    Nwk_VertexForEachAdjacent( p, pNext, pChanged, i )
    {
        if ( pChanged == pVertex )
            continue;
        Nwk_ManGraphListExtract( p, pChanged );
        // move those that use this one
        if ( pChanged->nEdges > 1 )
        Nwk_VertexForEachAdjacent( p, pChanged, pOther, k )
        {
            if ( pOther == pNext || pOther->nEdges > 1 )
                continue;
            assert( pOther->nEdges == 1 );
            Nwk_ManGraphListExtract( p, pOther );
            pChanged->nEdges--;
            Nwk_ManGraphListInsert( p, pOther );
            pChanged->nEdges++;
        }
        // remove the edge
        Nwk_ManGraphVertexRemoveEdge( pChanged, pNext );
        // add the changed vertex back
        if ( pChanged->nEdges > 0 )
            Nwk_ManGraphListInsert( p, pChanged );
    }
    // add to the result
    if ( pVertex->Id < pNext->Id )
    {
        Vec_IntPush( p->vPairs, p->pMapId2Lut[pVertex->Id] ); 
        Vec_IntPush( p->vPairs, p->pMapId2Lut[pNext->Id] ); 
    }
    else
    {
        Vec_IntPush( p->vPairs, p->pMapId2Lut[pNext->Id] ); 
        Vec_IntPush( p->vPairs, p->pMapId2Lut[pVertex->Id] ); 
    }
//    Nwk_ManGraphCheckLists( p );
}

/**Function*************************************************************

  Synopsis    [Counts the number of entries in the list.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Nwk_ManGraphListLength( Nwk_Grf_t * p, int List )
{
    Nwk_Vrt_t * pThis;
    int fVerbose = 0;
    int Counter = 0;
    Nwk_ListForEachVertex( p, List, pThis )
    {
        if ( fVerbose && Counter < 20 )
            printf( "%d ", p->pVerts[pThis->pEdges[0]]->nEdges );
        Counter++;
    }
    if ( fVerbose )
        printf( "\n" );
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Returns the adjacent vertex with the mininum number of edges.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Nwk_Vrt_t * Nwk_ManGraphListFindMinEdge( Nwk_Grf_t * p, Nwk_Vrt_t * pVert )
{
    Nwk_Vrt_t * pThis, * pMinCost = NULL;
    int k;
    Nwk_VertexForEachAdjacent( p, pVert, pThis, k )
    {
        if ( pMinCost == NULL || pMinCost->nEdges > pThis->nEdges )
            pMinCost = pThis;
    }
    return pMinCost;
}

/**Function*************************************************************

  Synopsis    [Finds the best vertext in the list.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Nwk_Vrt_t * Nwk_ManGraphListFindMin( Nwk_Grf_t * p, int List )
{
    Nwk_Vrt_t * pThis, * pMinCost = NULL;
    int k, Counter = 10000, BestCost = 1000000;
    Nwk_ListForEachVertex( p, List, pThis )
    {
        for ( k = 0; k < pThis->nEdges; k++ )
        {
            if ( pMinCost == NULL || BestCost > p->pVerts[pThis->pEdges[k]]->nEdges )
            {
                BestCost = p->pVerts[pThis->pEdges[k]]->nEdges;
                pMinCost = pThis;
            }
        }
        if ( --Counter == 0 )
            break;
    }
    return pMinCost;
}

/**Function*************************************************************

  Synopsis    [Solves the problem by extracting one edge at a time.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Nwk_ManGraphSolve( Nwk_Grf_t * p )
{
    Nwk_Vrt_t * pVertex, * pNext;
    int i, j;
    Nwk_ManGraphPrepare( p );
    while ( 1 )
    {
        // find the next vertex to extract
        assert( p->pLists1[0] == 0 );
        for ( i = 1; i <= NWK_MAX_LIST; i++ )
            if ( p->pLists1[i] )
            {
//                printf( "%d ", i );
//                printf( "ListA = %2d. Length = %5d.\n", i, Nwk_ManGraphListLength(p,p->pLists1[i]) );
                pVertex = p->pVerts[ p->pLists1[i] ];
                assert( pVertex->nEdges == 1 );
                pNext = p->pVerts[ pVertex->pEdges[0] ];
                Nwk_ManGraphUpdate( p, pVertex, pNext );
                break;
            }
        if ( i < NWK_MAX_LIST + 1 )
            continue;
        // find the next vertex to extract
        assert( p->pLists2[0] == 0 );
        assert( p->pLists2[1] == 0 );
        for ( j = 2; j <= NWK_MAX_LIST; j++ )
            if ( p->pLists2[j] )
            {
//                printf( "***%d ", j );
//                printf( "ListB = %2d. Length = %5d.\n", j, Nwk_ManGraphListLength(p,p->pLists2[j]) );
                pVertex = Nwk_ManGraphListFindMin( p, p->pLists2[j] ); 
                assert( pVertex->nEdges == j || j == NWK_MAX_LIST );
                pNext = Nwk_ManGraphListFindMinEdge( p, pVertex );
                Nwk_ManGraphUpdate( p, pVertex, pNext );
                break;
            }
        if ( j == NWK_MAX_LIST + 1 )
            break;
    }
    Nwk_ManGraphSortPairs( p );
}

/**Function*************************************************************

  Synopsis    [Reads graph from file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Nwk_Grf_t * Nwk_ManLutMergeReadGraph( char * pFileName )
{
    Nwk_Grf_t * p;
    FILE * pFile;
    char Buffer[100];
    int nNodes, nEdges, iNode1, iNode2;
    int RetValue;
    pFile = fopen( pFileName, "r" );
    RetValue = fscanf( pFile, "%s %d", Buffer, &nNodes );
    RetValue = fscanf( pFile, "%s %d", Buffer, &nEdges );
    p = Nwk_ManGraphAlloc( nNodes );
    while ( fscanf( pFile, "%s %d %d", Buffer, &iNode1, &iNode2 ) == 3 )
        Nwk_ManGraphHashEdge( p, iNode1, iNode2 );
    assert( p->nEdges == nEdges );
    fclose( pFile );
    return p;
}

/**Function*************************************************************

  Synopsis    [Solves the graph coming from file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Nwk_ManLutMergeGraphTest( char * pFileName )
{
    int nPairs;
    Nwk_Grf_t * p;
    abctime clk = Abc_Clock();
    p = Nwk_ManLutMergeReadGraph( pFileName );
    ABC_PRT( "Reading", Abc_Clock() - clk );
    clk = Abc_Clock();
    Nwk_ManGraphSolve( p );
    printf( "GRAPH: Nodes = %6d. Edges = %6d.  Pairs = %6d.  ", 
        p->nVerts, p->nEdges, Vec_IntSize(p->vPairs)/2 );
    ABC_PRT( "Solving", Abc_Clock() - clk );
    nPairs = Vec_IntSize(p->vPairs)/2;
    Nwk_ManGraphReportMemoryUsage( p );
    Nwk_ManGraphFree( p );
    return nPairs;
}




/**Function*************************************************************

  Synopsis    [Marks the fanins of the node with the current trav ID.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Nwk_ManMarkFanins_rec( Nwk_Obj_t * pLut, int nLevMin )
{
    Nwk_Obj_t * pNext;
    int i;
    if ( !Nwk_ObjIsNode(pLut) )
        return;
    if ( Nwk_ObjIsTravIdCurrent( pLut ) )
        return;
    Nwk_ObjSetTravIdCurrent( pLut );
    if ( Nwk_ObjLevel(pLut) < nLevMin )
        return;
    Nwk_ObjForEachFanin( pLut, pNext, i )
        Nwk_ManMarkFanins_rec( pNext, nLevMin );
}

/**Function*************************************************************

  Synopsis    [Marks the fanouts of the node with the current trav ID.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Nwk_ManMarkFanouts_rec( Nwk_Obj_t * pLut, int nLevMax, int nFanMax )
{
    Nwk_Obj_t * pNext;
    int i;
    if ( !Nwk_ObjIsNode(pLut) )
        return;
    if ( Nwk_ObjIsTravIdCurrent( pLut ) )
        return;
    Nwk_ObjSetTravIdCurrent( pLut );
    if ( Nwk_ObjLevel(pLut) > nLevMax )
        return;
    if ( Nwk_ObjFanoutNum(pLut) > nFanMax )
        return;
    Nwk_ObjForEachFanout( pLut, pNext, i )
        Nwk_ManMarkFanouts_rec( pNext, nLevMax, nFanMax );
}

/**Function*************************************************************

  Synopsis    [Collects the circle of nodes around the given set.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Nwk_ManCollectCircle( Vec_Ptr_t * vStart, Vec_Ptr_t * vNext, int nFanMax )
{
    Nwk_Obj_t * pObj, * pNext;
    int i, k;
    Vec_PtrClear( vNext );
    Vec_PtrForEachEntry( Nwk_Obj_t *, vStart, pObj, i )
    {
        Nwk_ObjForEachFanin( pObj, pNext, k )
        {
            if ( !Nwk_ObjIsNode(pNext) )
                continue;
            if ( Nwk_ObjIsTravIdCurrent( pNext ) )
                continue;
            Nwk_ObjSetTravIdCurrent( pNext );
            Vec_PtrPush( vNext, pNext );
        }
        Nwk_ObjForEachFanout( pObj, pNext, k )
        {
            if ( !Nwk_ObjIsNode(pNext) )
                continue;
            if ( Nwk_ObjIsTravIdCurrent( pNext ) )
                continue;
            Nwk_ObjSetTravIdCurrent( pNext );
            if ( Nwk_ObjFanoutNum(pNext) > nFanMax )
                continue;
            Vec_PtrPush( vNext, pNext );
        }
    }
}

/**Function*************************************************************

  Synopsis    [Collects the circle of nodes removes from the given one.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Nwk_ManCollectNonOverlapCands( Nwk_Obj_t * pLut, Vec_Ptr_t * vStart, Vec_Ptr_t * vNext, Vec_Ptr_t * vCands, Nwk_LMPars_t * pPars )
{
    Vec_Ptr_t * vTemp;
    Nwk_Obj_t * pObj;
    int i, k;
    Vec_PtrClear( vCands );
    if ( pPars->nMaxSuppSize - Nwk_ObjFaninNum(pLut) <= 1 )
        return;

    // collect nodes removed by this distance
    assert( pPars->nMaxDistance > 0 );
    Vec_PtrClear( vStart );
    Vec_PtrPush( vStart, pLut );
    Nwk_ManIncrementTravId( pLut->pMan );
    Nwk_ObjSetTravIdCurrent( pLut );
    for ( i = 1; i <= pPars->nMaxDistance; i++ )
    {
        Nwk_ManCollectCircle( vStart, vNext, pPars->nMaxFanout );
        vTemp  = vStart;
        vStart = vNext;
        vNext  = vTemp;
        // collect the nodes in vStart
        Vec_PtrForEachEntry( Nwk_Obj_t *, vStart, pObj, k )
            Vec_PtrPush( vCands, pObj );
    }

    // mark the TFI/TFO nodes
    Nwk_ManIncrementTravId( pLut->pMan );
    if ( pPars->fUseTfiTfo )
        Nwk_ObjSetTravIdCurrent( pLut );
    else
    {
        Nwk_ObjSetTravIdPrevious( pLut );
        Nwk_ManMarkFanins_rec( pLut, Nwk_ObjLevel(pLut) - pPars->nMaxDistance );
        Nwk_ObjSetTravIdPrevious( pLut );
        Nwk_ManMarkFanouts_rec( pLut, Nwk_ObjLevel(pLut) + pPars->nMaxDistance, pPars->nMaxFanout );
    }

    // collect nodes satisfying the following conditions:
    // - they are close enough in terms of distance
    // - they are not in the TFI/TFO of the LUT
    // - they have no more than the given number of fanins
    // - they have no more than the given diff in delay
    k = 0;
    Vec_PtrForEachEntry( Nwk_Obj_t *, vCands, pObj, i )
    {
        if ( Nwk_ObjIsTravIdCurrent(pObj) )
            continue;
        if ( Nwk_ObjFaninNum(pLut) + Nwk_ObjFaninNum(pObj) > pPars->nMaxSuppSize )
            continue;
        if ( Nwk_ObjLevel(pLut) - Nwk_ObjLevel(pObj) > pPars->nMaxLevelDiff || 
             Nwk_ObjLevel(pObj) - Nwk_ObjLevel(pLut) > pPars->nMaxLevelDiff )
             continue;
        Vec_PtrWriteEntry( vCands, k++, pObj );
    }
    Vec_PtrShrink( vCands, k );
}


/**Function*************************************************************

  Synopsis    [Count the total number of fanins.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Nwk_ManCountTotalFanins( Nwk_Obj_t * pLut, Nwk_Obj_t * pCand )
{
    Nwk_Obj_t * pFanin;
    int i, nCounter = Nwk_ObjFaninNum(pLut);
    Nwk_ObjForEachFanin( pCand, pFanin, i )
        nCounter += !pFanin->MarkC;
    return nCounter;
}

/**Function*************************************************************

  Synopsis    [Collects overlapping candidates.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Nwk_ManCollectOverlapCands( Nwk_Obj_t * pLut, Vec_Ptr_t * vCands, Nwk_LMPars_t * pPars )
{
    Nwk_Obj_t * pFanin, * pObj;
    int i, k;
    // mark fanins of pLut
    Nwk_ObjForEachFanin( pLut, pFanin, i )
        pFanin->MarkC = 1;
    // collect the matching fanouts of each fanin of the node
    Vec_PtrClear( vCands );
    Nwk_ManIncrementTravId( pLut->pMan );
    Nwk_ObjSetTravIdCurrent( pLut );
    Nwk_ObjForEachFanin( pLut, pFanin, i )
    {
        if ( !Nwk_ObjIsNode(pFanin) )
            continue;
        if ( Nwk_ObjFanoutNum(pFanin) > pPars->nMaxFanout )
            continue;
        Nwk_ObjForEachFanout( pFanin, pObj, k )
        {
            if ( !Nwk_ObjIsNode(pObj) )
                continue;
            if ( Nwk_ObjIsTravIdCurrent( pObj ) )
                continue;
            Nwk_ObjSetTravIdCurrent( pObj );
            // check the difference in delay
            if ( Nwk_ObjLevel(pLut) - Nwk_ObjLevel(pObj) > pPars->nMaxLevelDiff || 
                 Nwk_ObjLevel(pObj) - Nwk_ObjLevel(pLut) > pPars->nMaxLevelDiff )
                 continue;
            // check the total number of fanins of the node
            if ( Nwk_ManCountTotalFanins(pLut, pObj) > pPars->nMaxSuppSize )
                continue;
            Vec_PtrPush( vCands, pObj );
        }
    }
    // unmark fanins of pLut
    Nwk_ObjForEachFanin( pLut, pFanin, i )
        pFanin->MarkC = 0;
}

/**Function*************************************************************

  Synopsis    [Performs LUT merging with parameters.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Nwk_ManLutMerge( Nwk_Man_t * pNtk, void * pParsInit )
{
    Nwk_LMPars_t * pPars = (Nwk_LMPars_t *)pParsInit;
    Nwk_Grf_t * p;
    Vec_Int_t * vResult;
    Vec_Ptr_t * vStart, * vNext, * vCands1, * vCands2;
    Nwk_Obj_t * pLut, * pCand;
    int i, k, nVertsMax, nCands;
    abctime clk = Abc_Clock();
    // count the number of vertices
    nVertsMax = 0;
    Nwk_ManForEachNode( pNtk, pLut, i )
        nVertsMax += (int)(Nwk_ObjFaninNum(pLut) <= pPars->nMaxLutSize);
    p = Nwk_ManGraphAlloc( nVertsMax );
    // create graph
    vStart  = Vec_PtrAlloc( 1000 );
    vNext   = Vec_PtrAlloc( 1000 );
    vCands1 = Vec_PtrAlloc( 1000 );
    vCands2 = Vec_PtrAlloc( 1000 );
    nCands  = 0;
    Nwk_ManForEachNode( pNtk, pLut, i )
    {
        if ( Nwk_ObjFaninNum(pLut) > pPars->nMaxLutSize )
            continue;
        Nwk_ManCollectOverlapCands( pLut, vCands1, pPars );
        if ( pPars->fUseDiffSupp )
            Nwk_ManCollectNonOverlapCands( pLut, vStart, vNext, vCands2, pPars );
        if ( Vec_PtrSize(vCands1) == 0 && Vec_PtrSize(vCands2) == 0 )
            continue;
        nCands += Vec_PtrSize(vCands1) + Vec_PtrSize(vCands2);
        // save candidates
        Vec_PtrForEachEntry( Nwk_Obj_t *, vCands1, pCand, k )
            Nwk_ManGraphHashEdge( p, Nwk_ObjId(pLut), Nwk_ObjId(pCand) );
        Vec_PtrForEachEntry( Nwk_Obj_t *, vCands2, pCand, k )
            Nwk_ManGraphHashEdge( p, Nwk_ObjId(pLut), Nwk_ObjId(pCand) );
        // print statistics about this node
        if ( pPars->fVeryVerbose )
        printf( "Node %6d : Fanins = %d. Fanouts = %3d.  Cand1 = %3d. Cand2 = %3d.\n",
            Nwk_ObjId(pLut), Nwk_ObjFaninNum(pLut), Nwk_ObjFaninNum(pLut), 
            Vec_PtrSize(vCands1), Vec_PtrSize(vCands2) );
    }
    Vec_PtrFree( vStart );
    Vec_PtrFree( vNext );
    Vec_PtrFree( vCands1 );
    Vec_PtrFree( vCands2 );
    if ( pPars->fVerbose )
    {
        printf( "Mergable LUTs = %6d. Total cands = %6d. ", p->nVertsMax, nCands );
        ABC_PRT( "Deriving graph", Abc_Clock() - clk );
    }
    // solve the graph problem
    clk = Abc_Clock();
    Nwk_ManGraphSolve( p );
    if ( pPars->fVerbose )
    {
        printf( "GRAPH: Nodes = %6d. Edges = %6d.  Pairs = %6d.  ", 
            p->nVerts, p->nEdges, Vec_IntSize(p->vPairs)/2 );
        ABC_PRT( "Solving", Abc_Clock() - clk );
        Nwk_ManGraphReportMemoryUsage( p );
    }
    vResult = p->vPairs; p->vPairs = NULL;
/*
    for ( i = 0; i < vResult->nSize; i += 2 )
        printf( "(%d,%d) ", vResult->pArray[i], vResult->pArray[i+1] );
    printf( "\n" );
*/
    Nwk_ManGraphFree( p );
    return vResult;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

