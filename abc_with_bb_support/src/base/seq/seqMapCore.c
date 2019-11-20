/**CFile****************************************************************

  FileName    [seqMapCore.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Construction and manipulation of sequential AIGs.]

  Synopsis    [The core of SC mapping/retiming package.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: seqMapCore.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "seqInt.h"
#include "main.h"
#include "mio.h"
#include "mapper.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

extern Abc_Ntk_t *  Seq_NtkMapDup( Abc_Ntk_t * pNtk );
extern int          Seq_NtkMapInitCompatible( Abc_Ntk_t * pNtk, int fVerbose );
extern Abc_Ntk_t *  Seq_NtkSeqMapMapped( Abc_Ntk_t * pNtk );

static int          Seq_MapMappingCount( Abc_Ntk_t * pNtk );
static int          Seq_MapMappingCount_rec( Abc_Ntk_t * pNtk, unsigned SeqEdge, Vec_Ptr_t * vLeaves );
static Abc_Obj_t *  Seq_MapMappingBuild_rec( Abc_Ntk_t * pNtkNew, Abc_Ntk_t * pNtk, unsigned SeqEdge, int fTop, int fCompl, int LagCut, Vec_Ptr_t * vLeaves, unsigned uPhase );
static DdNode *     Seq_MapMappingBdd_rec( DdManager * dd, Abc_Ntk_t * pNtk, unsigned SeqEdge, Vec_Ptr_t * vLeaves );
static void         Seq_MapMappingEdges_rec( Abc_Ntk_t * pNtk, unsigned SeqEdge, Abc_Obj_t * pPrev, Vec_Ptr_t * vLeaves, Vec_Vec_t * vMapEdges );
static void         Seq_MapMappingConnect_rec( Abc_Ntk_t * pNtk, unsigned SeqEdge, Abc_Obj_t * pPrev, int Edge, Abc_Obj_t * pRoot, Vec_Ptr_t * vLeaves );
static DdNode *     Seq_MapMappingConnectBdd_rec( Abc_Ntk_t * pNtk, unsigned SeqEdge, Abc_Obj_t * pPrev, int Edge, Abc_Obj_t * pRoot, Vec_Ptr_t * vLeaves );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Performs Map mapping and retiming.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Seq_MapRetime( Abc_Ntk_t * pNtk, int nMaxIters, int fVerbose )
{
    Abc_Seq_t * p = pNtk->pManFunc;
    Abc_Ntk_t * pNtkNew;
    Abc_Ntk_t * pNtkMap;
    int RetValue;

    // derive the supergate library
    if ( Abc_FrameReadLibSuper() == NULL && Abc_FrameReadLibGen() )
    {
        printf( "A simple supergate library is derived from gate library \"%s\".\n", 
            Mio_LibraryReadName(Abc_FrameReadLibGen()) );
        Map_SuperLibDeriveFromGenlib( Abc_FrameReadLibGen() );
    }
    p->pSuperLib   = Abc_FrameReadLibSuper();
    p->nVarsMax    = Map_SuperLibReadVarsMax(p->pSuperLib);
    p->nMaxIters   = nMaxIters;
    p->fStandCells = 1;

    // find the best mapping and retiming for all nodes (p->vLValues, p->vBestCuts, p->vLags)
    if ( !Seq_MapRetimeDelayLags( pNtk, fVerbose ) )
        return NULL;
    if ( RetValue = Abc_NtkGetChoiceNum(pNtk) )
    {
        printf( "The network has %d choices. The resulting network is not derived (this is temporary).\n", RetValue );
        printf( "The mininum clock period computed is %5.2f.\n", p->FiBestFloat );
        return NULL;
    }
    printf( "The mininum clock period computed is %5.2f.\n", p->FiBestFloat );
    printf( "The resulting network is derived as BDD logic network (this is temporary).\n" );

    // duplicate the nodes contained in multiple cuts
    pNtkNew = Seq_NtkMapDup( pNtk );
    
    // implement the retiming
    RetValue = Seq_NtkImplementRetiming( pNtkNew, ((Abc_Seq_t *)pNtkNew->pManFunc)->vLags, fVerbose );
    if ( RetValue == 0 )
        printf( "Retiming completed but initial state computation has failed.\n" );

    // check the compatibility of initial states computed
    if ( RetValue = Seq_NtkMapInitCompatible( pNtkNew, fVerbose ) )
        printf( "The number of LUTs with incompatible edges = %d.\n", RetValue );
//    return pNtkNew;

    // create the final mapped network
    pNtkMap = Seq_NtkSeqMapMapped( pNtkNew );
    Abc_NtkDelete( pNtkNew );
    return pNtkMap;
}

/**Function*************************************************************

  Synopsis    [Derives the network by duplicating some of the nodes.]

  Description [Information about mapping is given as mapping nodes (p->vMapAnds)
  and best cuts for each node (p->vMapCuts).]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Seq_NtkMapDup( Abc_Ntk_t * pNtk )
{
    Abc_Seq_t * pNew, * p = pNtk->pManFunc;
    Seq_Match_t * pMatch;
    Abc_Ntk_t * pNtkNew; 
    Abc_Obj_t * pObj, * pFanin, * pFaninNew, * pLeaf;
    Vec_Ptr_t * vLeaves;
    unsigned SeqEdge;
    int i, k, nObjsNew, Lag;
    
    assert( Abc_NtkIsSeq(pNtk) );

    // start the expanded network
    pNtkNew = Abc_NtkStartFrom( pNtk, pNtk->ntkType, pNtk->ntkFunc );
    Abc_NtkCleanNext(pNtk);

    // start the new sequential AIG manager
    nObjsNew = 1 + Abc_NtkPiNum(pNtk) + Abc_NtkPoNum(pNtk) + Seq_MapMappingCount(pNtk);
    Seq_Resize( pNtkNew->pManFunc, nObjsNew );

    // duplicate the nodes in the mapping
    Vec_PtrForEachEntry( p->vMapAnds, pMatch, i )
    {
//        Abc_NtkDupObj( pNtkNew, pMatch->pAnd );
        if ( !pMatch->fCompl )
            pMatch->pAnd->pCopy = Abc_NtkCreateNode( pNtkNew );
        else
            pMatch->pAnd->pNext = Abc_NtkCreateNode( pNtkNew );
    }

    // compute the real phase assignment
    Vec_PtrForEachEntry( p->vMapAnds, pMatch, i )
    {
        pMatch->uPhaseR = 0;
        // get the leaves of the cut
        vLeaves = Vec_VecEntry( p->vMapCuts, i );
        // convert the leaf nodes
        Vec_PtrForEachEntry( vLeaves, pLeaf, k )
        {
            SeqEdge = (unsigned)pLeaf;
            pLeaf = Abc_NtkObj( pNtk, SeqEdge >> 8 );

            // set the phase
            if ( pMatch->uPhase & (1 << k) ) // neg is required
            {
                if ( pLeaf->pNext ) // neg is available
                    pMatch->uPhaseR |= (1 << k); // neg is used
//                else
//                    Seq_NodeSetLag( pLeaf, Seq_NodeGetLagN(pLeaf) );
            }
            else // pos is required
            {
                if ( pLeaf->pCopy == NULL ) // pos is not available
                    pMatch->uPhaseR |= (1 << k); // neg is used
//                else
//                    Seq_NodeSetLagN( pLeaf, Seq_NodeGetLag(pLeaf) );
            }
        }
    }


    // recursively construct the internals of each node
    Vec_PtrForEachEntry( p->vMapAnds, pMatch, i )
    {
//        if ( pMatch->pSuper == NULL )
//        {
//            int x = 0;
//        }
        vLeaves = Vec_VecEntry( p->vMapCuts, i );
        if ( !pMatch->fCompl )
            Seq_MapMappingBuild_rec( pNtkNew, pNtk, pMatch->pAnd->Id << 8, 1, pMatch->fCompl, Seq_NodeGetLag(pMatch->pAnd), vLeaves, pMatch->uPhaseR );
        else
            Seq_MapMappingBuild_rec( pNtkNew, pNtk, pMatch->pAnd->Id << 8, 1, pMatch->fCompl, Seq_NodeGetLagN(pMatch->pAnd), vLeaves, pMatch->uPhaseR );
    }
    assert( nObjsNew == pNtkNew->nObjs );

    // set the POs
//    Abc_NtkFinalize( pNtk, pNtkNew );
    Abc_NtkForEachPo( pNtk, pObj, i )
    {
        pFanin = Abc_ObjFanin0(pObj);
        if ( Abc_ObjFaninC0(pObj) )
            pFaninNew = pFanin->pNext ? pFanin->pNext : pFanin->pCopy;
        else
            pFaninNew = pFanin->pCopy ? pFanin->pCopy : pFanin->pNext;
        pFaninNew = Abc_ObjNotCond( pFaninNew, Abc_ObjFaninC0(pObj) );
        Abc_ObjAddFanin( pObj->pCopy, pFaninNew );
    }

    // duplicate the latches on the PO edges
    Abc_NtkForEachPo( pNtk, pObj, i )
        Seq_NodeDupLats( pObj->pCopy, pObj, 0 );

    // transfer the mapping info to the new manager
    Vec_PtrForEachEntry( p->vMapAnds, pMatch, i )
    {
        // get the leaves of the cut
        vLeaves = Vec_VecEntry( p->vMapCuts, i );
        // convert the leaf nodes
        Vec_PtrForEachEntry( vLeaves, pLeaf, k )
        {
            SeqEdge = (unsigned)pLeaf;
            pLeaf = Abc_NtkObj( pNtk, SeqEdge >> 8 );

//            Lag = (SeqEdge & 255) + Seq_NodeGetLag(pMatch->pAnd) - Seq_NodeGetLag(pLeaf);
            Lag = (SeqEdge & 255) + 
                (pMatch->fCompl?                     Seq_NodeGetLagN(pMatch->pAnd) : Seq_NodeGetLag(pMatch->pAnd)) - 
                (((pMatch->uPhaseR & (1 << k)) > 0)? Seq_NodeGetLagN(pLeaf)        : Seq_NodeGetLag(pLeaf)       );

            assert( Lag >= 0 );

            // translate the old leaf into the leaf in the new network
//            if ( pMatch->uPhase & (1 << k) ) // negative phase is required
//                pFaninNew = pLeaf->pNext? pLeaf->pNext : pLeaf->pCopy;
//            else // positive phase is required
//                pFaninNew = pLeaf->pCopy? pLeaf->pCopy : pLeaf->pNext;

            // translate the old leaf into the leaf in the new network
            if ( pMatch->uPhaseR & (1 << k) ) // negative phase is required
                pFaninNew = pLeaf->pNext;
            else // positive phase is required
                pFaninNew = pLeaf->pCopy;

            Vec_PtrWriteEntry( vLeaves, k, (void *)((pFaninNew->Id << 8) | Lag) );
//            printf( "%d -> %d\n", pLeaf->Id, pLeaf->pCopy->Id );

            // UPDATE PHASE!!!   leaving only those bits that require inverters
        }
        // convert the root node
//        Vec_PtrWriteEntry( p->vMapAnds, i, pObj->pCopy );
        pMatch->pAnd = pMatch->fCompl? pMatch->pAnd->pNext : pMatch->pAnd->pCopy;
    }
    pNew = pNtkNew->pManFunc;
    pNew->nVarsMax    = p->nVarsMax;
    pNew->vMapAnds    = p->vMapAnds;     p->vMapAnds    = NULL;
    pNew->vMapCuts    = p->vMapCuts;     p->vMapCuts    = NULL;
    pNew->fStandCells = p->fStandCells;  p->fStandCells = 0;

    if ( !Abc_NtkCheck( pNtkNew ) )
        fprintf( stdout, "Seq_NtkMapDup(): Network check has failed.\n" );
    return pNtkNew;
}

/**Function*************************************************************

  Synopsis    [Checks if the initial states are compatible.]

  Description [Checks of all the initial states on the fanins edges
  of the cut have compatible number of latches and initial states.
  If this is not true, then the mapped network with the does not have initial 
  state. Returns the number of LUTs with incompatible edges.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Seq_NtkMapInitCompatible( Abc_Ntk_t * pNtk, int fVerbose )
{
    Abc_Seq_t * p = pNtk->pManFunc;
    Seq_Match_t * pMatch;
    Abc_Obj_t * pAnd, * pLeaf, * pFanout0, * pFanout1;
    Vec_Vec_t * vTotalEdges;
    Vec_Ptr_t * vLeaves, * vEdges;
    int i, k, m, Edge0, Edge1, nLatchAfter, nLatches1, nLatches2;
    unsigned SeqEdge;
    int CountBad = 0, CountAll = 0;

    vTotalEdges = Vec_VecStart( p->nVarsMax );
    // go through all the nodes (cuts) used in the mapping
    Vec_PtrForEachEntry( p->vMapAnds, pMatch, i )
    {
        pAnd = pMatch->pAnd;
//        printf( "*** Node %d.\n", pAnd->Id );

        // get the cut of this gate
        vLeaves = Vec_VecEntry( p->vMapCuts, i );

        // get the edges pointing to the leaves
        Vec_VecClear( vTotalEdges );
        Seq_MapMappingEdges_rec( pNtk, pAnd->Id << 8, NULL, vLeaves, vTotalEdges );

        // for each leaf, consider its edges
        Vec_PtrForEachEntry( vLeaves, pLeaf, k )
        {
            SeqEdge = (unsigned)pLeaf;
            pLeaf = Abc_NtkObj( pNtk, SeqEdge >> 8 );
            nLatchAfter = SeqEdge & 255;
            if ( nLatchAfter == 0 )
                continue;

            // go through the edges
            vEdges = Vec_VecEntry( vTotalEdges, k );
            pFanout0 = NULL;
            Vec_PtrForEachEntry( vEdges, pFanout1, m )
            {
                Edge1    = Abc_ObjIsComplement(pFanout1);
                pFanout1 = Abc_ObjRegular(pFanout1);
//printf( "Fanin = %d. Fanout = %d.\n", pLeaf->Id, pFanout1->Id );

                // make sure this is the same fanin
                if ( Edge1 )
                    assert( pLeaf == Abc_ObjFanin1(pFanout1) );
                else
                    assert( pLeaf == Abc_ObjFanin0(pFanout1) );

                // save the first one
                if ( pFanout0 == NULL )
                {
                    pFanout0 = pFanout1;
                    Edge0    = Edge1;
                    continue;
                }
                // compare the rings
                // if they have different number of latches, this is the bug
                nLatches1 = Seq_NodeCountLats(pFanout0, Edge0);
                nLatches2 = Seq_NodeCountLats(pFanout1, Edge1);
                assert( nLatches1 == nLatches2 );
                assert( nLatches1 == nLatchAfter );
                assert( nLatches1 > 0 );

                // if they have different initial states, this is the problem
                if ( !Seq_NodeCompareLats(pFanout0, Edge0, pFanout1, Edge1) )
                {
                    CountBad++;
                    break;
                }
                CountAll++;
            }
        }
    }
    if ( fVerbose )
        printf( "The number of pairs of edges checked = %d.\n", CountAll );
    Vec_VecFree( vTotalEdges );
    return CountBad;
}

/**Function*************************************************************

  Synopsis    [Derives the final mapped network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Seq_NtkSeqMapMapped( Abc_Ntk_t * pNtk )
{
    Abc_Seq_t * p = pNtk->pManFunc;
    Seq_Match_t * pMatch;
    Abc_Ntk_t * pNtkMap; 
    Vec_Ptr_t * vLeaves;
    Abc_Obj_t * pObj, * pFaninNew;
    Seq_Lat_t * pRing;
    int i;

    assert( Abc_NtkIsSeq(pNtk) );

    // start the network
    pNtkMap = Abc_NtkStartFrom( pNtk, ABC_NTK_LOGIC, ABC_FUNC_BDD );

    // duplicate the nodes used in the mapping
    Vec_PtrForEachEntry( p->vMapAnds, pMatch, i )
        pMatch->pAnd->pCopy = Abc_NtkCreateNode( pNtkMap );

    // create and share the latches
    Seq_NtkShareLatchesMapping( pNtkMap, pNtk, p->vMapAnds, 0 );

    // connect the nodes
    Vec_PtrForEachEntry( p->vMapAnds, pMatch, i )
    {
        pObj = pMatch->pAnd;
        // get the leaves of this gate
        vLeaves = Vec_VecEntry( p->vMapCuts, i );
        // get the BDD of the node
        pObj->pCopy->pData = Seq_MapMappingConnectBdd_rec( pNtk, pObj->Id << 8, NULL, -1, pObj, vLeaves );
        Cudd_Ref( pObj->pCopy->pData );
        // complement the BDD of the cut if it came from the opposite polarity choice cut
//        if ( Vec_StrEntry(p->vPhase, i) )
//            pObj->pCopy->pData = Cudd_Not( pObj->pCopy->pData );
    }

    // set the POs
    Abc_NtkForEachPo( pNtk, pObj, i )
    {
        if ( pRing = Seq_NodeGetRing(pObj,0) )
            pFaninNew = pRing->pLatch;
        else
            pFaninNew = Abc_ObjFanin0(pObj)->pCopy;
        pFaninNew = Abc_ObjNotCond( pFaninNew, Abc_ObjFaninC0(pObj) );
        Abc_ObjAddFanin( pObj->pCopy, pFaninNew );
    }

    // add the latches and their names
    Abc_NtkAddDummyBoxNames( pNtkMap );
    Abc_NtkOrderCisCos( pNtkMap );
    // fix the problem with complemented and duplicated CO edges
    Abc_NtkLogicMakeSimpleCos( pNtkMap, 1 );
    // make the network minimum base
    Abc_NtkMinimumBase( pNtkMap );
    if ( !Abc_NtkCheck( pNtkMap ) )
        fprintf( stdout, "Seq_NtkSeqFpgaMapped(): Network check has failed.\n" );
    return pNtkMap;
}



/**Function*************************************************************

  Synopsis    [Counts the number of nodes in the bag.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Seq_MapMappingCount( Abc_Ntk_t * pNtk )
{
    Abc_Seq_t * p = pNtk->pManFunc;
    Vec_Ptr_t * vLeaves;
    Seq_Match_t * pMatch;
    int i, Counter = 0;
    Vec_PtrForEachEntry( p->vMapAnds, pMatch, i )
    {
        vLeaves = Vec_VecEntry( p->vMapCuts, i );
        Counter += Seq_MapMappingCount_rec( pNtk, pMatch->pAnd->Id << 8, vLeaves );
    }
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Counts the number of nodes in the bag.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Seq_MapMappingCount_rec( Abc_Ntk_t * pNtk, unsigned SeqEdge, Vec_Ptr_t * vLeaves )
{
    Abc_Obj_t * pObj, * pLeaf;
    unsigned SeqEdge0, SeqEdge1;
    int Lag, i;
    // get the object and the lag
    pObj = Abc_NtkObj( pNtk, SeqEdge >> 8 );
    Lag  = SeqEdge & 255;
    // if the node is the fanin of the cut, return
    Vec_PtrForEachEntry( vLeaves, pLeaf, i )
        if ( SeqEdge == (unsigned)pLeaf )
            return 0;
    // continue unfolding
    assert( Abc_AigNodeIsAnd(pObj) );
    // get new sequential edges
    assert( Lag + Seq_ObjFaninL0(pObj) < 255 );
    assert( Lag + Seq_ObjFaninL1(pObj) < 255 );
    SeqEdge0 = (Abc_ObjFanin0(pObj)->Id << 8) + Lag + Seq_ObjFaninL0(pObj);
    SeqEdge1 = (Abc_ObjFanin1(pObj)->Id << 8) + Lag + Seq_ObjFaninL1(pObj);
    // call for the children
    return 1 + Seq_MapMappingCount_rec( pNtk, SeqEdge0, vLeaves ) + 
        Seq_MapMappingCount_rec( pNtk, SeqEdge1, vLeaves );
}

/**Function*************************************************************

  Synopsis    [Collects the edges pointing to the leaves of the cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Obj_t * Seq_MapMappingBuild_rec( Abc_Ntk_t * pNtkNew, Abc_Ntk_t * pNtk, unsigned SeqEdge, int fTop, int fCompl, int LagCut, Vec_Ptr_t * vLeaves, unsigned uPhase )
{
    Abc_Obj_t * pObj, * pObjNew, * pLeaf, * pFaninNew0, * pFaninNew1;
    unsigned SeqEdge0, SeqEdge1;
    int Lag, i;
    // get the object and the lag
    pObj = Abc_NtkObj( pNtk, SeqEdge >> 8 );
    Lag  = SeqEdge & 255;
    // if the node is the fanin of the cut, return
    Vec_PtrForEachEntry( vLeaves, pLeaf, i )
        if ( SeqEdge == (unsigned)pLeaf )
        {
//            if ( uPhase & (1 << i) ) // negative phase is required
//                return pObj->pNext? pObj->pNext : pObj->pCopy;
//            else // positive phase is required
//                return pObj->pCopy? pObj->pCopy : pObj->pNext;

            if ( uPhase & (1 << i) ) // negative phase is required
                return pObj->pNext;
            else // positive phase is required
                return pObj->pCopy;
        }
    // continue unfolding
    assert( Abc_AigNodeIsAnd(pObj) );
    // get new sequential edges
    assert( Lag + Seq_ObjFaninL0(pObj) < 255 );
    assert( Lag + Seq_ObjFaninL1(pObj) < 255 );
    SeqEdge0 = (Abc_ObjFanin0(pObj)->Id << 8) + Lag + Seq_ObjFaninL0(pObj);
    SeqEdge1 = (Abc_ObjFanin1(pObj)->Id << 8) + Lag + Seq_ObjFaninL1(pObj);
    // call for the children    
    pObjNew = fTop? (fCompl? pObj->pNext : pObj->pCopy) : Abc_NtkCreateNode( pNtkNew );
    // solve subproblems
    pFaninNew0 = Seq_MapMappingBuild_rec( pNtkNew, pNtk, SeqEdge0, 0, fCompl, LagCut, vLeaves, uPhase );
    pFaninNew1 = Seq_MapMappingBuild_rec( pNtkNew, pNtk, SeqEdge1, 0, fCompl, LagCut, vLeaves, uPhase );
    // add the fanins to the node
    Abc_ObjAddFanin( pObjNew, Abc_ObjNotCond( pFaninNew0, Abc_ObjFaninC0(pObj) ) );
    Abc_ObjAddFanin( pObjNew, Abc_ObjNotCond( pFaninNew1, Abc_ObjFaninC1(pObj) ) );
    Seq_NodeDupLats( pObjNew, pObj, 0 );
    Seq_NodeDupLats( pObjNew, pObj, 1 );
    // set the lag of the new node equal to the internal lag plus mapping/retiming lag
    Seq_NodeSetLag( pObjNew, (char)(Lag + LagCut) );
//    Seq_NodeSetLag( pObjNew, (char)(Lag) );
    return pObjNew;
}


/**Function*************************************************************

  Synopsis    [Collects the edges pointing to the leaves of the cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Seq_MapMappingEdges_rec( Abc_Ntk_t * pNtk, unsigned SeqEdge, Abc_Obj_t * pPrev, Vec_Ptr_t * vLeaves, Vec_Vec_t * vMapEdges )
{
    Abc_Obj_t * pObj, * pLeaf;
    unsigned SeqEdge0, SeqEdge1;
    int Lag, i;
    // get the object and the lag
    pObj = Abc_NtkObj( pNtk, SeqEdge >> 8 );
    Lag  = SeqEdge & 255;
    // if the node is the fanin of the cut, return
    Vec_PtrForEachEntry( vLeaves, pLeaf, i )
    {
        if ( SeqEdge == (unsigned)pLeaf )
        {
            assert( pPrev != NULL );
            Vec_VecPush( vMapEdges, i, pPrev );
            return;
        }
    }
    // continue unfolding
    assert( Abc_AigNodeIsAnd(pObj) );
    // get new sequential edges
    assert( Lag + Seq_ObjFaninL0(pObj) < 255 );
    assert( Lag + Seq_ObjFaninL1(pObj) < 255 );
    SeqEdge0 = (Abc_ObjFanin0(pObj)->Id << 8) + Lag + Seq_ObjFaninL0(pObj);
    SeqEdge1 = (Abc_ObjFanin1(pObj)->Id << 8) + Lag + Seq_ObjFaninL1(pObj);
    // call for the children    
    Seq_MapMappingEdges_rec( pNtk, SeqEdge0, pObj            , vLeaves, vMapEdges );
    Seq_MapMappingEdges_rec( pNtk, SeqEdge1, Abc_ObjNot(pObj), vLeaves, vMapEdges );
}

/**Function*************************************************************

  Synopsis    [Collects the edges pointing to the leaves of the cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Seq_MapMappingConnectBdd_rec( Abc_Ntk_t * pNtk, unsigned SeqEdge, Abc_Obj_t * pPrev, int Edge, Abc_Obj_t * pRoot, Vec_Ptr_t * vLeaves )
{
    Seq_Lat_t * pRing;
    Abc_Obj_t * pObj, * pLeaf, * pFanin, * pFaninNew;
    unsigned SeqEdge0, SeqEdge1;
    DdManager * dd = pRoot->pCopy->pNtk->pManFunc;
    DdNode * bFunc, * bFunc0, * bFunc1;
    int Lag, i, k;
    // get the object and the lag
    pObj = Abc_NtkObj( pNtk, SeqEdge >> 8 );
    Lag  = SeqEdge & 255;
    // if the node is the fanin of the cut, add the connection and return
    Vec_PtrForEachEntry( vLeaves, pLeaf, i )
    {
        if ( SeqEdge == (unsigned)pLeaf )
        {
            assert( pPrev != NULL );
            if ( pRing = Seq_NodeGetRing(pPrev,Edge) )
                pFaninNew = pRing->pLatch;
            else
                pFaninNew = Abc_ObjFanin(pPrev,Edge)->pCopy;

            // check if the root already has this fanin
            Abc_ObjForEachFanin( pRoot->pCopy, pFanin, k )
                if ( pFanin == pFaninNew )
                    return Cudd_bddIthVar( dd, k );
            Abc_ObjAddFanin( pRoot->pCopy, pFaninNew );
            return Cudd_bddIthVar( dd, k );
        }
    }
    // continue unfolding
    assert( Abc_AigNodeIsAnd(pObj) );
    // get new sequential edges
    assert( Lag + Seq_ObjFaninL0(pObj) < 255 );
    assert( Lag + Seq_ObjFaninL1(pObj) < 255 );
    SeqEdge0 = (Abc_ObjFanin0(pObj)->Id << 8) + Lag + Seq_ObjFaninL0(pObj);
    SeqEdge1 = (Abc_ObjFanin1(pObj)->Id << 8) + Lag + Seq_ObjFaninL1(pObj);
    // call for the children    
    bFunc0 = Seq_MapMappingConnectBdd_rec( pNtk, SeqEdge0, pObj, 0, pRoot, vLeaves );    Cudd_Ref( bFunc0 );
    bFunc1 = Seq_MapMappingConnectBdd_rec( pNtk, SeqEdge1, pObj, 1, pRoot, vLeaves );    Cudd_Ref( bFunc1 );
    bFunc0 = Cudd_NotCond( bFunc0, Abc_ObjFaninC0(pObj) );
    bFunc1 = Cudd_NotCond( bFunc1, Abc_ObjFaninC1(pObj) );
    // get the BDD of the node
    bFunc  = Cudd_bddAnd( dd, bFunc0, bFunc1 );  Cudd_Ref( bFunc );
    Cudd_RecursiveDeref( dd, bFunc0 );
    Cudd_RecursiveDeref( dd, bFunc1 );
    // return the BDD
    Cudd_Deref( bFunc );
    return bFunc;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


