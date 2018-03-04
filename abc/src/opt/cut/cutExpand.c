/**CFile****************************************************************

  FileName    [cutExpand.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [K-feasible cut computation package.]

  Synopsis    [Computes the truth table of the cut after expansion.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: cutExpand.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "cutInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#define CUT_CELL_MVAR  9

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Computes the stretching phase of the cut w.r.t. the merged cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline unsigned Cut_TruthPhase( Cut_Cut_t * pCut, Cut_Cut_t * pCut1 )
{
    unsigned uPhase = 0;
    int i, k;
    for ( i = k = 0; i < (int)pCut->nLeaves; i++ )
    {
        if ( k == (int)pCut1->nLeaves )
            break;
        if ( pCut->pLeaves[i] < pCut1->pLeaves[k] )
            continue;
        assert( pCut->pLeaves[i] == pCut1->pLeaves[k] );
        uPhase |= (1 << i);
        k++;
    }
    return uPhase;
}

/**Function*************************************************************

  Synopsis    [Computes the truth table of the composition of cuts.]

  Description [Inputs are: 
  - a factor cut (truth table is stored inside)
  - a node in the factor cut
  - a tree cut to be substituted (truth table is stored inside)
  - the resulting cut (truth table will be filled in).
  Note that all cuts, including the resulting one, should be already 
  computed and the nodes should be stored in the ascending order.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cut_TruthCompose( Cut_Cut_t * pCutF, int Node, Cut_Cut_t * pCutT, Cut_Cut_t * pCutRes )
{
    static unsigned uCof0[1<<(CUT_CELL_MVAR-5)];
    static unsigned uCof1[1<<(CUT_CELL_MVAR-5)];
    static unsigned uTemp[1<<(CUT_CELL_MVAR-5)];
    unsigned * pIn, * pOut, * pTemp;
    unsigned uPhase;
    int NodeIndex, i, k;

    // sanity checks
    assert( pCutF->nVarsMax == pCutT->nVarsMax );
    assert( pCutF->nVarsMax == pCutRes->nVarsMax );
    assert( pCutF->nVarsMax <= CUT_CELL_MVAR );
    // the factor cut (pCutF) should have its nodes sorted in the ascending order
    assert( pCutF->nLeaves <= pCutF->nVarsMax );
    for ( i = 0; i < (int)pCutF->nLeaves - 1; i++ )
        assert( pCutF->pLeaves[i] < pCutF->pLeaves[i+1] );
    // the tree cut (pCutT) should have its nodes sorted in the ascending order
    assert( pCutT->nLeaves <= pCutT->nVarsMax );
    for ( i = 0; i < (int)pCutT->nLeaves - 1; i++ )
        assert( pCutT->pLeaves[i] < pCutT->pLeaves[i+1] );
    // the resulting cut (pCutRes) should have its nodes sorted in the ascending order
    assert( pCutRes->nLeaves <= pCutRes->nVarsMax );
    for ( i = 0; i < (int)pCutRes->nLeaves - 1; i++ )
        assert( pCutRes->pLeaves[i] < pCutRes->pLeaves[i+1] );
    // make sure that every node in pCutF (except Node) appears in pCutRes
    for ( i = 0; i < (int)pCutF->nLeaves; i++ )
    {
        if ( pCutF->pLeaves[i] == Node )
            continue;
        for ( k = 0; k < (int)pCutRes->nLeaves; k++ )
            if ( pCutF->pLeaves[i] == pCutRes->pLeaves[k] )
                break;
        assert( k < (int)pCutRes->nLeaves ); // node i from pCutF is not found in pCutRes!!!
    }
    // make sure that every node in pCutT appears in pCutRes
    for ( i = 0; i < (int)pCutT->nLeaves; i++ )
    {
        for ( k = 0; k < (int)pCutRes->nLeaves; k++ )
            if ( pCutT->pLeaves[i] == pCutRes->pLeaves[k] )
                break;
        assert( k < (int)pCutRes->nLeaves ); // node i from pCutT is not found in pCutRes!!!
    }


    // find the index of the given node in the factor cut
    NodeIndex = -1;
    for ( NodeIndex = 0; NodeIndex < (int)pCutF->nLeaves; NodeIndex++ )
        if ( pCutF->pLeaves[NodeIndex] == Node )
            break;
    assert( NodeIndex >= 0 );  // Node should be in pCutF

    // copy the truth table
    Extra_TruthCopy( uTemp, Cut_CutReadTruth(pCutF), pCutF->nLeaves );

    // bubble-move the NodeIndex variable to be the last one (the most significant one)
    pIn = uTemp; pOut = uCof0; // uCof0 is used for temporary storage here
    for ( i = NodeIndex; i < (int)pCutF->nLeaves - 1; i++ )
    {
        Extra_TruthSwapAdjacentVars( pOut, pIn, pCutF->nLeaves, i );
        pTemp = pIn; pIn = pOut; pOut = pTemp;
    }
    if ( (pCutF->nLeaves - 1 - NodeIndex) & 1 )
        Extra_TruthCopy( pOut, pIn, pCutF->nLeaves );
    // the result of stretching is in uTemp

    // cofactor the factor cut with respect to the node
    Extra_TruthCopy( uCof0, uTemp, pCutF->nLeaves );
    Extra_TruthCofactor0( uCof0, pCutF->nLeaves, pCutF->nLeaves-1 );
    Extra_TruthCopy( uCof1, uTemp, pCutF->nLeaves );
    Extra_TruthCofactor1( uCof1, pCutF->nLeaves, pCutF->nLeaves-1 );

    // temporarily shrink the factor cut's variables by removing Node 
    for ( i = NodeIndex; i < (int)pCutF->nLeaves - 1; i++ )
        pCutF->pLeaves[i] = pCutF->pLeaves[i+1];
    pCutF->nLeaves--;

    // spread out the cofactors' truth tables to the same var order as the resulting cut
    uPhase = Cut_TruthPhase(pCutRes, pCutF);
    assert( Extra_WordCountOnes(uPhase) == (int)pCutF->nLeaves );
    Extra_TruthStretch( uTemp, uCof0, pCutF->nLeaves, pCutF->nVarsMax, uPhase );
    Extra_TruthCopy( uCof0, uTemp, pCutF->nVarsMax );
    Extra_TruthStretch( uTemp, uCof1, pCutF->nLeaves, pCutF->nVarsMax, uPhase );
    Extra_TruthCopy( uCof1, uTemp, pCutF->nVarsMax );

    // spread out the tree cut's truth table to the same var order as the resulting cut
    uPhase = Cut_TruthPhase(pCutRes, pCutT); 
    assert( Extra_WordCountOnes(uPhase) == (int)pCutT->nLeaves );
    Extra_TruthStretch( uTemp, Cut_CutReadTruth(pCutT), pCutT->nLeaves, pCutT->nVarsMax, uPhase );

    // create the resulting truth table
    pTemp = Cut_CutReadTruth(pCutRes);
    for ( i = Extra_TruthWordNum(pCutRes->nLeaves)-1; i >= 0; i-- )
        pTemp[i] = (uCof0[i] & ~uTemp[i]) | (uCof1[i] & uTemp[i]);

    // undo the removal of the node from the cut
    for ( i = (int)pCutF->nLeaves - 1; i >= NodeIndex; --i )
        pCutF->pLeaves[i+1] = pCutF->pLeaves[i];
    pCutF->pLeaves[NodeIndex] = Node;
    pCutF->nLeaves++;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

