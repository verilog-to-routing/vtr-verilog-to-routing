/**CFile****************************************************************

  FileName    [abcMap.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Network and node package.]

  Synopsis    [Interface with the SC mapping package.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: abcMap.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "abc.h"
#include "main.h"
#include "mio.h"
#include "mapper.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static Map_Man_t *  Abc_NtkToMap( Abc_Ntk_t * pNtk, double DelayTarget, int fRecovery, float * pSwitching, int fVerbose );
static Abc_Ntk_t *  Abc_NtkFromMap( Map_Man_t * pMan, Abc_Ntk_t * pNtk );
static Abc_Obj_t *  Abc_NodeFromMap_rec( Abc_Ntk_t * pNtkNew, Map_Node_t * pNodeMap, int fPhase );
static Abc_Obj_t *  Abc_NodeFromMapPhase_rec( Abc_Ntk_t * pNtkNew, Map_Node_t * pNodeMap, int fPhase );
static Abc_Obj_t *  Abc_NodeFromMapSuper_rec( Abc_Ntk_t * pNtkNew, Map_Node_t * pNodeMap, Map_Super_t * pSuper, Abc_Obj_t * pNodePis[], int nNodePis );

static Abc_Ntk_t *  Abc_NtkFromMapSuperChoice( Map_Man_t * pMan, Abc_Ntk_t * pNtk );
static void         Abc_NodeSuperChoice( Abc_Ntk_t * pNtkNew, Abc_Obj_t * pNode );
static void         Abc_NodeFromMapCutPhase( Abc_Ntk_t * pNtkNew, Map_Cut_t * pCut, int fPhase );
static Abc_Obj_t *  Abc_NodeFromMapSuperChoice_rec( Abc_Ntk_t * pNtkNew, Map_Super_t * pSuper, Abc_Obj_t * pNodePis[], int nNodePis );

 
////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Interface with the mapping package.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkMap( Abc_Ntk_t * pNtk, double DelayTarget, int fRecovery, int fSwitching, int fVerbose )
{
    int fShowSwitching = 1;
    Abc_Ntk_t * pNtkNew;
    Map_Man_t * pMan;
    Vec_Int_t * vSwitching;
    float * pSwitching = NULL;
    int clk;

    assert( Abc_NtkIsStrash(pNtk) );

    // check that the library is available
    if ( Abc_FrameReadLibGen() == NULL )
    {
        printf( "The current library is not available.\n" );
        return 0;
    }

    // derive the supergate library
    if ( Abc_FrameReadLibSuper() == NULL && Abc_FrameReadLibGen() )
    {
        printf( "A simple supergate library is derived from gate library \"%s\".\n", 
            Mio_LibraryReadName(Abc_FrameReadLibGen()) );
        Map_SuperLibDeriveFromGenlib( Abc_FrameReadLibGen() );
    }

    // print a warning about choice nodes
    if ( Abc_NtkGetChoiceNum( pNtk ) )
        printf( "Performing mapping with choices.\n" );

    // compute switching activity
    fShowSwitching |= fSwitching;
    if ( fShowSwitching )
    {
        extern Vec_Int_t * Sim_NtkComputeSwitching( Abc_Ntk_t * pNtk, int nPatterns );
        vSwitching = Sim_NtkComputeSwitching( pNtk, 4096 );
        pSwitching = (float *)vSwitching->pArray;
    }

    // perform the mapping
    pMan = Abc_NtkToMap( pNtk, DelayTarget, fRecovery, pSwitching, fVerbose );
    if ( pSwitching ) Vec_IntFree( vSwitching );
    if ( pMan == NULL )
        return NULL;
clk = clock();
    Map_ManSetSwitching( pMan, fSwitching );
    if ( !Map_Mapping( pMan ) )
    {
        Map_ManFree( pMan );
        return NULL;
    }
//    Map_ManPrintStatsToFile( pNtk->pSpec, Map_ManReadAreaFinal(pMan), Map_ManReadRequiredGlo(pMan), clock()-clk );

    // reconstruct the network after mapping
    pNtkNew = Abc_NtkFromMap( pMan, pNtk );
    if ( pNtkNew == NULL )
        return NULL;
    Map_ManFree( pMan );

    if ( pNtk->pExdc )
        pNtkNew->pExdc = Abc_NtkDup( pNtk->pExdc );

    // make sure that everything is okay
    if ( !Abc_NtkCheck( pNtkNew ) )
    {
        printf( "Abc_NtkMap: The network check has failed.\n" );
        Abc_NtkDelete( pNtkNew );
        return NULL;
    }
    return pNtkNew;
}

/**Function*************************************************************

  Synopsis    [Load the network into manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Map_Man_t * Abc_NtkToMap( Abc_Ntk_t * pNtk, double DelayTarget, int fRecovery, float * pSwitching, int fVerbose )
{
    Map_Man_t * pMan;
    ProgressBar * pProgress;
    Map_Node_t * pNodeMap;
    Vec_Ptr_t * vNodes;
    Abc_Obj_t * pNode, * pFanin, * pPrev;
    int i;

    assert( Abc_NtkIsStrash(pNtk) );

    // start the mapping manager and set its parameters
    pMan = Map_ManCreate( Abc_NtkPiNum(pNtk) + Abc_NtkLatchNum(pNtk), Abc_NtkPoNum(pNtk) + Abc_NtkLatchNum(pNtk), fVerbose );
    if ( pMan == NULL )
        return NULL;
    Map_ManSetAreaRecovery( pMan, fRecovery );
    Map_ManSetOutputNames( pMan, Abc_NtkCollectCioNames(pNtk, 1) );
    Map_ManSetDelayTarget( pMan, (float)DelayTarget );
    Map_ManSetInputArrivals( pMan, (Map_Time_t *)Abc_NtkGetCiArrivalTimes(pNtk) );

    // create PIs and remember them in the old nodes
    Abc_NtkCleanCopy( pNtk );
    Abc_AigConst1(pNtk)->pCopy = (Abc_Obj_t *)Map_ManReadConst1(pMan);
    Abc_NtkForEachCi( pNtk, pNode, i )
    {
        pNodeMap = Map_ManReadInputs(pMan)[i];
        pNode->pCopy = (Abc_Obj_t *)pNodeMap;
        if ( pSwitching )
            Map_NodeSetSwitching( pNodeMap, pSwitching[pNode->Id] );
    }

    // load the AIG into the mapper
    vNodes = Abc_AigDfs( pNtk, 0, 0 );
    pProgress = Extra_ProgressBarStart( stdout, vNodes->nSize );
    Vec_PtrForEachEntry( vNodes, pNode, i )
    {
        Extra_ProgressBarUpdate( pProgress, i, NULL );
        // add the node to the mapper
        pNodeMap = Map_NodeAnd( pMan, 
            Map_NotCond( Abc_ObjFanin0(pNode)->pCopy, Abc_ObjFaninC0(pNode) ),
            Map_NotCond( Abc_ObjFanin1(pNode)->pCopy, Abc_ObjFaninC1(pNode) ) );
        assert( pNode->pCopy == NULL );
        // remember the node
        pNode->pCopy = (Abc_Obj_t *)pNodeMap;
        if ( pSwitching )
            Map_NodeSetSwitching( pNodeMap, pSwitching[pNode->Id] );
        // set up the choice node
        if ( Abc_AigNodeIsChoice( pNode ) )
            for ( pPrev = pNode, pFanin = pNode->pData; pFanin; pPrev = pFanin, pFanin = pFanin->pData )
            {
                Map_NodeSetNextE( (Map_Node_t *)pPrev->pCopy, (Map_Node_t *)pFanin->pCopy );
                Map_NodeSetRepr( (Map_Node_t *)pFanin->pCopy, (Map_Node_t *)pNode->pCopy );
            }
    }
    Extra_ProgressBarStop( pProgress );
    Vec_PtrFree( vNodes );

    // set the primary outputs in the required phase
    Abc_NtkForEachCo( pNtk, pNode, i )
        Map_ManReadOutputs(pMan)[i] = Map_NotCond( (Map_Node_t *)Abc_ObjFanin0(pNode)->pCopy, Abc_ObjFaninC0(pNode) );
    return pMan;
}

/**Function*************************************************************

  Synopsis    [Creates the mapped network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkFromMap( Map_Man_t * pMan, Abc_Ntk_t * pNtk )
{
    ProgressBar * pProgress;
    Abc_Ntk_t * pNtkNew;
    Map_Node_t * pNodeMap;
    Abc_Obj_t * pNode, * pNodeNew;
    int i, nDupGates;
    // create the new network
    pNtkNew = Abc_NtkStartFrom( pNtk, ABC_NTK_LOGIC, ABC_FUNC_MAP );
    // make the mapper point to the new network
    Map_ManCleanData( pMan );
    Abc_NtkForEachCi( pNtk, pNode, i )
        Map_NodeSetData( Map_ManReadInputs(pMan)[i], 1, (char *)pNode->pCopy );
    // assign the mapping of the required phase to the POs
    pProgress = Extra_ProgressBarStart( stdout, Abc_NtkCoNum(pNtk) );
    Abc_NtkForEachCo( pNtk, pNode, i )
    {
        Extra_ProgressBarUpdate( pProgress, i, NULL );
        pNodeMap = Map_ManReadOutputs(pMan)[i];
        pNodeNew = Abc_NodeFromMap_rec( pNtkNew, Map_Regular(pNodeMap), !Map_IsComplement(pNodeMap) );
        assert( !Abc_ObjIsComplement(pNodeNew) );
        Abc_ObjAddFanin( pNode->pCopy, pNodeNew );
    }
    Extra_ProgressBarStop( pProgress );
    // decouple the PO driver nodes to reduce the number of levels
    nDupGates = Abc_NtkLogicMakeSimpleCos( pNtkNew, 1 );
//    if ( nDupGates && Map_ManReadVerbose(pMan) )
//        printf( "Duplicated %d gates to decouple the CO drivers.\n", nDupGates );
    return pNtkNew;
}

/**Function*************************************************************

  Synopsis    [Constructs the nodes corrresponding to one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Obj_t * Abc_NodeFromMap_rec( Abc_Ntk_t * pNtkNew, Map_Node_t * pNodeMap, int fPhase )
{
    Abc_Obj_t * pNodeNew, * pNodeInv;

    // check the case of constant node
    if ( Map_NodeIsConst(pNodeMap) )
        return fPhase? Abc_NtkCreateNodeConst1(pNtkNew) : Abc_NtkCreateNodeConst0(pNtkNew);

    // check if the phase is already implemented
    pNodeNew = (Abc_Obj_t *)Map_NodeReadData( pNodeMap, fPhase );
    if ( pNodeNew )
        return pNodeNew;

    // implement the node if the best cut is assigned
    if ( Map_NodeReadCutBest(pNodeMap, fPhase) != NULL )
        return Abc_NodeFromMapPhase_rec( pNtkNew, pNodeMap, fPhase );

    // if the cut is not assigned, implement the node
    assert( Map_NodeReadCutBest(pNodeMap, !fPhase) != NULL || Map_NodeIsConst(pNodeMap) );
    pNodeNew = Abc_NodeFromMapPhase_rec( pNtkNew, pNodeMap, !fPhase );

    // add the inverter
    pNodeInv = Abc_NtkCreateNode( pNtkNew );
    Abc_ObjAddFanin( pNodeInv, pNodeNew );
    pNodeInv->pData = Mio_LibraryReadInv(Map_ManReadGenLib(Map_NodeReadMan(pNodeMap)));

    // set the inverter
    Map_NodeSetData( pNodeMap, fPhase, (char *)pNodeInv );
    return pNodeInv;
}

/**Function*************************************************************

  Synopsis    [Constructs the nodes corrresponding to one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Obj_t * Abc_NodeFromMapPhase_rec( Abc_Ntk_t * pNtkNew, Map_Node_t * pNodeMap, int fPhase )
{
    Abc_Obj_t * pNodePIs[10];
    Abc_Obj_t * pNodeNew;
    Map_Node_t ** ppLeaves;
    Map_Cut_t * pCutBest;
    Map_Super_t * pSuperBest;
    unsigned uPhaseBest;
    int i, fInvPin, nLeaves;

    // make sure the node can be implemented in this phase
    assert( Map_NodeReadCutBest(pNodeMap, fPhase) != NULL || Map_NodeIsConst(pNodeMap) );
    // check if the phase is already implemented
    pNodeNew = (Abc_Obj_t *)Map_NodeReadData( pNodeMap, fPhase );
    if ( pNodeNew )
        return pNodeNew;

    // get the information about the best cut 
    pCutBest   = Map_NodeReadCutBest( pNodeMap, fPhase );
    pSuperBest = Map_CutReadSuperBest( pCutBest, fPhase );
    uPhaseBest = Map_CutReadPhaseBest( pCutBest, fPhase );
    nLeaves    = Map_CutReadLeavesNum( pCutBest );
    ppLeaves   = Map_CutReadLeaves( pCutBest );

    // collect the PI nodes
    for ( i = 0; i < nLeaves; i++ )
    {
        fInvPin = ((uPhaseBest & (1 << i)) > 0);
        pNodePIs[i] = Abc_NodeFromMap_rec( pNtkNew, ppLeaves[i], !fInvPin );
        assert( pNodePIs[i] != NULL );
    }

    // implement the supergate
    pNodeNew = Abc_NodeFromMapSuper_rec( pNtkNew, pNodeMap, pSuperBest, pNodePIs, nLeaves );
    Map_NodeSetData( pNodeMap, fPhase, (char *)pNodeNew );
    return pNodeNew;
}

/**Function*************************************************************

  Synopsis    [Constructs the nodes corrresponding to one supergate.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Obj_t * Abc_NodeFromMapSuper_rec( Abc_Ntk_t * pNtkNew, Map_Node_t * pNodeMap, Map_Super_t * pSuper, Abc_Obj_t * pNodePis[], int nNodePis )
{
    Mio_Gate_t * pRoot;
    Map_Super_t ** ppFanins;
    Abc_Obj_t * pNodeNew, * pNodeFanin;
    int nFanins, Number, i;

    // get the parameters of the supergate
    pRoot = Map_SuperReadRoot(pSuper);
    if ( pRoot == NULL )
    {
        Number = Map_SuperReadNum(pSuper);
        if ( Number < nNodePis )  
        {
            return pNodePis[Number];
        }
        else
        {  
//            assert( 0 );
            /* It might happen that a super gate with 5 inputs is constructed that
             * actually depends only on the first four variables; i.e the fifth is a
             * don't care -- in that case we connect constant node for the fifth
             * (since the cut only has 4 variables). An interesting question is what
             * if the first variable (and not the fifth one is the redundant one;
             * can that happen?) */
            return Abc_NtkCreateNodeConst0(pNtkNew);
        }
    }

    // get information about the fanins of the supergate
    nFanins  = Map_SuperReadFaninNum( pSuper );
    ppFanins = Map_SuperReadFanins( pSuper );
    // create a new node with these fanins
    pNodeNew = Abc_NtkCreateNode( pNtkNew );
    for ( i = 0; i < nFanins; i++ )
    {
        pNodeFanin = Abc_NodeFromMapSuper_rec( pNtkNew, pNodeMap, ppFanins[i], pNodePis, nNodePis );
        Abc_ObjAddFanin( pNodeNew, pNodeFanin );
    }
    pNodeNew->pData = pRoot;
    return pNodeNew;
}





/**Function*************************************************************

  Synopsis    [Interface with the mapping package.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkSuperChoice( Abc_Ntk_t * pNtk )
{
    Abc_Ntk_t * pNtkNew;

    Map_Man_t * pMan;

    assert( Abc_NtkIsStrash(pNtk) );

    // check that the library is available
    if ( Abc_FrameReadLibGen() == NULL )
    {
        printf( "The current library is not available.\n" );
        return 0;
    }

    // derive the supergate library
    if ( Abc_FrameReadLibSuper() == NULL && Abc_FrameReadLibGen() )
    {
        printf( "A simple supergate library is derived from gate library \"%s\".\n", 
            Mio_LibraryReadName(Abc_FrameReadLibGen()) );
        Map_SuperLibDeriveFromGenlib( Abc_FrameReadLibGen() );
    }

    // print a warning about choice nodes
    if ( Abc_NtkGetChoiceNum( pNtk ) )
        printf( "Performing mapping with choices.\n" );

    // perform the mapping
    pMan = Abc_NtkToMap( pNtk, -1, 1, NULL, 0 );
    if ( pMan == NULL )
        return NULL;
    if ( !Map_Mapping( pMan ) )
    {
        Map_ManFree( pMan );
        return NULL;
    }

    // reconstruct the network after mapping
    pNtkNew = Abc_NtkFromMapSuperChoice( pMan, pNtk );
    if ( pNtkNew == NULL )
        return NULL;
    Map_ManFree( pMan );

    // make sure that everything is okay
    if ( !Abc_NtkCheck( pNtkNew ) )
    {
        printf( "Abc_NtkMap: The network check has failed.\n" );
        Abc_NtkDelete( pNtkNew );
        return NULL;
    }
    return pNtkNew;
}


/**Function*************************************************************

  Synopsis    [Creates the mapped network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkFromMapSuperChoice( Map_Man_t * pMan, Abc_Ntk_t * pNtk )
{
    extern Abc_Ntk_t * Abc_NtkMulti( Abc_Ntk_t * pNtk, int nThresh, int nFaninMax, int fCnf, int fMulti, int fSimple, int fFactor );
    ProgressBar * pProgress;
    Abc_Ntk_t * pNtkNew, * pNtkNew2;
    Abc_Obj_t * pNode;
    int i;

    // save the pointer to the mapped nodes
    Abc_NtkForEachCi( pNtk, pNode, i )
        pNode->pNext = pNode->pCopy;
    Abc_NtkForEachPo( pNtk, pNode, i )
        pNode->pNext = pNode->pCopy;
    Abc_NtkForEachNode( pNtk, pNode, i )
        pNode->pNext = pNode->pCopy;

    // duplicate the network
    pNtkNew2 = Abc_NtkDup( pNtk );
    pNtkNew  = Abc_NtkMulti( pNtkNew2, 0, 20, 0, 0, 1, 0 );
    if ( !Abc_NtkBddToSop( pNtkNew, 0 ) )
    {
        printf( "Abc_NtkFromMapSuperChoice(): Converting to SOPs has failed.\n" );
        return NULL;
    }

    // set the old network to point to the new network
    Abc_NtkForEachCi( pNtk, pNode, i )
        pNode->pCopy = pNode->pCopy->pCopy;
    Abc_NtkForEachPo( pNtk, pNode, i )
        pNode->pCopy = pNode->pCopy->pCopy;
    Abc_NtkForEachNode( pNtk, pNode, i )
        pNode->pCopy = pNode->pCopy->pCopy;
    Abc_NtkDelete( pNtkNew2 );

    // set the pointers from the mapper to the new nodes
    Abc_NtkForEachCi( pNtk, pNode, i )
    {
        Map_NodeSetData( Map_ManReadInputs(pMan)[i], 0, (char *)Abc_NtkCreateNodeInv(pNtkNew,pNode->pCopy) );
        Map_NodeSetData( Map_ManReadInputs(pMan)[i], 1, (char *)pNode->pCopy );
    }
    Abc_NtkForEachNode( pNtk, pNode, i )
    {
//        if ( Abc_NodeIsConst(pNode) )
//            continue;
        Map_NodeSetData( (Map_Node_t *)pNode->pNext, 0, (char *)Abc_NtkCreateNodeInv(pNtkNew,pNode->pCopy) );
        Map_NodeSetData( (Map_Node_t *)pNode->pNext, 1, (char *)pNode->pCopy );
    }

    // assign the mapping of the required phase to the POs
    pProgress = Extra_ProgressBarStart( stdout, Abc_NtkObjNumMax(pNtk) );
    Abc_NtkForEachNode( pNtk, pNode, i )
    {
        Extra_ProgressBarUpdate( pProgress, i, NULL );
//        if ( Abc_NodeIsConst(pNode) )
//            continue;
        Abc_NodeSuperChoice( pNtkNew, pNode );
    }
    Extra_ProgressBarStop( pProgress );
    return pNtkNew;
}


/**Function*************************************************************

  Synopsis    [Creates the mapped network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NodeSuperChoice( Abc_Ntk_t * pNtkNew, Abc_Obj_t * pNode )
{
    Map_Node_t * pMapNode = (Map_Node_t *)pNode->pNext;
    Map_Cut_t * pCuts, * pTemp;

    pCuts = Map_NodeReadCuts(pMapNode);
    for ( pTemp = Map_CutReadNext(pCuts); pTemp; pTemp = Map_CutReadNext(pTemp) )
    {
        Abc_NodeFromMapCutPhase( pNtkNew, pTemp, 0 );
        Abc_NodeFromMapCutPhase( pNtkNew, pTemp, 1 );
    }
}


/**Function*************************************************************

  Synopsis    [Constructs the nodes corrresponding to one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NodeFromMapCutPhase( Abc_Ntk_t * pNtkNew, Map_Cut_t * pCut, int fPhase )
{
    Abc_Obj_t * pNodePIs[10];
    Map_Node_t ** ppLeaves;
    Map_Super_t * pSuperBest;
    unsigned uPhaseBest;
    int i, fInvPin, nLeaves;

    pSuperBest = Map_CutReadSuperBest( pCut, fPhase );
    if ( pSuperBest == NULL )
        return;

    // get the information about the best cut 
    uPhaseBest = Map_CutReadPhaseBest( pCut, fPhase );
    nLeaves    = Map_CutReadLeavesNum( pCut );
    ppLeaves   = Map_CutReadLeaves( pCut );

    // collect the PI nodes
    for ( i = 0; i < nLeaves; i++ )
    {
        fInvPin = ((uPhaseBest & (1 << i)) > 0);
        pNodePIs[i] = (Abc_Obj_t *)Map_NodeReadData( ppLeaves[i], !fInvPin );
        assert( pNodePIs[i] != NULL );
    }

    // implement the supergate
    Abc_NodeFromMapSuperChoice_rec( pNtkNew, pSuperBest, pNodePIs, nLeaves );
}


/**Function*************************************************************

  Synopsis    [Constructs the nodes corrresponding to one supergate.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Obj_t * Abc_NodeFromMapSuperChoice_rec( Abc_Ntk_t * pNtkNew, Map_Super_t * pSuper, Abc_Obj_t * pNodePis[], int nNodePis )
{
    Mio_Gate_t * pRoot;
    Map_Super_t ** ppFanins;
    Abc_Obj_t * pNodeNew, * pNodeFanin;
    int nFanins, Number, i;

    // get the parameters of the supergate
    pRoot = Map_SuperReadRoot(pSuper);
    if ( pRoot == NULL )
    {
        Number = Map_SuperReadNum(pSuper);
        if ( Number < nNodePis )  
        {
            return pNodePis[Number];
        }
        else
        {  
//            assert( 0 );
            /* It might happen that a super gate with 5 inputs is constructed that
             * actually depends only on the first four variables; i.e the fifth is a
             * don't care -- in that case we connect constant node for the fifth
             * (since the cut only has 4 variables). An interesting question is what
             * if the first variable (and not the fifth one is the redundant one;
             * can that happen?) */
            return Abc_NtkCreateNodeConst0(pNtkNew);
        }
    }

    // get information about the fanins of the supergate
    nFanins  = Map_SuperReadFaninNum( pSuper );
    ppFanins = Map_SuperReadFanins( pSuper );
    // create a new node with these fanins
    pNodeNew = Abc_NtkCreateNode( pNtkNew );
    for ( i = 0; i < nFanins; i++ )
    {
        pNodeFanin = Abc_NodeFromMapSuperChoice_rec( pNtkNew, ppFanins[i], pNodePis, nNodePis );
        Abc_ObjAddFanin( pNodeNew, pNodeFanin );
    }
    pNodeNew->pData = Abc_SopRegister( pNtkNew->pManFunc, Mio_GateReadSop(pRoot) );
    return pNodeNew;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


