/**CFile****************************************************************

  FileName    [rwtDec.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [DAG-aware AIG rewriting package.]

  Synopsis    [Evaluation and decomposition procedures.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: rwtDec.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "rwt.h"
#include "bool/deco/deco.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static Dec_Graph_t * Rwt_NodePreprocess( Rwt_Man_t * p, Rwt_Node_t * pNode );
static Dec_Edge_t    Rwt_TravCollect_rec( Rwt_Man_t * p, Rwt_Node_t * pNode, Dec_Graph_t * pGraph );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Preprocesses computed library of subgraphs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Rwt_ManPreprocess( Rwt_Man_t * p )
{
    Dec_Graph_t * pGraph;
    Rwt_Node_t * pNode;
    int i, k;
    // put the nodes into the structure
    p->pMapInv  = ABC_ALLOC( unsigned short, 222 );
    memset( p->pMapInv, 0, sizeof(unsigned short) * 222 );
    p->vClasses = Vec_VecStart( 222 );
    for ( i = 0; i < p->nFuncs; i++ )
    {
        if ( p->pTable[i] == NULL )
            continue;
        // consider all implementations of this function
        for ( pNode = p->pTable[i]; pNode; pNode = pNode->pNext )
        {
            assert( pNode->uTruth == p->pTable[i]->uTruth );
            assert( p->pMap[pNode->uTruth] < 222 ); // Always >= 0 b/c unsigned.
            Vec_VecPush( p->vClasses, p->pMap[pNode->uTruth], pNode );
            p->pMapInv[ p->pMap[pNode->uTruth] ] = p->puCanons[pNode->uTruth];
        }
    }
    // compute decomposition forms for each node and verify them
    Vec_VecForEachEntry( Rwt_Node_t *, p->vClasses, pNode, i, k )
    {
        pGraph = Rwt_NodePreprocess( p, pNode );
        pNode->pNext = (Rwt_Node_t *)pGraph;
//        assert( pNode->uTruth == (Dec_GraphDeriveTruth(pGraph) & 0xFFFF) );
    }
}

/**Function*************************************************************

  Synopsis    [Preprocesses subgraphs rooted at this node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Dec_Graph_t * Rwt_NodePreprocess( Rwt_Man_t * p, Rwt_Node_t * pNode )
{
    Dec_Graph_t * pGraph;
    Dec_Edge_t eRoot;
    assert( !Rwt_IsComplement(pNode) );
    // consider constant
    if ( pNode->uTruth == 0 )
        return Dec_GraphCreateConst0();
    // consider the case of elementary var
    if ( pNode->uTruth == 0x00FF )
        return Dec_GraphCreateLeaf( 3, 4, 1 );
    // start the subgraphs
    pGraph = Dec_GraphCreate( 4 );
    // collect the nodes
    Rwt_ManIncTravId( p );
    eRoot = Rwt_TravCollect_rec( p, pNode, pGraph );
    Dec_GraphSetRoot( pGraph, eRoot );
    return pGraph;
}

/**Function*************************************************************

  Synopsis    [Adds one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Dec_Edge_t Rwt_TravCollect_rec( Rwt_Man_t * p, Rwt_Node_t * pNode, Dec_Graph_t * pGraph )
{
    Dec_Edge_t eNode0, eNode1, eNode;
    // elementary variable
    if ( pNode->fUsed )
        return Dec_EdgeCreate( pNode->Id - 1, 0 );
    // previously visited node
    if ( pNode->TravId == p->nTravIds )
        return Dec_IntToEdge( pNode->Volume );
    pNode->TravId = p->nTravIds;
    // solve for children
    eNode0 = Rwt_TravCollect_rec( p, Rwt_Regular(pNode->p0), pGraph );
    if ( Rwt_IsComplement(pNode->p0) )    
        eNode0.fCompl = !eNode0.fCompl;
    eNode1 = Rwt_TravCollect_rec( p, Rwt_Regular(pNode->p1), pGraph );
    if ( Rwt_IsComplement(pNode->p1) )    
        eNode1.fCompl = !eNode1.fCompl;
    // create the decomposition node(s)
    if ( pNode->fExor )
        eNode = Dec_GraphAddNodeXor( pGraph, eNode0, eNode1, 0 );
    else
        eNode = Dec_GraphAddNodeAnd( pGraph, eNode0, eNode1 );
    // save the result
    pNode->Volume = Dec_EdgeToInt( eNode );
    return eNode;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

