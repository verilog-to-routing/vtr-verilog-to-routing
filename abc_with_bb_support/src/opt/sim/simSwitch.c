/**CFile****************************************************************

  FileName    [simSwitch.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Network and node package.]

  Synopsis    [Computes switching activity of nodes in the ABC network.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: simSwitch.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "base/abc/abc.h"
#include "sim.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static void Sim_NodeSimulate( Abc_Obj_t * pNode, Vec_Ptr_t * vSimInfo, int nSimWords );
static float Sim_ComputeSwitching( unsigned * pSimInfo, int nSimWords );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Computes switching activity using simulation.]

  Description [Computes switching activity, which is understood as the
  probability of switching under random simulation. Assigns the
  random simulation information at the CI and propagates it through
  the internal nodes of the AIG.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Sim_NtkComputeSwitching( Abc_Ntk_t * pNtk, int nPatterns )
{
    Vec_Int_t * vSwitching;
    float * pSwitching;
    Vec_Ptr_t * vNodes;
    Vec_Ptr_t * vSimInfo;
    Abc_Obj_t * pNode;
    unsigned * pSimInfo;
    int nSimWords, i;

    // allocate space for simulation info of all nodes
    nSimWords = SIM_NUM_WORDS(nPatterns);
    vSimInfo = Sim_UtilInfoAlloc( Abc_NtkObjNumMax(pNtk), nSimWords, 0 );
    // assign the random simulation to the CIs
    vSwitching = Vec_IntStart( Abc_NtkObjNumMax(pNtk) );
    pSwitching = (float *)vSwitching->pArray;
    Abc_NtkForEachCi( pNtk, pNode, i )
    {
        pSimInfo = (unsigned *)Vec_PtrEntry(vSimInfo, pNode->Id);
        Sim_UtilSetRandom( pSimInfo, nSimWords );
        pSwitching[pNode->Id] = Sim_ComputeSwitching( pSimInfo, nSimWords );
    }
    // simulate the internal nodes
    vNodes  = Abc_AigDfs( pNtk, 1, 0 );
    Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pNode, i )
    {
        pSimInfo = (unsigned *)Vec_PtrEntry(vSimInfo, pNode->Id);
        Sim_UtilSimulateNodeOne( pNode, vSimInfo, nSimWords, 0 );
        pSwitching[pNode->Id] = Sim_ComputeSwitching( pSimInfo, nSimWords );
    }
    Vec_PtrFree( vNodes );
    Sim_UtilInfoFree( vSimInfo );
    return vSwitching;
}

/**Function*************************************************************

  Synopsis    [Computes switching activity of one node.]

  Description [Uses the formula: Switching = 2 * nOnes * nZeros / (nTotal ^ 2) ]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
float Sim_ComputeSwitching( unsigned * pSimInfo, int nSimWords )
{
    int nOnes, nTotal;
    nTotal = 32 * nSimWords;
    nOnes = Sim_UtilCountOnes( pSimInfo, nSimWords );
    return (float)2.0 * nOnes / nTotal * (nTotal - nOnes) / nTotal;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

