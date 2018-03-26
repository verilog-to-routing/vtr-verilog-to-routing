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

#include "base/abc/abc.h"
#include "base/main/main.h"
#include "map/mio/mio.h"
#include "map/mapper/mapper.h"
#include "misc/util/utilNam.h"
#include "map/scl/sclCon.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static Map_Man_t *  Abc_NtkToMap( Abc_Ntk_t * pNtk, double DelayTarget, int fRecovery, float * pSwitching, int fVerbose );
static Abc_Ntk_t *  Abc_NtkFromMap( Map_Man_t * pMan, Abc_Ntk_t * pNtk );
static Abc_Obj_t *  Abc_NodeFromMap_rec( Abc_Ntk_t * pNtkNew, Map_Node_t * pNodeMap, int fPhase );
static Abc_Obj_t *  Abc_NodeFromMapPhase_rec( Abc_Ntk_t * pNtkNew, Map_Node_t * pNodeMap, int fPhase );

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
Abc_Ntk_t * Abc_NtkMap( Abc_Ntk_t * pNtk, double DelayTarget, double AreaMulti, double DelayMulti, float LogFan, float Slew, float Gain, int nGatesMin, int fRecovery, int fSwitching, int fSkipFanout, int fUseProfile, int fVerbose )
{
    static int fUseMulti = 0;
    int fShowSwitching = 1;
    Abc_Ntk_t * pNtkNew;
    Map_Man_t * pMan;
    Vec_Int_t * vSwitching = NULL;
    float * pSwitching = NULL;
    abctime clk, clkTotal = Abc_Clock();
    Mio_Library_t * pLib = (Mio_Library_t *)Abc_FrameReadLibGen();

    assert( Abc_NtkIsStrash(pNtk) );
    // derive library from SCL
    // if the library is created here, it will be deleted when pSuperLib is deleted in Map_SuperLibFree()
    if ( Abc_FrameReadLibScl() && Abc_SclHasDelayInfo( Abc_FrameReadLibScl() ) )
    {
        if ( pLib && Mio_LibraryHasProfile(pLib) )
            pLib = Abc_SclDeriveGenlib( Abc_FrameReadLibScl(), pLib, Slew, Gain, nGatesMin, fVerbose );
        else
            pLib = Abc_SclDeriveGenlib( Abc_FrameReadLibScl(), NULL, Slew, Gain, nGatesMin, fVerbose );
        if ( Abc_FrameReadLibGen() )
        {
            Mio_LibraryTransferDelays( (Mio_Library_t *)Abc_FrameReadLibGen(), pLib );
            Mio_LibraryTransferProfile( pLib, (Mio_Library_t *)Abc_FrameReadLibGen() );
        }
        // remove supergate library
        Map_SuperLibFree( (Map_SuperLib_t *)Abc_FrameReadLibSuper() );
        Abc_FrameSetLibSuper( NULL );
    }
    // quit if there is no library
    if ( pLib == NULL )
    {
        printf( "The current library is not available.\n" );
        return 0;
    }
    if ( AreaMulti != 0.0 )
        fUseMulti = 1, printf( "The cell areas are multiplied by the factor: <num_fanins> ^ (%.2f).\n", AreaMulti );
    if ( DelayMulti != 0.0 )
        fUseMulti = 1, printf( "The cell delays are multiplied by the factor: <num_fanins> ^ (%.2f).\n", DelayMulti );

    // penalize large gates by increasing their area
    if ( AreaMulti != 0.0 )
        Mio_LibraryMultiArea( pLib, AreaMulti );
    if ( DelayMulti != 0.0 )
        Mio_LibraryMultiDelay( pLib, DelayMulti );

    // derive the supergate library
    if ( fUseMulti || Abc_FrameReadLibSuper() == NULL )
    {
        if ( fVerbose )
            printf( "Converting \"%s\" into supergate library \"%s\".\n", 
                Mio_LibraryReadName(pLib), Extra_FileNameGenericAppend(Mio_LibraryReadName(pLib), ".super") );
        // compute supergate library to be used for mapping
        if ( Mio_LibraryHasProfile(pLib) )
            printf( "Abc_NtkMap(): Genlib library has profile.\n" );
        Map_SuperLibDeriveFromGenlib( pLib, fVerbose );
    }

    // return the library to normal
    if ( AreaMulti != 0.0 )
        Mio_LibraryMultiArea( (Mio_Library_t *)Abc_FrameReadLibGen(), -AreaMulti );
    if ( DelayMulti != 0.0 )
        Mio_LibraryMultiDelay( (Mio_Library_t *)Abc_FrameReadLibGen(), -DelayMulti );

    // print a warning about choice nodes
    if ( fVerbose && Abc_NtkGetChoiceNum( pNtk ) )
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
clk = Abc_Clock();
    Map_ManSetSwitching( pMan, fSwitching );
    Map_ManSetSkipFanout( pMan, fSkipFanout );
    if ( fUseProfile )
        Map_ManSetUseProfile( pMan );
    if ( LogFan != 0 )
        Map_ManCreateNodeDelays( pMan, LogFan );
    if ( !Map_Mapping( pMan ) )
    {
        Map_ManFree( pMan );
        return NULL;
    }
//    Map_ManPrintStatsToFile( pNtk->pSpec, Map_ManReadAreaFinal(pMan), Map_ManReadRequiredGlo(pMan), Abc_Clock()-clk );

    // reconstruct the network after mapping
    pNtkNew = Abc_NtkFromMap( pMan, pNtk );
    if ( Mio_LibraryHasProfile(pLib) )
        Mio_LibraryTransferProfile2( (Mio_Library_t *)Abc_FrameReadLibGen(), pLib );
    Map_ManFree( pMan );
    if ( pNtkNew == NULL )
        return NULL;

    if ( pNtk->pExdc )
        pNtkNew->pExdc = Abc_NtkDup( pNtk->pExdc );
if ( fVerbose )
{
ABC_PRT( "Total runtime", Abc_Clock() - clkTotal );
}

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
Map_Time_t * Abc_NtkMapCopyCiArrival( Abc_Ntk_t * pNtk, Abc_Time_t * ppTimes )
{
    Map_Time_t * p;
    int i;
    p = ABC_CALLOC( Map_Time_t, Abc_NtkCiNum(pNtk) );
    for ( i = 0; i < Abc_NtkCiNum(pNtk); i++ )
    {
        p[i].Fall = ppTimes[i].Fall;
        p[i].Rise = ppTimes[i].Rise;
        p[i].Worst = Abc_MaxFloat( p[i].Fall, p[i].Rise );
    }
    ABC_FREE( ppTimes );
    return p;
}
Map_Time_t * Abc_NtkMapCopyCoRequired( Abc_Ntk_t * pNtk, Abc_Time_t * ppTimes )
{
    Map_Time_t * p;
    int i;
    p = ABC_CALLOC( Map_Time_t, Abc_NtkCoNum(pNtk) );
    for ( i = 0; i < Abc_NtkCoNum(pNtk); i++ )
    {
        p[i].Fall = ppTimes[i].Fall;
        p[i].Rise = ppTimes[i].Rise;
        p[i].Worst = Abc_MaxFloat( p[i].Fall, p[i].Rise );
    }
    ABC_FREE( ppTimes );
    return p;
}

/**Function*************************************************************

  Synopsis    [Load the network into manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Map_Time_t * Abc_NtkMapCopyCiArrivalCon( Abc_Ntk_t * pNtk )
{
    Map_Time_t * p; int i;
    p = ABC_CALLOC( Map_Time_t, Abc_NtkCiNum(pNtk) );
    for ( i = 0; i < Abc_NtkCiNum(pNtk); i++ )
        p[i].Fall = p[i].Rise = p[i].Worst = Scl_Int2Flt( Scl_ConGetInArr(i) );
    return p;
}
Map_Time_t * Abc_NtkMapCopyCoRequiredCon( Abc_Ntk_t * pNtk )
{
    Map_Time_t * p; int i;
    p = ABC_CALLOC( Map_Time_t, Abc_NtkCoNum(pNtk) );
    for ( i = 0; i < Abc_NtkCoNum(pNtk); i++ )
        p[i].Fall = p[i].Rise = p[i].Worst = Scl_Int2Flt( Scl_ConGetOutReq(i) );
    return p;
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
    Map_Node_t * pNodeMap;
    Vec_Ptr_t * vNodes;
    Abc_Obj_t * pNode, * pFanin, * pPrev;
    int i;

    assert( Abc_NtkIsStrash(pNtk) );

    // start the mapping manager and set its parameters
    pMan = Map_ManCreate( Abc_NtkPiNum(pNtk) + Abc_NtkLatchNum(pNtk) - pNtk->nBarBufs, Abc_NtkPoNum(pNtk) + Abc_NtkLatchNum(pNtk) - pNtk->nBarBufs, fVerbose );
    if ( pMan == NULL )
        return NULL;
    Map_ManSetAreaRecovery( pMan, fRecovery );
    Map_ManSetOutputNames( pMan, Abc_NtkCollectCioNames(pNtk, 1) );
    Map_ManSetDelayTarget( pMan, (float)DelayTarget );

    // set arrival and requireds
    if ( Scl_ConIsRunning() && Scl_ConHasInArrs() )
        Map_ManSetInputArrivals( pMan, Abc_NtkMapCopyCiArrivalCon(pNtk) );
    else
        Map_ManSetInputArrivals( pMan, Abc_NtkMapCopyCiArrival(pNtk, Abc_NtkGetCiArrivalTimes(pNtk)) );
    if ( Scl_ConIsRunning() && Scl_ConHasOutReqs() )
        Map_ManSetOutputRequireds( pMan, Abc_NtkMapCopyCoRequiredCon(pNtk) );
    else
        Map_ManSetOutputRequireds( pMan, Abc_NtkMapCopyCoRequired(pNtk, Abc_NtkGetCoRequiredTimes(pNtk)) );

    // create PIs and remember them in the old nodes
    Abc_NtkCleanCopy( pNtk );
    Abc_AigConst1(pNtk)->pCopy = (Abc_Obj_t *)Map_ManReadConst1(pMan);
    Abc_NtkForEachCi( pNtk, pNode, i )
    {
        if ( i == Abc_NtkCiNum(pNtk) - pNtk->nBarBufs )
            break;
        pNodeMap = Map_ManReadInputs(pMan)[i];
        pNode->pCopy = (Abc_Obj_t *)pNodeMap;
        if ( pSwitching )
            Map_NodeSetSwitching( pNodeMap, pSwitching[pNode->Id] );
    }

    // load the AIG into the mapper
    vNodes = Abc_AigDfsMap( pNtk );
    Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pNode, i )
    {
        if ( Abc_ObjIsLatch(pNode) )
        {
            pFanin = Abc_ObjFanin0(pNode);
            pNodeMap = Map_NodeBuf( pMan, Map_NotCond( Abc_ObjFanin0(pFanin)->pCopy, (int)Abc_ObjFaninC0(pFanin) ) );
            Abc_ObjFanout0(pNode)->pCopy = (Abc_Obj_t *)pNodeMap;
            continue;
        }
        assert( Abc_ObjIsNode(pNode) );
        // add the node to the mapper
        pNodeMap = Map_NodeAnd( pMan, 
            Map_NotCond( Abc_ObjFanin0(pNode)->pCopy, (int)Abc_ObjFaninC0(pNode) ),
            Map_NotCond( Abc_ObjFanin1(pNode)->pCopy, (int)Abc_ObjFaninC1(pNode) ) );
        assert( pNode->pCopy == NULL );
        // remember the node
        pNode->pCopy = (Abc_Obj_t *)pNodeMap;
        if ( pSwitching )
            Map_NodeSetSwitching( pNodeMap, pSwitching[pNode->Id] );
        // set up the choice node
        if ( Abc_AigNodeIsChoice( pNode ) )
            for ( pPrev = pNode, pFanin = (Abc_Obj_t *)pNode->pData; pFanin; pPrev = pFanin, pFanin = (Abc_Obj_t *)pFanin->pData )
            {
                Map_NodeSetNextE( (Map_Node_t *)pPrev->pCopy, (Map_Node_t *)pFanin->pCopy );
                Map_NodeSetRepr( (Map_Node_t *)pFanin->pCopy, (Map_Node_t *)pNode->pCopy );
            }
    }
    assert( Map_ManReadBufNum(pMan) == pNtk->nBarBufs );
    Vec_PtrFree( vNodes );

    // set the primary outputs in the required phase
    Abc_NtkForEachCo( pNtk, pNode, i )
    {
        if ( i == Abc_NtkCoNum(pNtk) - pNtk->nBarBufs )
            break;
        Map_ManReadOutputs(pMan)[i] = Map_NotCond( (Map_Node_t *)Abc_ObjFanin0(pNode)->pCopy, (int)Abc_ObjFaninC0(pNode) );
    }
    return pMan;
}

/**Function*************************************************************

  Synopsis    [Creates the mapped network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Obj_t * Abc_NodeFromMapSuper_rec( Abc_Ntk_t * pNtkNew, Map_Node_t * pNodeMap, Map_Super_t * pSuper, Abc_Obj_t * pNodePis[], int nNodePis )
{
    Mio_Library_t * pLib = (Mio_Library_t *)Abc_FrameReadLibGen();
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
    pRoot = Mio_LibraryReadGateByName( pLib, Mio_GateReadName(pRoot), NULL );

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
Abc_Obj_t * Abc_NodeFromMap_rec( Abc_Ntk_t * pNtkNew, Map_Node_t * pNodeMap, int fPhase )
{
    Abc_Obj_t * pNodeNew, * pNodeInv;

    // check the case of constant node
    if ( Map_NodeIsConst(pNodeMap) )
    {
        pNodeNew = fPhase? Abc_NtkCreateNodeConst1(pNtkNew) : Abc_NtkCreateNodeConst0(pNtkNew);
        if ( pNodeNew->pData == NULL )
            printf( "Error creating mapped network: Library does not have a constant %d gate.\n", fPhase );
        return pNodeNew;
    }

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
    pNodeInv->pData = Mio_LibraryReadInv((Mio_Library_t *)Abc_FrameReadLibGen());

    // set the inverter
    Map_NodeSetData( pNodeMap, fPhase, (char *)pNodeInv );
    return pNodeInv;
}
Abc_Ntk_t * Abc_NtkFromMap( Map_Man_t * pMan, Abc_Ntk_t * pNtk )
{
    Abc_Ntk_t * pNtkNew;
    Map_Node_t * pNodeMap;
    Abc_Obj_t * pNode, * pNodeNew;
    int i, nDupGates;
    assert( Map_ManReadBufNum(pMan) == pNtk->nBarBufs );
    // create the new network
    pNtkNew = Abc_NtkStartFrom( pNtk, ABC_NTK_LOGIC, ABC_FUNC_MAP );
    // make the mapper point to the new network
    Map_ManCleanData( pMan );
    Abc_NtkForEachCi( pNtk, pNode, i )
    {
        if ( i >= Abc_NtkCiNum(pNtk) - pNtk->nBarBufs )
            break;
        Map_NodeSetData( Map_ManReadInputs(pMan)[i], 1, (char *)pNode->pCopy );
    }
    Abc_NtkForEachCi( pNtk, pNode, i )
    {
        if ( i < Abc_NtkCiNum(pNtk) - pNtk->nBarBufs )
            continue;
        Map_NodeSetData( Map_ManReadBufs(pMan)[i - (Abc_NtkCiNum(pNtk) - pNtk->nBarBufs)], 1, (char *)pNode->pCopy );
    }
    // assign the mapping of the required phase to the POs
    Abc_NtkForEachCo( pNtk, pNode, i )
    {
        if ( i < Abc_NtkCoNum(pNtk) - pNtk->nBarBufs )
            continue;
        pNodeMap = Map_ManReadBufDriver( pMan, i - (Abc_NtkCoNum(pNtk) - pNtk->nBarBufs) );
        pNodeNew = Abc_NodeFromMap_rec( pNtkNew, Map_Regular(pNodeMap), !Map_IsComplement(pNodeMap) );
        assert( !Abc_ObjIsComplement(pNodeNew) );
        Abc_ObjAddFanin( pNode->pCopy, pNodeNew );
    }
    Abc_NtkForEachCo( pNtk, pNode, i )
    {
        if ( i >= Abc_NtkCoNum(pNtk) - pNtk->nBarBufs )
            break;
        pNodeMap = Map_ManReadOutputs(pMan)[i];
        pNodeNew = Abc_NodeFromMap_rec( pNtkNew, Map_Regular(pNodeMap), !Map_IsComplement(pNodeMap) );
        assert( !Abc_ObjIsComplement(pNodeNew) );
        Abc_ObjAddFanin( pNode->pCopy, pNodeNew );
    }
    // decouple the PO driver nodes to reduce the number of levels
    nDupGates = Abc_NtkLogicMakeSimpleCos( pNtkNew, 1 );
//    if ( nDupGates && Map_ManReadVerbose(pMan) )
//        printf( "Duplicated %d gates to decouple the CO drivers.\n", nDupGates );
    return pNtkNew;
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
//        printf( "A simple supergate library is derived from gate library \"%s\".\n", 
//            Mio_LibraryReadName((Mio_Library_t *)Abc_FrameReadLibGen()) );
        Map_SuperLibDeriveFromGenlib( (Mio_Library_t *)Abc_FrameReadLibGen(), 0 );
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
    if ( !Abc_NtkBddToSop( pNtkNew, -1, ABC_INFINITY ) )
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
    Mio_Library_t * pLib = (Mio_Library_t *)Abc_FrameReadLibGen();
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
    pRoot = Mio_LibraryReadGateByName( pLib, Mio_GateReadName(pRoot), NULL );

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
    pNodeNew->pData = Abc_SopRegister( (Mem_Flex_t *)pNtkNew->pManFunc, Mio_GateReadSop(pRoot) );
    return pNodeNew;
}

/**Function*************************************************************

  Synopsis    [Returns the twin node if it exists.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Obj_t * Abc_NtkFetchTwinNode( Abc_Obj_t * pNode )
{
    Abc_Obj_t * pNode2;
    Mio_Gate_t * pGate = (Mio_Gate_t *)pNode->pData;
    assert( Abc_NtkHasMapping(pNode->pNtk) );
    if ( pGate == NULL || Mio_GateReadTwin(pGate) == NULL )
        return NULL;
    // assuming the twin node is following next
    if ( (int)Abc_ObjId(pNode) == Abc_NtkObjNumMax(pNode->pNtk) - 1 )
        return NULL;
    pNode2 = Abc_NtkObj( pNode->pNtk, Abc_ObjId(pNode) + 1 );
    if ( pNode2 == NULL || !Abc_ObjIsNode(pNode2) || Abc_ObjFaninNum(pNode) != Abc_ObjFaninNum(pNode2) )
        return NULL;
    if ( Mio_GateReadTwin(pGate) != (Mio_Gate_t *)pNode2->pData )
        return NULL;
    return pNode2;
}


/**Function*************************************************************

  Synopsis    [Dumps mapped network in the mini-mapped format.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Abc_NtkWriteMiniMapping( Abc_Ntk_t * pNtk )
{
    Vec_Ptr_t * vNodes;
    Vec_Int_t * vMapping;
    Vec_Str_t * vGates;
    Abc_Obj_t * pObj, * pFanin;
    int i, k, nNodes, nFanins, nExtra, * pArray;
    assert( Abc_NtkHasMapping(pNtk) );
    // collect nodes in the DFS order
    vNodes = Abc_NtkDfs( pNtk, 0 );
    // assign unique numbers
    nNodes = nFanins = 0;
    Abc_NtkForEachCi( pNtk, pObj, i )
        pObj->iTemp = nNodes++;
    Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pObj, i )
        pObj->iTemp = nNodes++, nFanins += Abc_ObjFaninNum(pObj);
    // allocate attay to store mapping (4 counters + fanins for each node + PO drivers + gate names)
    vMapping = Vec_IntAlloc( 4 + Abc_NtkNodeNum(pNtk) + nFanins + Abc_NtkCoNum(pNtk) + 10000 );
    // write the numbers of CI/CO/Node/FF
    Vec_IntPush( vMapping, Abc_NtkCiNum(pNtk) );
    Vec_IntPush( vMapping, Abc_NtkCoNum(pNtk) );
    Vec_IntPush( vMapping, Abc_NtkNodeNum(pNtk) );
    Vec_IntPush( vMapping, Abc_NtkLatchNum(pNtk) );
    // write the nodes
    vGates = Vec_StrAlloc( 10000 );
    Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pObj, i )
    {
        Vec_IntPush( vMapping, Abc_ObjFaninNum(pObj) );
        Abc_ObjForEachFanin( pObj, pFanin, k )
            Vec_IntPush( vMapping, pFanin->iTemp );
        // remember this gate (to be added to the mapping later)
        Vec_StrPrintStr( vGates, Mio_GateReadName((Mio_Gate_t *)pObj->pData) );
        Vec_StrPush( vGates, '\0' );
    }
    // write the COs literals
    Abc_NtkForEachCo( pNtk, pObj, i )
        Vec_IntPush( vMapping, Abc_ObjFanin0(pObj)->iTemp );
    // finish off the array
    nExtra = 4 - Vec_StrSize(vGates) % 4;
    for ( i = 0; i < nExtra; i++ )
        Vec_StrPush( vGates, '\0' );
    // add gates to the array
    assert( Vec_StrSize(vGates) % 4 == 0 );
    nExtra = Vec_StrSize(vGates) / 4;
    pArray = (int *)Vec_StrArray(vGates);
    for ( i = 0; i < nExtra; i++ )
        Vec_IntPush( vMapping, pArray[i] );
    // cleanup and return
    Vec_PtrFree( vNodes );
    Vec_StrFree( vGates );
    return vMapping;
}

/**Function*************************************************************

  Synopsis    [Prints mapped network represented in mini-mapped format.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkPrintMiniMapping( int * pArray )
{
    int nCis, nCos, nNodes, nFlops;
    int i, k, nLeaves, Pos = 4;
    char * pBuffer, * pName;
    nCis = pArray[0];
    nCos = pArray[1];
    nNodes = pArray[2];
    nFlops = pArray[3];
    printf( "Mapped network has %d CIs, %d COs, %d gates, and %d flops.\n", nCis, nCos, nNodes, nFlops );
    printf( "The first %d object IDs (from 0 to %d) are reserved for the CIs.\n", nCis, nCis - 1 );
    for ( i = 0; i < nNodes; i++ )
    {
        printf( "Node %d has fanins {", nCis + i );
        nLeaves = pArray[Pos++];
        for ( k = 0; k < nLeaves; k++ )
            printf( " %d", pArray[Pos++] );
        printf( " }\n" );
    }
    for ( i = 0; i < nCos; i++ )
        printf( "CO %d is driven by node %d\n", i, pArray[Pos++] );
    pBuffer = (char *)(pArray + Pos);
    for ( i = 0; i < nNodes; i++ )
    {
        pName = pBuffer;
        pBuffer += strlen(pName) + 1;
        printf( "Node %d has gate \"%s\"\n", nCis + i, pName );
    }
}

/**Function*************************************************************

  Synopsis    [This procedure outputs an array representing mini-mapped network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int * Abc_NtkOutputMiniMapping( Abc_Frame_t * pAbc )
{
    //Abc_Frame_t * pAbc = (Abc_Frame_t *)pAbc0;
    Abc_Ntk_t * pNtk;
    Vec_Int_t * vMapping;
    int * pArray;
    if ( pAbc == NULL )
        printf( "ABC framework is not initialized by calling Abc_Start()\n" );
    pNtk = Abc_FrameReadNtk( pAbc );
    if ( pNtk == NULL )
        printf( "Current network in ABC framework is not defined.\n" );
    if ( !Abc_NtkHasMapping(pNtk) )
        printf( "Current network in ABC framework is not mapped.\n" );
    // derive mini-mapping
    vMapping = Abc_NtkWriteMiniMapping( pNtk );
    pArray = Vec_IntArray( vMapping );
    ABC_FREE( vMapping );
    // print mini-mapping (optional)
//    Abc_NtkPrintMiniMapping( pArray );
    // return the array representation of mini-mapping
    return pArray;
}

/**Function*************************************************************

  Synopsis    [Test for mini-mapped format.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkTestMiniMapping( Abc_Ntk_t * p )
{
    Vec_Int_t * vMapping;
    vMapping = Abc_NtkWriteMiniMapping( p );
    Abc_NtkPrintMiniMapping( Vec_IntArray(vMapping) );
    printf( "Array has size %d ints.\n", Vec_IntSize(vMapping) );
    Vec_IntFree( vMapping );
}

/**Function*************************************************************

  Synopsis    [These APIs set arrival/required times of CIs/COs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkSetCiArrivalTime( Abc_Frame_t * pAbc, int iCi, float Rise, float Fall )
{
    //Abc_Frame_t * pAbc = (Abc_Frame_t *)pAbc0;
    Abc_Ntk_t * pNtk;
    Abc_Obj_t * pNode;
    if ( pAbc == NULL )
    {
        printf( "ABC framework is not initialized by calling Abc_Start()\n" );
        return;
    }
    pNtk = Abc_FrameReadNtk( pAbc );
    if ( pNtk == NULL )
    {
        printf( "Current network in ABC framework is not defined.\n" );
        return;
    }
    if ( iCi < 0 || iCi >= Abc_NtkCiNum(pNtk) )
    {
        printf( "CI index is not valid.\n" );
        return;
    }
    pNode = Abc_NtkCi( pNtk, iCi );
    Abc_NtkTimeSetArrival( pNtk, Abc_ObjId(pNode), Rise, Fall );
}
void Abc_NtkSetCoRequiredTime( Abc_Frame_t * pAbc, int iCo, float Rise, float Fall )
{
    //Abc_Frame_t * pAbc = (Abc_Frame_t *)pAbc0;
    Abc_Ntk_t * pNtk;
    Abc_Obj_t * pNode;
    if ( pAbc == NULL )\
    {
        printf( "ABC framework is not initialized by calling Abc_Start()\n" );
        return;
    }
    pNtk = Abc_FrameReadNtk( pAbc );
    if ( pNtk == NULL )
    {
        printf( "Current network in ABC framework is not defined.\n" );
        return;
    }
    if ( iCo < 0 || iCo >= Abc_NtkCoNum(pNtk) )
    {
        printf( "CO index is not valid.\n" );
        return;
    }
    pNode = Abc_NtkCo( pNtk, iCo );
    Abc_NtkTimeSetRequired( pNtk, Abc_ObjId(pNode), Rise, Fall );
}

/**Function*************************************************************

  Synopsis    [This APIs set AND gate delay.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkSetAndGateDelay( Abc_Frame_t * pAbc, float Delay )
{
    //Abc_Frame_t * pAbc = (Abc_Frame_t *)pAbc0;
    Abc_Ntk_t * pNtk;
    if ( pAbc == NULL )
    {
        printf( "ABC framework is not initialized by calling Abc_Start()\n" );
        return;
    }
    pNtk = Abc_FrameReadNtk( pAbc );
    if ( pNtk == NULL )
    {
        printf( "Current network in ABC framework is not defined.\n" );
        return;
    }
    pNtk->AndGateDelay = Delay;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

