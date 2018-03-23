/**CFile****************************************************************

  FileName    [decUtil.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Decomposition unitilies.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: decUtil.c,v 1.1 2003/05/22 19:20:05 alanmi Exp $]

***********************************************************************/

#include "base/abc/abc.h"
#include "dec.h"

#ifdef ABC_USE_CUDD
#include "bdd/extrab/extraBdd.h"
#endif

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

#ifdef ABC_USE_CUDD

/**Function*************************************************************

  Synopsis    [Converts graph to BDD.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Dec_GraphDeriveBdd( DdManager * dd, Dec_Graph_t * pGraph )
{
    DdNode * bFunc, * bFunc0, * bFunc1;
    Dec_Node_t * pNode = NULL; // Suppress "might be used uninitialized"
    int i;

    // sanity checks
    assert( Dec_GraphLeaveNum(pGraph) >= 0 );
    assert( Dec_GraphLeaveNum(pGraph) <= pGraph->nSize );

    // check for constant function
    if ( Dec_GraphIsConst(pGraph) )
        return Cudd_NotCond( b1, Dec_GraphIsComplement(pGraph) );
    // check for a literal
    if ( Dec_GraphIsVar(pGraph) )
        return Cudd_NotCond( Cudd_bddIthVar(dd, Dec_GraphVarInt(pGraph)), Dec_GraphIsComplement(pGraph) );

    // assign the elementary variables
    Dec_GraphForEachLeaf( pGraph, pNode, i )
        pNode->pFunc = Cudd_bddIthVar( dd, i );

    // compute the function for each internal node
    Dec_GraphForEachNode( pGraph, pNode, i )
    {
        bFunc0 = Cudd_NotCond( Dec_GraphNode(pGraph, pNode->eEdge0.Node)->pFunc, pNode->eEdge0.fCompl ); 
        bFunc1 = Cudd_NotCond( Dec_GraphNode(pGraph, pNode->eEdge1.Node)->pFunc, pNode->eEdge1.fCompl ); 
        pNode->pFunc = Cudd_bddAnd( dd, bFunc0, bFunc1 );   Cudd_Ref( (DdNode *)pNode->pFunc );
    }

    // deref the intermediate results
    bFunc = (DdNode *)pNode->pFunc;   Cudd_Ref( bFunc );
    Dec_GraphForEachNode( pGraph, pNode, i )
        Cudd_RecursiveDeref( dd, (DdNode *)pNode->pFunc );
    Cudd_Deref( bFunc );

    // complement the result if necessary
    return Cudd_NotCond( bFunc, Dec_GraphIsComplement(pGraph) );
}

#endif

/**Function*************************************************************

  Synopsis    [Derives the truth table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned Dec_GraphDeriveTruth( Dec_Graph_t * pGraph )
{
    unsigned uTruths[5] = { 0xAAAAAAAA, 0xCCCCCCCC, 0xF0F0F0F0, 0xFF00FF00, 0xFFFF0000 };
    unsigned uTruth = 0; // Suppress "might be used uninitialized"
    unsigned uTruth0, uTruth1;
    Dec_Node_t * pNode;
    int i;

    // sanity checks
    assert( Dec_GraphLeaveNum(pGraph) >= 0 );
    assert( Dec_GraphLeaveNum(pGraph) <= pGraph->nSize );
    assert( Dec_GraphLeaveNum(pGraph) <= 5 );

    // check for constant function
    if ( Dec_GraphIsConst(pGraph) )
        return Dec_GraphIsComplement(pGraph)? 0 : ~((unsigned)0);
    // check for a literal
    if ( Dec_GraphIsVar(pGraph) )
        return Dec_GraphIsComplement(pGraph)? ~uTruths[Dec_GraphVarInt(pGraph)] : uTruths[Dec_GraphVarInt(pGraph)];

    // assign the elementary variables
    Dec_GraphForEachLeaf( pGraph, pNode, i )
        pNode->pFunc = (void *)(ABC_PTRUINT_T)uTruths[i];

    // compute the function for each internal node
    Dec_GraphForEachNode( pGraph, pNode, i )
    {
        uTruth0 = (unsigned)(ABC_PTRUINT_T)Dec_GraphNode(pGraph, pNode->eEdge0.Node)->pFunc;
        uTruth1 = (unsigned)(ABC_PTRUINT_T)Dec_GraphNode(pGraph, pNode->eEdge1.Node)->pFunc;
        uTruth0 = pNode->eEdge0.fCompl? ~uTruth0 : uTruth0;
        uTruth1 = pNode->eEdge1.fCompl? ~uTruth1 : uTruth1;
        uTruth = uTruth0 & uTruth1;
        pNode->pFunc = (void *)(ABC_PTRUINT_T)uTruth;
    }

    // complement the result if necessary
    return Dec_GraphIsComplement(pGraph)? ~uTruth : uTruth;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

