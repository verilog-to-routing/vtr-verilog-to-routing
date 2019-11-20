/**CFile****************************************************************

  FileName    [kitHop.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Computation kit.]

  Synopsis    [Procedures involving AIGs.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - Dec 6, 2006.]

  Revision    [$Id: kitHop.c,v 1.00 2006/12/06 00:00:00 alanmi Exp $]

***********************************************************************/

#include "kit.h"
#include "hop.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////


/**Function*************************************************************

  Synopsis    [Transforms the decomposition graph into the AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Hop_Obj_t * Kit_GraphToHopInternal( Hop_Man_t * pMan, Kit_Graph_t * pGraph )
{
    Kit_Node_t * pNode;
    Hop_Obj_t * pAnd0, * pAnd1;
    int i;
    // check for constant function
    if ( Kit_GraphIsConst(pGraph) )
        return Hop_NotCond( Hop_ManConst1(pMan), Kit_GraphIsComplement(pGraph) );
    // check for a literal
    if ( Kit_GraphIsVar(pGraph) )
        return Hop_NotCond( Kit_GraphVar(pGraph)->pFunc, Kit_GraphIsComplement(pGraph) );
    // build the AIG nodes corresponding to the AND gates of the graph
    Kit_GraphForEachNode( pGraph, pNode, i )
    {
        pAnd0 = Hop_NotCond( Kit_GraphNode(pGraph, pNode->eEdge0.Node)->pFunc, pNode->eEdge0.fCompl ); 
        pAnd1 = Hop_NotCond( Kit_GraphNode(pGraph, pNode->eEdge1.Node)->pFunc, pNode->eEdge1.fCompl ); 
        pNode->pFunc = Hop_And( pMan, pAnd0, pAnd1 );
    }
    // complement the result if necessary
    return Hop_NotCond( pNode->pFunc, Kit_GraphIsComplement(pGraph) );
}

/**Function*************************************************************

  Synopsis    [Strashes one logic node using its SOP.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Hop_Obj_t * Kit_GraphToHop( Hop_Man_t * pMan, Kit_Graph_t * pGraph )
{
    Kit_Node_t * pNode;
    int i;
    // collect the fanins
    Kit_GraphForEachLeaf( pGraph, pNode, i )
        pNode->pFunc = Hop_IthVar( pMan, i );
    // perform strashing
    return Kit_GraphToHopInternal( pMan, pGraph );
}

/**Function*************************************************************

  Synopsis    [Strashes one logic node using its SOP.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Hop_Obj_t * Kit_CoverToHop( Hop_Man_t * pMan, Vec_Int_t * vCover, int nVars, Vec_Int_t * vMemory )
{
    Kit_Graph_t * pGraph;
    Hop_Obj_t * pFunc;
    // perform factoring
    pGraph = Kit_SopFactor( vCover, 0, nVars, vMemory );
    // convert graph to the AIG
    pFunc = Kit_GraphToHop( pMan, pGraph );
    Kit_GraphFree( pGraph );
    return pFunc;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


