/**CFile****************************************************************

  FileName    [kitBdd.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Computation kit.]

  Synopsis    [Procedures involving BDDs.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - Dec 6, 2006.]

  Revision    [$Id: kitBdd.c,v 1.00 2006/12/06 00:00:00 alanmi Exp $]

***********************************************************************/

#include "kit.h"

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

  Synopsis    [Derives the BDD for the given SOP.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Kit_SopToBdd( DdManager * dd, Kit_Sop_t * cSop, int nVars )
{
    DdNode * bSum, * bCube, * bTemp, * bVar;
    unsigned uCube;
    int Value, i, v;
    assert( nVars < 16 );
    // start the cover
    bSum = Cudd_ReadLogicZero(dd);   Cudd_Ref( bSum );
   // check the logic function of the node
    Kit_SopForEachCube( cSop, uCube, i )
    {
        bCube = Cudd_ReadOne(dd);   Cudd_Ref( bCube );
        for ( v = 0; v < nVars; v++ )
        {
            Value = ((uCube >> 2*v) & 3);
            if ( Value == 1 )
                bVar = Cudd_Not( Cudd_bddIthVar( dd, v ) );
            else if ( Value == 2 )
                bVar = Cudd_bddIthVar( dd, v );
            else
                continue;
            bCube  = Cudd_bddAnd( dd, bTemp = bCube, bVar );   Cudd_Ref( bCube );
            Cudd_RecursiveDeref( dd, bTemp );
        }
        bSum = Cudd_bddOr( dd, bTemp = bSum, bCube );   
        Cudd_Ref( bSum );
        Cudd_RecursiveDeref( dd, bTemp );
        Cudd_RecursiveDeref( dd, bCube );
    }
    // complement the result if necessary
    Cudd_Deref( bSum );
    return bSum;
}

/**Function*************************************************************

  Synopsis    [Converts graph to BDD.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Kit_GraphToBdd( DdManager * dd, Kit_Graph_t * pGraph )
{
    DdNode * bFunc, * bFunc0, * bFunc1;
    Kit_Node_t * pNode = NULL; // Suppress "might be used uninitialized"
    int i;

    // sanity checks
    assert( Kit_GraphLeaveNum(pGraph) >= 0 );
    assert( Kit_GraphLeaveNum(pGraph) <= pGraph->nSize );

    // check for constant function
    if ( Kit_GraphIsConst(pGraph) )
        return Cudd_NotCond( b1, Kit_GraphIsComplement(pGraph) );
    // check for a literal
    if ( Kit_GraphIsVar(pGraph) )
        return Cudd_NotCond( Cudd_bddIthVar(dd, Kit_GraphVarInt(pGraph)), Kit_GraphIsComplement(pGraph) );

    // assign the elementary variables
    Kit_GraphForEachLeaf( pGraph, pNode, i )
        pNode->pFunc = Cudd_bddIthVar( dd, i );

    // compute the function for each internal node
    Kit_GraphForEachNode( pGraph, pNode, i )
    {
        bFunc0 = Cudd_NotCond( Kit_GraphNode(pGraph, pNode->eEdge0.Node)->pFunc, pNode->eEdge0.fCompl ); 
        bFunc1 = Cudd_NotCond( Kit_GraphNode(pGraph, pNode->eEdge1.Node)->pFunc, pNode->eEdge1.fCompl ); 
        pNode->pFunc = Cudd_bddAnd( dd, bFunc0, bFunc1 );   Cudd_Ref( (DdNode *)pNode->pFunc );
    }

    // deref the intermediate results
    bFunc = (DdNode *)pNode->pFunc;   Cudd_Ref( bFunc );
    Kit_GraphForEachNode( pGraph, pNode, i )
        Cudd_RecursiveDeref( dd, (DdNode *)pNode->pFunc );
    Cudd_Deref( bFunc );

    // complement the result if necessary
    return Cudd_NotCond( bFunc, Kit_GraphIsComplement(pGraph) );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Kit_TruthToBdd_rec( DdManager * dd, unsigned * pTruth, int iBit, int nVars, int nVarsTotal, int fMSBonTop )
{
    DdNode * bF0, * bF1, * bF;
    int Var;
    if ( nVars <= 5 )
    {
        unsigned uTruth, uMask;
        uMask = ((~(unsigned)0) >> (32 - (1<<nVars)));
        uTruth = (pTruth[iBit>>5] >> (iBit&31)) & uMask;
        if ( uTruth == 0 )
            return b0;
        if ( uTruth == uMask )
            return b1;        
    }
    // find the variable to use
    Var = fMSBonTop? nVarsTotal-nVars : nVars-1;
    // other special cases can be added
    bF0 = Kit_TruthToBdd_rec( dd, pTruth, iBit,                nVars-1, nVarsTotal, fMSBonTop );  Cudd_Ref( bF0 );
    bF1 = Kit_TruthToBdd_rec( dd, pTruth, iBit+(1<<(nVars-1)), nVars-1, nVarsTotal, fMSBonTop );  Cudd_Ref( bF1 );
    bF  = Cudd_bddIte( dd, dd->vars[Var], bF1, bF0 );                     Cudd_Ref( bF );
    Cudd_RecursiveDeref( dd, bF0 );
    Cudd_RecursiveDeref( dd, bF1 );
    Cudd_Deref( bF );
    return bF;
}

/**Function*************************************************************

  Synopsis    [Compute BDD corresponding to the truth table.]

  Description [If truth table has N vars, the BDD depends on N topmost
  variables of the BDD manager. The most significant variable of the table
  is encoded by the topmost variable of the manager. BDD construction is 
  efficient in this case because BDD is constructed one node at a time, 
  by simply adding BDD nodes on top of existent BDD nodes.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Kit_TruthToBdd( DdManager * dd, unsigned * pTruth, int nVars, int fMSBonTop )
{
    return Kit_TruthToBdd_rec( dd, pTruth, 0, nVars, nVars, fMSBonTop );
}

/**Function*************************************************************

  Synopsis    [Verifies that the factoring is correct.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Kit_SopFactorVerify( Vec_Int_t * vCover, Kit_Graph_t * pFForm, int nVars )
{
    static DdManager * dd = NULL;
    Kit_Sop_t Sop, * cSop = &Sop;
    DdNode * bFunc1, * bFunc2;
    Vec_Int_t * vMemory;
    int RetValue;
    // get the manager
    if ( dd == NULL )
        dd = Cudd_Init( 16, 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0 );
    // derive SOP
    vMemory = Vec_IntAlloc( Vec_IntSize(vCover) );
    Kit_SopCreate( cSop, vCover, nVars, vMemory );
    // get the functions
    bFunc1 = Kit_SopToBdd( dd, cSop, nVars );      Cudd_Ref( bFunc1 );
    bFunc2 = Kit_GraphToBdd( dd, pFForm );         Cudd_Ref( bFunc2 );
//Extra_bddPrint( dd, bFunc1 ); printf("\n");
//Extra_bddPrint( dd, bFunc2 ); printf("\n");
    RetValue = (bFunc1 == bFunc2);
    if ( bFunc1 != bFunc2 )
    {
        int s;
        Extra_bddPrint( dd, bFunc1 ); printf("\n");
        Extra_bddPrint( dd, bFunc2 ); printf("\n");
        s  = 0;
    }
    Cudd_RecursiveDeref( dd, bFunc1 );
    Cudd_RecursiveDeref( dd, bFunc2 );
    Vec_IntFree( vMemory );
    return RetValue;
}

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

