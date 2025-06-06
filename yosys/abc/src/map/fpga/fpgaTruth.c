/**CFile****************************************************************

  FileName    [fpgaTruth.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Technology mapping for variable-size-LUT FPGAs.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 2.0. Started - August 18, 2004.]

  Revision    [$Id: fpgaTruth.c,v 1.4 2005/01/23 06:59:42 alanmi Exp $]

***********************************************************************/

#include "fpgaInt.h"
#include "bdd/cudd/cudd.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Recursively derives the truth table for the cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Fpga_TruthsCutBdd_rec( DdManager * dd, Fpga_Cut_t * pCut, Fpga_NodeVec_t * vVisited )
{
    DdNode * bFunc, * bFunc0, * bFunc1;
    assert( !Fpga_IsComplement(pCut) );
    // if the cut is visited, return the result
    if ( pCut->uSign )
        return (DdNode *)(ABC_PTRUINT_T)pCut->uSign;
    // compute the functions of the children
    bFunc0 = Fpga_TruthsCutBdd_rec( dd, Fpga_CutRegular(pCut->pOne), vVisited );   Cudd_Ref( bFunc0 );
    bFunc0 = Cudd_NotCond( bFunc0, Fpga_CutIsComplement(pCut->pOne) );
    bFunc1 = Fpga_TruthsCutBdd_rec( dd, Fpga_CutRegular(pCut->pTwo), vVisited );   Cudd_Ref( bFunc1 );
    bFunc1 = Cudd_NotCond( bFunc1, Fpga_CutIsComplement(pCut->pTwo) );
    // get the function of the cut
    bFunc  = Cudd_bddAnd( dd, bFunc0, bFunc1 );   Cudd_Ref( bFunc );
    bFunc  = Cudd_NotCond( bFunc, pCut->Phase );
    Cudd_RecursiveDeref( dd, bFunc0 );
    Cudd_RecursiveDeref( dd, bFunc1 );
    assert( pCut->uSign == 0 );
    pCut->uSign = (unsigned)(ABC_PTRUINT_T)bFunc;
    // add this cut to the visited list
    Fpga_NodeVecPush( vVisited, (Fpga_Node_t *)pCut );
    return bFunc;
}

/**Function*************************************************************

  Synopsis    [Derives the truth table for one cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void * Fpga_TruthsCutBdd( void * dd, Fpga_Cut_t * pCut )
{
    Fpga_NodeVec_t * vVisited;
    DdNode * bFunc;
    int i;
    assert( pCut->nLeaves > 1 );
    // set the leaf variables
    for ( i = 0; i < pCut->nLeaves; i++ )
        pCut->ppLeaves[i]->pCuts->uSign = (unsigned)(ABC_PTRUINT_T)Cudd_bddIthVar( (DdManager *)dd, i );
    // recursively compute the function
    vVisited = Fpga_NodeVecAlloc( 10 );
    bFunc = Fpga_TruthsCutBdd_rec( (DdManager *)dd, pCut, vVisited );   Cudd_Ref( bFunc );
    // clean the intermediate BDDs
    for ( i = 0; i < pCut->nLeaves; i++ )
        pCut->ppLeaves[i]->pCuts->uSign = 0;
    for ( i = 0; i < vVisited->nSize; i++ )
    {
        pCut = (Fpga_Cut_t *)vVisited->pArray[i];
        Cudd_RecursiveDeref( (DdManager *)dd, (DdNode*)(ABC_PTRUINT_T)pCut->uSign );
        pCut->uSign = 0;
    }
//    printf( "%d ", vVisited->nSize );
    Fpga_NodeVecFree( vVisited );
    Cudd_Deref( bFunc );
    return bFunc;
}


/**Function*************************************************************

  Synopsis    [Recursively derives the truth table for the cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fpga_CutVolume_rec( Fpga_Cut_t * pCut, Fpga_NodeVec_t * vVisited )
{
    assert( !Fpga_IsComplement(pCut) );
    if ( pCut->fMark )
        return;
    pCut->fMark = 1;
    Fpga_CutVolume_rec( Fpga_CutRegular(pCut->pOne), vVisited );
    Fpga_CutVolume_rec( Fpga_CutRegular(pCut->pTwo), vVisited );
    Fpga_NodeVecPush( vVisited, (Fpga_Node_t *)pCut );
}

/**Function*************************************************************

  Synopsis    [Derives the truth table for one cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fpga_CutVolume( Fpga_Cut_t * pCut )
{
    Fpga_NodeVec_t * vVisited;
    int Volume, i;
    assert( pCut->nLeaves > 1 );
    // set the leaf variables
    for ( i = 0; i < pCut->nLeaves; i++ )
        pCut->ppLeaves[i]->pCuts->fMark = 1;
    // recursively compute the function
    vVisited = Fpga_NodeVecAlloc( 10 );
    Fpga_CutVolume_rec( pCut, vVisited ); 
    // clean the marks
    for ( i = 0; i < pCut->nLeaves; i++ )
        pCut->ppLeaves[i]->pCuts->fMark = 0;
    for ( i = 0; i < vVisited->nSize; i++ )
    {
        pCut = (Fpga_Cut_t *)vVisited->pArray[i];
        pCut->fMark = 0;
    }
    Volume = vVisited->nSize;
    printf( "%d ", Volume );
    Fpga_NodeVecFree( vVisited );
    return Volume;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

