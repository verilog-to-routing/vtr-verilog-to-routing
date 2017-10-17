/**CFile****************************************************************

  FileName    [mapperSwitch.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Generic technology mapping engine.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 8, 2003.]

  Revision    [$Id: mapperSwitch.h,v 1.0 2003/09/08 00:00:00 alanmi Exp $]

***********************************************************************/

#include "mapperInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static float Map_SwitchCutRefDeref( Map_Node_t * pNode, Map_Cut_t * pCut, int fPhase, int fReference );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**function*************************************************************

  synopsis    [Computes the exact area associated with the cut.]

  description []
               
  sideeffects []

  seealso     []

***********************************************************************/
float Map_SwitchCutGetDerefed( Map_Node_t * pNode, Map_Cut_t * pCut, int fPhase )
{
    float aResult, aResult2;
//    assert( pNode->Switching > 0 );
    aResult2 = Map_SwitchCutRefDeref( pNode, pCut, fPhase, 1 ); // reference
    aResult  = Map_SwitchCutRefDeref( pNode, pCut, fPhase, 0 ); // dereference
//    assert( aResult == aResult2 );
    return aResult;
}

/**function*************************************************************

  synopsis    [References the cut.]

  description []
               
  sideeffects []

  seealso     []

***********************************************************************/
float Map_SwitchCutRef( Map_Node_t * pNode, Map_Cut_t * pCut, int fPhase )
{
    return Map_SwitchCutRefDeref( pNode, pCut, fPhase, 1 ); // reference
}

/**function*************************************************************

  synopsis    [References the cut.]

  description []
               
  sideeffects []

  seealso     []

***********************************************************************/
float Map_SwitchCutDeref( Map_Node_t * pNode, Map_Cut_t * pCut, int fPhase )
{
    return Map_SwitchCutRefDeref( pNode, pCut, fPhase, 0 ); // dereference
}

/**function*************************************************************

  synopsis    [References or dereferences the cut.]

  description [This reference part is similar to Cudd_NodeReclaim(). 
  The dereference part is similar to Cudd_RecursiveDeref().]
               
  sideeffects []

  seealso     []

***********************************************************************/
float Map_SwitchCutRefDeref( Map_Node_t * pNode, Map_Cut_t * pCut, int fPhase, int fReference )
{
    Map_Node_t * pNodeChild;
    Map_Cut_t * pCutChild;
    float aSwitchActivity;
    int i, fPhaseChild;

    // start switching activity for the node
    aSwitchActivity = pNode->Switching;
    // consider the elementary variable
    if ( pCut->nLeaves == 1 )
        return aSwitchActivity;

    // go through the children
    assert( pCut->M[fPhase].pSuperBest );
    for ( i = 0; i < pCut->nLeaves; i++ )
    {
        pNodeChild  = pCut->ppLeaves[i];
        fPhaseChild = Map_CutGetLeafPhase( pCut, fPhase, i );
        // get the reference counter of the child

        if ( fReference )
        {
            if ( pNodeChild->pCutBest[0] && pNodeChild->pCutBest[1] ) // both phases are present
            {
                // if this phase of the node is referenced, there is no recursive call
                pNodeChild->nRefAct[2]++;
                if ( pNodeChild->nRefAct[fPhaseChild]++ > 0 )
                    continue;
            }
            else // only one phase is present
            {
                // inverter should be added if the phase
                // (a) has no reference and (b) is implemented using other phase
                if ( pNodeChild->nRefAct[fPhaseChild]++ == 0 && pNodeChild->pCutBest[fPhaseChild] == NULL )
                    aSwitchActivity += pNodeChild->Switching; // inverter switches the same as the node
                // if the node is referenced, there is no recursive call
                if ( pNodeChild->nRefAct[2]++ > 0 )
                    continue;
            }
        }
        else
        {
            if ( pNodeChild->pCutBest[0] && pNodeChild->pCutBest[1] ) // both phases are present
            {
                // if this phase of the node is referenced, there is no recursive call
                --pNodeChild->nRefAct[2];
                if ( --pNodeChild->nRefAct[fPhaseChild] > 0 )
                    continue;
            }
            else // only one phase is present
            {
                // inverter should be added if the phase
                // (a) has no reference and (b) is implemented using other phase
                if ( --pNodeChild->nRefAct[fPhaseChild] == 0 && pNodeChild->pCutBest[fPhaseChild] == NULL )
                    aSwitchActivity += pNodeChild->Switching; // inverter switches the same as the node
                // if the node is referenced, there is no recursive call
                if ( --pNodeChild->nRefAct[2] > 0 )
                    continue;
            }
            assert( pNodeChild->nRefAct[fPhaseChild] >= 0 );
        }

        // get the child cut
        pCutChild = pNodeChild->pCutBest[fPhaseChild];
        // if the child does not have this phase mapped, take the opposite phase
        if ( pCutChild == NULL )
        {
            fPhaseChild = !fPhaseChild;
            pCutChild   = pNodeChild->pCutBest[fPhaseChild];
        }
        // reference and compute area recursively
        aSwitchActivity += Map_SwitchCutRefDeref( pNodeChild, pCutChild, fPhaseChild, fReference );
    }
    return aSwitchActivity;
}

/**Function*************************************************************

  Synopsis    [Computes the array of mapping.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
float Map_MappingGetSwitching( Map_Man_t * pMan )
{
    Map_Node_t * pNode;
    float Switch = 0.0;
    int i;
    for ( i = 0; i < pMan->vMapObjs->nSize; i++ )
    {
        pNode = pMan->vMapObjs->pArray[i];
        if ( pNode->nRefAct[2] == 0 )
            continue;
        // at least one phase has the best cut assigned
        assert( pNode->pCutBest[0] != NULL || pNode->pCutBest[1] != NULL );
        // at least one phase is used in the mapping
        assert( pNode->nRefAct[0] > 0 || pNode->nRefAct[1] > 0 );
        // compute the array due to the supergate
        if ( Map_NodeIsAnd(pNode) )
        {
            // count switching of the negative phase
            if ( pNode->pCutBest[0] && (pNode->nRefAct[0] > 0 || pNode->pCutBest[1] == NULL) )
                Switch += pNode->Switching;
            // count switching of the positive phase
            if ( pNode->pCutBest[1] && (pNode->nRefAct[1] > 0 || pNode->pCutBest[0] == NULL) )
                Switch += pNode->Switching;
        }
        // count switching of the interver if we need to implement one phase with another phase
        if ( (pNode->pCutBest[0] == NULL && pNode->nRefAct[0] > 0) || 
             (pNode->pCutBest[1] == NULL && pNode->nRefAct[1] > 0) )
            Switch += pNode->Switching; // inverter switches the same as the node
    }
    // add buffers for each CO driven by a CI
    for ( i = 0; i < pMan->nOutputs; i++ )
        if ( Map_NodeIsVar(pMan->pOutputs[i]) && !Map_IsComplement(pMan->pOutputs[i]) )
            Switch += pMan->pOutputs[i]->Switching;
    return Switch;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

