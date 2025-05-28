/**CFile****************************************************************

  FileName    [kitAig.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Computation kit.]

  Synopsis    [Procedures involving AIGs.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - Dec 6, 2006.]

  Revision    [$Id: kitAig.c,v 1.00 2006/12/06 00:00:00 alanmi Exp $]

***********************************************************************/

#include "kit.h"
#include "aig/aig/aig.h"

ABC_NAMESPACE_IMPL_START


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
Aig_Obj_t * Kit_GraphToAigInternal( Aig_Man_t * pMan, Kit_Graph_t * pGraph )
{
    Kit_Node_t * pNode = NULL;
    Aig_Obj_t * pAnd0, * pAnd1;
    int i;
    // check for constant function
    if ( Kit_GraphIsConst(pGraph) )
        return Aig_NotCond( Aig_ManConst1(pMan), Kit_GraphIsComplement(pGraph) );
    // check for a literal
    if ( Kit_GraphIsVar(pGraph) )
        return Aig_NotCond( (Aig_Obj_t *)Kit_GraphVar(pGraph)->pFunc, Kit_GraphIsComplement(pGraph) );
    // build the AIG nodes corresponding to the AND gates of the graph
    Kit_GraphForEachNode( pGraph, pNode, i )
    {
        pAnd0 = Aig_NotCond( (Aig_Obj_t *)Kit_GraphNode(pGraph, pNode->eEdge0.Node)->pFunc, pNode->eEdge0.fCompl ); 
        pAnd1 = Aig_NotCond( (Aig_Obj_t *)Kit_GraphNode(pGraph, pNode->eEdge1.Node)->pFunc, pNode->eEdge1.fCompl ); 
        pNode->pFunc = Aig_And( pMan, pAnd0, pAnd1 );
    }
    // complement the result if necessary
    return Aig_NotCond( (Aig_Obj_t *)pNode->pFunc, Kit_GraphIsComplement(pGraph) );
}

/**Function*************************************************************

  Synopsis    [Strashes one logic node using its SOP.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Obj_t * Kit_GraphToAig( Aig_Man_t * pMan, Aig_Obj_t ** pFanins, Kit_Graph_t * pGraph )
{
    Kit_Node_t * pNode = NULL;
    int i;
    // collect the fanins
    Kit_GraphForEachLeaf( pGraph, pNode, i )
        pNode->pFunc = pFanins[i];
    // perform strashing
    return Kit_GraphToAigInternal( pMan, pGraph );
}

/**Function*************************************************************

  Synopsis    [Strashed onen logic nodes using its truth table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Obj_t * Kit_TruthToAig( Aig_Man_t * pMan, Aig_Obj_t ** pFanins, unsigned * pTruth, int nVars, Vec_Int_t * vMemory )
{
    Aig_Obj_t * pObj;
    Kit_Graph_t * pGraph;
    // transform truth table into the decomposition tree
    if ( vMemory == NULL )
    {
        vMemory = Vec_IntAlloc( 0 );
        pGraph = Kit_TruthToGraph( pTruth, nVars, vMemory );
        Vec_IntFree( vMemory );
    }
    else
        pGraph = Kit_TruthToGraph( pTruth, nVars, vMemory );
    // derive the AIG for the decomposition tree
    pObj = Kit_GraphToAig( pMan, pFanins, pGraph );
    Kit_GraphFree( pGraph );
    return pObj;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

