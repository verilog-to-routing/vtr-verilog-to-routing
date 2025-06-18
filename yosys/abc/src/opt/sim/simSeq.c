/**CFile****************************************************************

  FileName    [simSeq.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Network and node package.]

  Synopsis    [Simulation for sequential circuits.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: simUtils.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "base/abc/abc.h"
#include "sim.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static void Sim_SimulateSeqFrame( Vec_Ptr_t * vInfo, Abc_Ntk_t * pNtk, int iFrames, int nWords, int fTransfer );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Simulates sequential circuit.]

  Description [Takes sequential circuit (pNtk). Simulates the given number
  (nFrames) of the circuit with the given number of machine words (nWords)
  of random simulation data, starting from the initial state. If the initial
  state of some latches is a don't-care, uses random input for that latch.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Sim_SimulateSeqRandom( Abc_Ntk_t * pNtk, int nFrames, int nWords )
{
    Vec_Ptr_t * vInfo;
    Abc_Obj_t * pNode;
    int i;
    assert( Abc_NtkIsStrash(pNtk) );
    vInfo = Sim_UtilInfoAlloc( Abc_NtkObjNumMax(pNtk), nWords * nFrames, 0 );
    // set the constant data
    pNode = Abc_AigConst1(pNtk);
    Sim_UtilSetConst( Sim_SimInfoGet(vInfo,pNode), nWords * nFrames, 1 );
    // set the random PI data
    Abc_NtkForEachPi( pNtk, pNode, i )
        Sim_UtilSetRandom( Sim_SimInfoGet(vInfo,pNode), nWords * nFrames );
    // set the initial state data
    Abc_NtkForEachLatch( pNtk, pNode, i )
        if ( Abc_LatchIsInit0(pNode) )
            Sim_UtilSetConst( Sim_SimInfoGet(vInfo,pNode), nWords, 0 );
        else if ( Abc_LatchIsInit1(pNode) )
            Sim_UtilSetConst( Sim_SimInfoGet(vInfo,pNode), nWords, 1 );
        else 
            Sim_UtilSetRandom( Sim_SimInfoGet(vInfo,pNode), nWords );
    // simulate the nodes for the given number of timeframes
    for ( i = 0; i < nFrames; i++ )
        Sim_SimulateSeqFrame( vInfo, pNtk, i, nWords, (int)(i < nFrames-1) );
    return vInfo;
}

/**Function*************************************************************

  Synopsis    [Simulates sequential circuit.]

  Description [Takes sequential circuit (pNtk). Simulates the given number
  (nFrames) of the circuit with the given model. The model is assumed to 
  contain values of PIs for each frame. The latches are initialized to
  the initial state. One word of data is simulated.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Sim_SimulateSeqModel( Abc_Ntk_t * pNtk, int nFrames, int * pModel )
{
    Vec_Ptr_t * vInfo;
    Abc_Obj_t * pNode;
    unsigned * pUnsigned;
    int i, k;
    vInfo = Sim_UtilInfoAlloc( Abc_NtkObjNumMax(pNtk), nFrames, 0 );
    // set the constant data
    pNode = Abc_AigConst1(pNtk);
    Sim_UtilSetConst( Sim_SimInfoGet(vInfo,pNode), nFrames, 1 );
    // set the random PI data
    Abc_NtkForEachPi( pNtk, pNode, i )
    {
        pUnsigned = Sim_SimInfoGet(vInfo,pNode);
        for ( k = 0; k < nFrames; k++ )
            pUnsigned[k] = pModel[k * Abc_NtkPiNum(pNtk) + i] ? ~((unsigned)0) : 0;
    }
    // set the initial state data
    Abc_NtkForEachLatch( pNtk, pNode, i )
    {
        pUnsigned = Sim_SimInfoGet(vInfo,pNode);
        if ( Abc_LatchIsInit0(pNode) )
            pUnsigned[0] = 0;
        else if ( Abc_LatchIsInit1(pNode) )
            pUnsigned[0] = ~((unsigned)0);
        else 
            pUnsigned[0] = SIM_RANDOM_UNSIGNED;
    }
    // simulate the nodes for the given number of timeframes
    for ( i = 0; i < nFrames; i++ )
        Sim_SimulateSeqFrame( vInfo, pNtk, i, 1, (int)(i < nFrames-1) );
/*
    // print the simulated values
    for ( i = 0; i < nFrames; i++ )
    {
        printf( "Frame %d : ", i+1 );
        Abc_NtkForEachPi( pNtk, pNode, k )
            printf( "%d", Sim_SimInfoGet(vInfo,pNode)[i] > 0 );
        printf( " " );
        Abc_NtkForEachLatch( pNtk, pNode, k )
            printf( "%d", Sim_SimInfoGet(vInfo,pNode)[i] > 0 );
        printf( " " );
        Abc_NtkForEachPo( pNtk, pNode, k )
            printf( "%d", Sim_SimInfoGet(vInfo,pNode)[i] > 0 );
        printf( "\n" );
    }
    printf( "\n" );
*/
    return vInfo;
}

/**Function*************************************************************

  Synopsis    [Simulates one frame of sequential circuit.]

  Description [Assumes that the latches and POs are already initialized.
  In the end transfers the data to the latches of the next frame.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sim_SimulateSeqFrame( Vec_Ptr_t * vInfo, Abc_Ntk_t * pNtk, int iFrames, int nWords, int fTransfer )
{
    Abc_Obj_t * pNode;
    int i;
    Abc_NtkForEachNode( pNtk, pNode, i )
        Sim_UtilSimulateNodeOne( pNode, vInfo, nWords, iFrames * nWords );
    Abc_NtkForEachPo( pNtk, pNode, i )
        Sim_UtilTransferNodeOne( pNode, vInfo, nWords, iFrames * nWords, 0 );
    if ( !fTransfer )
        return;
    Abc_NtkForEachLatch( pNtk, pNode, i )
        Sim_UtilTransferNodeOne( pNode, vInfo, nWords, iFrames * nWords, 1 );
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

