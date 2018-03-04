/**CFile****************************************************************

  FileName    [nwkMerge.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Logic network representation.]

  Synopsis    [External declarations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: nwkMerge.h,v 1.1 2008/05/14 22:13:09 wudenni Exp $]

***********************************************************************/
 
#ifndef __NWK_MERGE_H__
#define __NWK_MERGE_H__


////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////



ABC_NAMESPACE_HEADER_START


#define NWK_MAX_LIST  16

////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////

// the LUT merging parameters
typedef struct Nwk_LMPars_t_  Nwk_LMPars_t;
struct Nwk_LMPars_t_
{
    int    nMaxLutSize;    // the max LUT size for merging (N=5)
    int    nMaxSuppSize;   // the max total support size after merging (S=5)
    int    nMaxDistance;   // the max number of nodes separating LUTs
    int    nMaxLevelDiff;  // the max difference in levels
    int    nMaxFanout;     // the max number of fanouts to traverse
    int    fUseDiffSupp;   // enables the use of nodes with different support
    int    fUseTfiTfo;     // enables the use of TFO/TFO nodes as candidates
    int    fVeryVerbose;   // enables additional verbose output
    int    fVerbose;       // enables verbose output
};

// edge of the graph
typedef struct Nwk_Edg_t_  Nwk_Edg_t;
struct Nwk_Edg_t_
{
    int             iNode1;      // the first node
    int             iNode2;      // the second node
    Nwk_Edg_t *     pNext;       // the next edge
};

// vertex of the graph
typedef struct Nwk_Vrt_t_  Nwk_Vrt_t;
struct Nwk_Vrt_t_
{
    int             Id;          // the vertex number
    int             iPrev;       // the previous vertex in the list
    int             iNext;       // the next vertex in the list
    int             nEdges;      // the number of edges
    int             pEdges[0];   // the array of edges
};

// the connectivity graph
typedef struct Nwk_Grf_t_  Nwk_Grf_t;
struct Nwk_Grf_t_
{
    // preliminary graph representation
    int             nObjs;       // the number of objects
    int             nVertsMax;   // the upper bound on the number of vertices
    int             nEdgeHash;   // an approximate number of edges
    Nwk_Edg_t **    pEdgeHash;   // hash table for edges
    Aig_MmFixed_t * pMemEdges;   // memory for edges
    // graph representation
    int             nEdges;      // the number of edges
    int             nVerts;      // the number of vertices
    Nwk_Vrt_t **    pVerts;      // the array of vertices
    Aig_MmFlex_t *  pMemVerts;   // memory for vertices
    // intermediate data
    int pLists1[NWK_MAX_LIST+1]; // lists of nodes with one edge
    int pLists2[NWK_MAX_LIST+1]; // lists of nodes with more than one edge
    // the results of matching
    Vec_Int_t *     vPairs;      // pairs matched in the graph
    // object mappings
    int *           pMapLut2Id;  // LUT numbers into vertex IDs
    int *           pMapId2Lut;  // vertex IDs into LUT numbers
    // other things
    int             nMemBytes1;  // memory usage in bytes
    int             nMemBytes2;  // memory usage in bytes
};

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

#define Nwk_GraphForEachEdge( p, pEdge, k )             \
    for ( k = 0; k < p->nEdgeHash; k++ )                \
        for ( pEdge = p->pEdgeHash[k]; pEdge; pEdge = pEdge->pNext )

#define Nwk_ListForEachVertex( p, List, pVrt )          \
    for ( pVrt = List? p->pVerts[List] : NULL;  pVrt;   \
          pVrt = pVrt->iNext? p->pVerts[pVrt->iNext] : NULL )

#define Nwk_VertexForEachAdjacent( p, pVrt, pNext, k )  \
    for ( k = 0; (k < pVrt->nEdges) && (((pNext) = p->pVerts[pVrt->pEdges[k]]), 1); k++ )

////////////////////////////////////////////////////////////////////////
///                      INLINED FUNCTIONS                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                         ITERATORS                                ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== nwkMerge.c ==========================================================*/
extern ABC_DLL Nwk_Grf_t *   Nwk_ManGraphAlloc( int nVertsMax );
extern ABC_DLL void          Nwk_ManGraphFree( Nwk_Grf_t * p );
extern ABC_DLL void          Nwk_ManGraphReportMemoryUsage( Nwk_Grf_t * p );
extern ABC_DLL void          Nwk_ManGraphHashEdge( Nwk_Grf_t * p, int iLut1, int iLut2 );
extern ABC_DLL void          Nwk_ManGraphSolve( Nwk_Grf_t * p );
extern ABC_DLL int           Nwk_ManLutMergeGraphTest( char * pFileName );



ABC_NAMESPACE_HEADER_END



#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

