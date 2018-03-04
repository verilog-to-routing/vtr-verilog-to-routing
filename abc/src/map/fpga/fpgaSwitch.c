/**CFile****************************************************************

  FileName    [fpgaSwitch.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Generic technology mapping engine.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 8, 2003.]

  Revision    [$Id: fpgaSwitch.h,v 1.0 2003/09/08 00:00:00 alanmi Exp $]

***********************************************************************/

#include "fpgaInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**function*************************************************************

  synopsis    [Computes the exact area associated with the cut.]

  description []
               
  sideeffects []

  seealso     []

***********************************************************************/
float Fpga_CutGetSwitchDerefed( Fpga_Man_t * pMan, Fpga_Node_t * pNode, Fpga_Cut_t * pCut )
{
    float aResult, aResult2;
    aResult2 = Fpga_CutRefSwitch( pMan, pNode, pCut, 0 );
    aResult  = Fpga_CutDerefSwitch( pMan, pNode, pCut, 0 );
//    assert( aResult == aResult2 );
    return aResult;
}

/**function*************************************************************

  synopsis    [References the cut.]

  description [This procedure is similar to the procedure NodeReclaim.]
               
  sideeffects []

  seealso     []

***********************************************************************/
float Fpga_CutRefSwitch( Fpga_Man_t * pMan, Fpga_Node_t * pNode, Fpga_Cut_t * pCut, int fFanouts )
{
    Fpga_Node_t * pNodeChild;
    float aArea;
    int i;
    // start the area of this cut
    aArea = pNode->Switching;
    if ( pCut->nLeaves == 1 )
        return aArea;
    // go through the children
    for ( i = 0; i < pCut->nLeaves; i++ )
    {
        pNodeChild = pCut->ppLeaves[i];
        assert( pNodeChild->nRefs >= 0 );
        if ( pNodeChild->nRefs++ > 0 )  
            continue;
        aArea += Fpga_CutRefSwitch( pMan, pNodeChild, pNodeChild->pCutBest, fFanouts );
    }
    return aArea;
}

/**function*************************************************************

  synopsis    [Dereferences the cut.]

  description [This procedure is similar to the procedure NodeRecusiveDeref.]
               
  sideeffects []

  seealso     []

***********************************************************************/
float Fpga_CutDerefSwitch( Fpga_Man_t * pMan, Fpga_Node_t * pNode, Fpga_Cut_t * pCut, int fFanouts )
{
    Fpga_Node_t * pNodeChild;
    float aArea;
    int i;
    // start the area of this cut
    aArea = pNode->Switching;
    if ( pCut->nLeaves == 1 )
        return aArea;
    // go through the children
    for ( i = 0; i < pCut->nLeaves; i++ )
    {
        pNodeChild = pCut->ppLeaves[i];
        assert( pNodeChild->nRefs > 0 );
        if ( --pNodeChild->nRefs > 0 )  
            continue;
        aArea += Fpga_CutDerefSwitch( pMan, pNodeChild, pNodeChild->pCutBest, fFanouts );
    }
    return aArea;
}

/**Function*************************************************************

  Synopsis    [Computes the array of mapping.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
float Fpga_MappingGetSwitching( Fpga_Man_t * pMan, Fpga_NodeVec_t * vMapping )
{
    Fpga_Node_t * pNode;
    float Switch;
    int i;
    Switch = 0.0;
    for ( i = 0; i < vMapping->nSize; i++ )
    {
        pNode = vMapping->pArray[i];
        // at least one phase has the best cut assigned
        assert( !Fpga_NodeIsAnd(pNode) || pNode->pCutBest != NULL );
        // at least one phase is used in the mapping
        assert( pNode->nRefs > 0 );
        // compute the array due to the supergate
        Switch += pNode->Switching;
    }
    // add buffer for each CO driven by a CI
    for ( i = 0; i < pMan->nOutputs; i++ )
        if ( Fpga_NodeIsVar(Fpga_Regular(pMan->pOutputs[i])) && !Fpga_IsComplement(pMan->pOutputs[i]) )
            Switch += Fpga_Regular(pMan->pOutputs[i])->Switching;
    return Switch;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

