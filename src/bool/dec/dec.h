/**CFile****************************************************************

  FileName    [dec.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [A simple decomposition tree/node data structure and its APIs.]

  Synopsis    [External declarations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: dec.h,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/
 
#ifndef ABC__opt__dec__dec_h
#define ABC__opt__dec__dec_h


////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////



ABC_NAMESPACE_HEADER_START


////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Dec_Edge_t_ Dec_Edge_t;
struct Dec_Edge_t_
{
    unsigned          fCompl   :  1;   // the complemented bit
    unsigned          Node     : 30;   // the decomposition node pointed by the edge
};

typedef struct Dec_Node_t_ Dec_Node_t;
struct Dec_Node_t_
{
    Dec_Edge_t        eEdge0;          // the left child of the node
    Dec_Edge_t        eEdge1;          // the right child of the node
    // other info
    union { int       iFunc;           // the literal of the node (AIG)
    void *            pFunc; };        // the function of the node (BDD or AIG)
    unsigned          Level    : 14;   // the level of this node in the global AIG
    // printing info 
    unsigned          fNodeOr  :  1;   // marks the original OR node
    unsigned          fCompl0  :  1;   // marks the original complemented edge
    unsigned          fCompl1  :  1;   // marks the original complemented edge
    // latch info
    unsigned          nLat0    :  5;   // the number of latches on the first edge
    unsigned          nLat1    :  5;   // the number of latches on the second edge
    unsigned          nLat2    :  5;   // the number of latches on the output edge
};

typedef struct Dec_Graph_t_ Dec_Graph_t;
struct Dec_Graph_t_
{
    int               fConst;          // marks the constant 1 graph
    int               nLeaves;         // the number of leaves
    int               nSize;           // the number of nodes (including the leaves) 
    int               nCap;            // the number of allocated nodes
    Dec_Node_t *      pNodes;          // the array of leaves and internal nodes
    Dec_Edge_t        eRoot;           // the pointer to the topmost node
};

typedef struct Dec_Man_t_ Dec_Man_t;
struct Dec_Man_t_
{
    void *            pMvcMem;         // memory manager for MVC cover (used for factoring)
    Vec_Int_t *       vCubes;          // storage for cubes
    Vec_Int_t *       vLits;           // storage for literals 
    // precomputation information about 4-variable functions
    unsigned short *  puCanons;        // canonical forms
    char *            pPhases;         // canonical phases
    char *            pPerms;          // canonical permutations
    unsigned char *   pMap;            // mapping of functions into class numbers
};


////////////////////////////////////////////////////////////////////////
///                        ITERATORS                                 ///
////////////////////////////////////////////////////////////////////////

// interator throught the leaves
#define Dec_GraphForEachLeaf( pGraph, pLeaf, i )                                              \
    for ( i = 0; (i < (pGraph)->nLeaves) && (((pLeaf) = Dec_GraphNode(pGraph, i)), 1); i++ )
// interator throught the internal nodes
#define Dec_GraphForEachNode( pGraph, pAnd, i )                                               \
    for ( i = (pGraph)->nLeaves; (i < (pGraph)->nSize) && (((pAnd) = Dec_GraphNode(pGraph, i)), 1); i++ )

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== decAbc.c ========================================================*/
/*=== decFactor.c ========================================================*/
extern Dec_Graph_t *  Dec_Factor( char * pSop );
/*=== decMan.c ========================================================*/
extern Dec_Man_t *    Dec_ManStart();
extern void           Dec_ManStop( Dec_Man_t * p );
/*=== decPrint.c ========================================================*/
extern void           Dec_GraphPrint( FILE * pFile, Dec_Graph_t * pGraph, char * pNamesIn[], char * pNameOut );
/*=== decUtil.c ========================================================*/
extern unsigned       Dec_GraphDeriveTruth( Dec_Graph_t * pGraph );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Creates an edge pointing to the node in the given polarity.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Dec_Edge_t Dec_EdgeCreate( int Node, int fCompl )   
{
    Dec_Edge_t eEdge = { fCompl, Node }; 
    return eEdge; 
}

/**Function*************************************************************

  Synopsis    [Converts the edge into unsigned integer.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline unsigned Dec_EdgeToInt( Dec_Edge_t eEdge )   
{
    return (eEdge.Node << 1) | eEdge.fCompl; 
}

/**Function*************************************************************

  Synopsis    [Converts unsigned integer into the edge.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Dec_Edge_t Dec_IntToEdge( unsigned Edge )   
{
    return Dec_EdgeCreate( Edge >> 1, Edge & 1 ); 
}

/**Function*************************************************************

  Synopsis    [Converts the edge into unsigned integer.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline unsigned    Dec_EdgeToInt_( Dec_Edge_t m )  { union { Dec_Edge_t x; unsigned y; } v; v.x = m; return v.y;  }
/*
static inline unsigned Dec_EdgeToInt_( Dec_Edge_t eEdge )   
{
    return *(unsigned *)&eEdge;
}
*/

/**Function*************************************************************

  Synopsis    [Converts unsigned integer into the edge.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Dec_Edge_t  Dec_IntToEdge_( unsigned m )    { union { Dec_Edge_t x; unsigned y; } v; v.y = m; return v.x;  }
/*
static inline Dec_Edge_t Dec_IntToEdge_( unsigned Edge )   
{
    return *(Dec_Edge_t *)&Edge;
}
*/

/**Function*************************************************************

  Synopsis    [Creates a graph with the given number of leaves.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Dec_Graph_t * Dec_GraphCreate( int nLeaves )   
{
    Dec_Graph_t * pGraph;
    pGraph = ABC_ALLOC( Dec_Graph_t, 1 );
    memset( pGraph, 0, sizeof(Dec_Graph_t) );
    pGraph->nLeaves = nLeaves;
    pGraph->nSize = nLeaves;
    pGraph->nCap = 2 * nLeaves + 50;
    pGraph->pNodes = ABC_ALLOC( Dec_Node_t, pGraph->nCap );
    memset( pGraph->pNodes, 0, sizeof(Dec_Node_t) * pGraph->nSize );
    return pGraph;
}

/**Function*************************************************************

  Synopsis    [Creates constant 0 graph.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Dec_Graph_t * Dec_GraphCreateConst0()   
{
    Dec_Graph_t * pGraph;
    pGraph = ABC_ALLOC( Dec_Graph_t, 1 );
    memset( pGraph, 0, sizeof(Dec_Graph_t) );
    pGraph->fConst = 1;
    pGraph->eRoot.fCompl = 1;
    return pGraph;
}

/**Function*************************************************************

  Synopsis    [Creates constant 1 graph.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Dec_Graph_t * Dec_GraphCreateConst1()   
{
    Dec_Graph_t * pGraph;
    pGraph = ABC_ALLOC( Dec_Graph_t, 1 );
    memset( pGraph, 0, sizeof(Dec_Graph_t) );
    pGraph->fConst = 1;
    return pGraph;
}

/**Function*************************************************************

  Synopsis    [Creates the literal graph.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Dec_Graph_t * Dec_GraphCreateLeaf( int iLeaf, int nLeaves, int fCompl )   
{
    Dec_Graph_t * pGraph;
    assert( 0 <= iLeaf && iLeaf < nLeaves );
    pGraph = Dec_GraphCreate( nLeaves );
    pGraph->eRoot.Node   = iLeaf;
    pGraph->eRoot.fCompl = fCompl;
    return pGraph;
}

/**Function*************************************************************

  Synopsis    [Creates a graph with the given number of leaves.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Dec_GraphFree( Dec_Graph_t * pGraph )   
{
    ABC_FREE( pGraph->pNodes );
    ABC_FREE( pGraph );
}

/**Function*************************************************************

  Synopsis    [Returns 1 if the graph is a constant.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Dec_GraphIsConst( Dec_Graph_t * pGraph )   
{
    return pGraph->fConst;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if the graph is constant 0.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Dec_GraphIsConst0( Dec_Graph_t * pGraph )   
{
    return pGraph->fConst && pGraph->eRoot.fCompl;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if the graph is constant 1.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Dec_GraphIsConst1( Dec_Graph_t * pGraph )   
{
    return pGraph->fConst && !pGraph->eRoot.fCompl;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if the graph is complemented.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Dec_GraphIsComplement( Dec_Graph_t * pGraph )   
{
    return pGraph->eRoot.fCompl;
}

/**Function*************************************************************

  Synopsis    [Checks if the graph is complemented.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Dec_GraphComplement( Dec_Graph_t * pGraph )   
{
    pGraph->eRoot.fCompl ^= 1;
}


/**Function*************************************************************

  Synopsis    [Returns the number of leaves.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Dec_GraphLeaveNum( Dec_Graph_t * pGraph )   
{
    return pGraph->nLeaves;
}

/**Function*************************************************************

  Synopsis    [Returns the number of internal nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Dec_GraphNodeNum( Dec_Graph_t * pGraph )   
{
    return pGraph->nSize - pGraph->nLeaves;
}

/**Function*************************************************************

  Synopsis    [Returns the pointer to the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Dec_Node_t * Dec_GraphNode( Dec_Graph_t * pGraph, int i )   
{
    return pGraph->pNodes + i;
}

/**Function*************************************************************

  Synopsis    [Returns the pointer to the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Dec_Node_t * Dec_GraphNodeLast( Dec_Graph_t * pGraph )   
{
    return pGraph->pNodes + pGraph->nSize - 1;
}

/**Function*************************************************************

  Synopsis    [Returns the number of the given node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Dec_GraphNodeInt( Dec_Graph_t * pGraph, Dec_Node_t * pNode )   
{
    return pNode - pGraph->pNodes;
}

/**Function*************************************************************

  Synopsis    [Check if the graph represents elementary variable.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Dec_GraphIsVar( Dec_Graph_t * pGraph )   
{
    return pGraph->eRoot.Node < (unsigned)pGraph->nLeaves;
}

/**Function*************************************************************

  Synopsis    [Check if the graph represents elementary variable.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Dec_GraphNodeIsVar( Dec_Graph_t * pGraph, Dec_Node_t * pNode )   
{
    return Dec_GraphNodeInt(pGraph,pNode) < pGraph->nLeaves;
}

/**Function*************************************************************

  Synopsis    [Returns the elementary variable elementary variable.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Dec_Node_t * Dec_GraphVar( Dec_Graph_t * pGraph )   
{
    assert( Dec_GraphIsVar( pGraph ) );
    return Dec_GraphNode( pGraph, pGraph->eRoot.Node );
}

/**Function*************************************************************

  Synopsis    [Returns the number of the elementary variable.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Dec_GraphVarInt( Dec_Graph_t * pGraph )   
{
    assert( Dec_GraphIsVar( pGraph ) );
    return Dec_GraphNodeInt( pGraph, Dec_GraphVar(pGraph) );
}

/**Function*************************************************************

  Synopsis    [Sets the root of the graph.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Dec_GraphSetRoot( Dec_Graph_t * pGraph, Dec_Edge_t eRoot )   
{
    pGraph->eRoot = eRoot;
}

/**Function*************************************************************

  Synopsis    [Appends a new node to the graph.]

  Description [This procedure is meant for internal use.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Dec_Node_t * Dec_GraphAppendNode( Dec_Graph_t * pGraph )   
{
    Dec_Node_t * pNode;
    if ( pGraph->nSize == pGraph->nCap )
    {
        pGraph->pNodes = ABC_REALLOC( Dec_Node_t, pGraph->pNodes, 2 * pGraph->nCap ); 
        pGraph->nCap   = 2 * pGraph->nCap;
    }
    pNode = pGraph->pNodes + pGraph->nSize++;
    memset( pNode, 0, sizeof(Dec_Node_t) );
    return pNode;
}

/**Function*************************************************************

  Synopsis    [Creates an AND node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Dec_Edge_t Dec_GraphAddNodeAnd( Dec_Graph_t * pGraph, Dec_Edge_t eEdge0, Dec_Edge_t eEdge1 )
{
    Dec_Node_t * pNode;
    // get the new node
    pNode = Dec_GraphAppendNode( pGraph );
    // set the inputs and other info
    pNode->eEdge0 = eEdge0;
    pNode->eEdge1 = eEdge1;
    pNode->fCompl0 = eEdge0.fCompl;
    pNode->fCompl1 = eEdge1.fCompl;
    return Dec_EdgeCreate( pGraph->nSize - 1, 0 );
}

/**Function*************************************************************

  Synopsis    [Creates an OR node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Dec_Edge_t Dec_GraphAddNodeOr( Dec_Graph_t * pGraph, Dec_Edge_t eEdge0, Dec_Edge_t eEdge1 )
{
    Dec_Node_t * pNode;
    // get the new node
    pNode = Dec_GraphAppendNode( pGraph );
    // set the inputs and other info
    pNode->eEdge0 = eEdge0;
    pNode->eEdge1 = eEdge1;
    pNode->fCompl0 = eEdge0.fCompl;
    pNode->fCompl1 = eEdge1.fCompl;
    // make adjustments for the OR gate
    pNode->fNodeOr = 1;
    pNode->eEdge0.fCompl = !pNode->eEdge0.fCompl;
    pNode->eEdge1.fCompl = !pNode->eEdge1.fCompl;
    return Dec_EdgeCreate( pGraph->nSize - 1, 1 );
}

/**Function*************************************************************

  Synopsis    [Creates an XOR node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Dec_Edge_t Dec_GraphAddNodeXor( Dec_Graph_t * pGraph, Dec_Edge_t eEdge0, Dec_Edge_t eEdge1, int Type )
{
    Dec_Edge_t eNode0, eNode1, eNode;
    if ( Type == 0 )
    {
        // derive the first AND
        eEdge0.fCompl ^= 1;
        eNode0 = Dec_GraphAddNodeAnd( pGraph, eEdge0, eEdge1 );
        eEdge0.fCompl ^= 1;
        // derive the second AND
        eEdge1.fCompl ^= 1;
        eNode1 = Dec_GraphAddNodeAnd( pGraph, eEdge0, eEdge1 );
        // derive the final OR
        eNode = Dec_GraphAddNodeOr( pGraph, eNode0, eNode1 );
    }
    else
    {
        // derive the first AND
        eNode0 = Dec_GraphAddNodeAnd( pGraph, eEdge0, eEdge1 );
        // derive the second AND
        eEdge0.fCompl ^= 1;
        eEdge1.fCompl ^= 1;
        eNode1 = Dec_GraphAddNodeAnd( pGraph, eEdge0, eEdge1 );
        // derive the final OR
        eNode = Dec_GraphAddNodeOr( pGraph, eNode0, eNode1 );
        eNode.fCompl ^= 1;
    }
    return eNode;
}

/**Function*************************************************************

  Synopsis    [Creates an XOR node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Dec_Edge_t Dec_GraphAddNodeMux( Dec_Graph_t * pGraph, Dec_Edge_t eEdgeC, Dec_Edge_t eEdgeT, Dec_Edge_t eEdgeE, int Type )
{
    Dec_Edge_t eNode0, eNode1, eNode;
    if ( Type == 0 )
    {
        // derive the first AND
        eNode0 = Dec_GraphAddNodeAnd( pGraph, eEdgeC, eEdgeT );
        // derive the second AND
        eEdgeC.fCompl ^= 1;
        eNode1 = Dec_GraphAddNodeAnd( pGraph, eEdgeC, eEdgeE );
        // derive the final OR
        eNode = Dec_GraphAddNodeOr( pGraph, eNode0, eNode1 );
    }
    else
    {
        // complement the arguments
        eEdgeT.fCompl ^= 1;
        eEdgeE.fCompl ^= 1;
        // derive the first AND
        eNode0 = Dec_GraphAddNodeAnd( pGraph, eEdgeC, eEdgeT );
        // derive the second AND
        eEdgeC.fCompl ^= 1;
        eNode1 = Dec_GraphAddNodeAnd( pGraph, eEdgeC, eEdgeE );
        // derive the final OR
        eNode = Dec_GraphAddNodeOr( pGraph, eNode0, eNode1 );
        eNode.fCompl ^= 1;
    }
    return eNode;
}



ABC_NAMESPACE_HEADER_END



#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

