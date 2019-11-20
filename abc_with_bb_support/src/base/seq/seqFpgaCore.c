/**CFile****************************************************************

  FileName    [seqFpgaCore.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Construction and manipulation of sequential AIGs.]

  Synopsis    [The core of FPGA mapping/retiming package.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: seqFpgaCore.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "seqInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static Abc_Ntk_t *  Seq_NtkFpgaDup( Abc_Ntk_t * pNtk );
static int          Seq_NtkFpgaInitCompatible( Abc_Ntk_t * pNtk, int fVerbose );
static Abc_Ntk_t *  Seq_NtkSeqFpgaMapped( Abc_Ntk_t * pNtkNew );
static int          Seq_FpgaMappingCount( Abc_Ntk_t * pNtk );
static int          Seq_FpgaMappingCount_rec( Abc_Ntk_t * pNtk, unsigned SeqEdge, Vec_Ptr_t * vLeaves );
static Abc_Obj_t *  Seq_FpgaMappingBuild_rec( Abc_Ntk_t * pNtkNew, Abc_Ntk_t * pNtk, unsigned SeqEdge, int fTop, int LagCut, Vec_Ptr_t * vLeaves );
static DdNode *     Seq_FpgaMappingBdd_rec( DdManager * dd, Abc_Ntk_t * pNtk, unsigned SeqEdge, Vec_Ptr_t * vLeaves );
static void         Seq_FpgaMappingEdges_rec( Abc_Ntk_t * pNtk, unsigned SeqEdge, Abc_Obj_t * pPrev, Vec_Ptr_t * vLeaves, Vec_Vec_t * vMapEdges );
static void         Seq_FpgaMappingConnect_rec( Abc_Ntk_t * pNtk, unsigned SeqEdge, Abc_Obj_t * pPrev, int Edge, Abc_Obj_t * pRoot, Vec_Ptr_t * vLeaves );
static DdNode *     Seq_FpgaMappingConnectBdd_rec( Abc_Ntk_t * pNtk, unsigned SeqEdge, Abc_Obj_t * pPrev, int Edge, Abc_Obj_t * pRoot, Vec_Ptr_t * vLeaves );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Performs FPGA mapping and retiming.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Seq_NtkFpgaMapRetime( Abc_Ntk_t * pNtk, int nMaxIters, int fVerbose )
{
    Abc_Seq_t * p = pNtk->pManFunc;
    Abc_Ntk_t * pNtkNew;
    Abc_Ntk_t * pNtkMap;
    int RetValue;

    // get the LUT library
    p->nVarsMax = Fpga_LutLibReadVarMax( Abc_FrameReadLibLut() );
    p->nMaxIters = nMaxIters;

    // find the best mapping and retiming for all nodes (p->vLValues, p->vBestCuts, p->vLags)
    if ( !Seq_FpgaMappingDelays( pNtk, fVerbose ) )
        return NULL;
    if ( RetValue = Abc_NtkGetChoiceNum(pNtk) )
    {
        printf( "The network has %d choices. The resulting network is not derived (this is temporary).\n", RetValue );
        printf( "The mininum clock period computed is %d.\n", p->FiBestInt );
        return NULL;
    }

    // duplicate the nodes contained in multiple cuts
    pNtkNew = Seq_NtkFpgaDup( pNtk );
//    return pNtkNew;
    
    // implement the retiming
    RetValue = Seq_NtkImplementRetiming( pNtkNew, ((Abc_Seq_t *)pNtkNew->pManFunc)->vLags, fVerbose );
    if ( RetValue == 0 )
        printf( "Retiming completed but initial state computation has failed.\n" );
//    return pNtkNew;

    // check the compatibility of initial states computed
    if ( RetValue = Seq_NtkFpgaInitCompatible( pNtkNew, fVerbose ) )
        printf( "The number of LUTs with incompatible edges = %d.\n", RetValue );

    // create the final mapped network
    pNtkMap = Seq_NtkSeqFpgaMapped( pNtkNew );
    Abc_NtkDelete( pNtkNew );
    if ( RetValue )
        printf( "The number of LUTs with more than %d inputs = %d.\n", 
            p->nVarsMax, Seq_NtkCountNodesAboveLimit(pNtkMap, p->nVarsMax) );
    return pNtkMap;
}

/**Function*************************************************************

  Synopsis    [Derives the network by duplicating some of the nodes.]

  Description [Information about mapping is given as mapping nodes (p->vMapAnds)
  and best cuts for each node (p->vMapCuts).]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Seq_NtkFpgaDup( Abc_Ntk_t * pNtk )
{
    Abc_Seq_t * pNew, * p = pNtk->pManFunc;
    Abc_Ntk_t * pNtkNew; 
    Abc_Obj_t * pObj, * pLeaf;
    Vec_Ptr_t * vLeaves;
    unsigned SeqEdge;
    int i, k, nObjsNew, Lag;
    
    assert( Abc_NtkIsSeq(pNtk) );

    // start the expanded network
    pNtkNew = Abc_NtkStartFrom( pNtk, pNtk->ntkType, pNtk->ntkFunc );

    // start the new sequential AIG manager
    nObjsNew = 1 + Abc_NtkPiNum(pNtk) + Abc_NtkPoNum(pNtk) + Seq_FpgaMappingCount(pNtk);
    Seq_Resize( pNtkNew->pManFunc, nObjsNew );

    // duplicate the nodes in the mapping
    Vec_PtrForEachEntry( p->vMapAnds, pObj, i )
        Abc_NtkDupObj( pNtkNew, pObj, 0 );

    // recursively construct the internals of each node
    Vec_PtrForEachEntry( p->vMapAnds, pObj, i )
    {
        vLeaves = Vec_VecEntry( p->vMapCuts, i );
        Seq_FpgaMappingBuild_rec( pNtkNew, pNtk, pObj->Id << 8, 1, Seq_NodeGetLag(pObj), vLeaves );
    }
    assert( nObjsNew == pNtkNew->nObjs );

    // set the POs
    Abc_NtkFinalize( pNtk, pNtkNew );
    // duplicate the latches on the PO edges
    Abc_NtkForEachPo( pNtk, pObj, i )
        Seq_NodeDupLats( pObj->pCopy, pObj, 0 );

    // transfer the mapping info to the new manager
    Vec_PtrForEachEntry( p->vMapAnds, pObj, i )
    {
        // get the leaves of the cut
        vLeaves = Vec_VecEntry( p->vMapCuts, i );
        // convert the leaf nodes
        Vec_PtrForEachEntry( vLeaves, pLeaf, k )
        {
            SeqEdge = (unsigned)pLeaf;
            pLeaf = Abc_NtkObj( pNtk, SeqEdge >> 8 );
            Lag = (SeqEdge & 255) + Seq_NodeGetLag(pObj) - Seq_NodeGetLag(pLeaf);
            assert( Lag >= 0 );
            // translate the old leaf into the leaf in the new network
            Vec_PtrWriteEntry( vLeaves, k, (void *)((pLeaf->pCopy->Id << 8) | Lag) );
//            printf( "%d -> %d\n", pLeaf->Id, pLeaf->pCopy->Id );
        }
        // convert the root node
        Vec_PtrWriteEntry( p->vMapAnds, i, pObj->pCopy );
    }
    pNew = pNtkNew->pManFunc;
    pNew->nVarsMax  = p->nVarsMax;
    pNew->vMapAnds  = p->vMapAnds;   p->vMapAnds  = NULL;
    pNew->vMapCuts  = p->vMapCuts;   p->vMapCuts  = NULL;

    if ( !Abc_NtkCheck( pNtkNew ) )
        fprintf( stdout, "Seq_NtkFpgaDup(): Network check has failed.\n" );
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
int Seq_NtkFpgaInitCompatible( Abc_Ntk_t * pNtk, int fVerbose )
{
    Abc_Seq_t * p = pNtk->pManFunc;
    Abc_Obj_t * pAnd, * pLeaf, * pFanout0, * pFanout1;
    Vec_Vec_t * vTotalEdges;
    Vec_Ptr_t * vLeaves, * vEdges;
    int i, k, m, Edge0, Edge1, nLatchAfter, nLatches1, nLatches2;
    unsigned SeqEdge;
    int CountBad = 0, CountAll = 0;

    vTotalEdges = Vec_VecStart( p->nVarsMax );
    // go through all the nodes (cuts) used in the mapping
    Vec_PtrForEachEntry( p->vMapAnds, pAnd, i )
    {
//        printf( "*** Node %d.\n", pAnd->Id );

        // get the cut of this gate
        vLeaves = Vec_VecEntry( p->vMapCuts, i );

        // get the edges pointing to the leaves
        Vec_VecClear( vTotalEdges );
        Seq_FpgaMappingEdges_rec( pNtk, pAnd->Id << 8, NULL, vLeaves, vTotalEdges );

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
Abc_Ntk_t * Seq_NtkSeqFpgaMapped( Abc_Ntk_t * pNtk )
{
    Abc_Seq_t * p = pNtk->pManFunc;
    Abc_Ntk_t * pNtkMap; 
    Vec_Ptr_t * vLeaves;
    Abc_Obj_t * pObj, * pFaninNew;
    Seq_Lat_t * pRing;
    int i;

    assert( Abc_NtkIsSeq(pNtk) );

    // start the network
    pNtkMap = Abc_NtkStartFrom( pNtk, ABC_NTK_LOGIC, ABC_FUNC_BDD );

    // duplicate the nodes used in the mapping
    Vec_PtrForEachEntry( p->vMapAnds, pObj, i )
        pObj->pCopy = Abc_NtkCreateNode( pNtkMap );

    // create and share the latches
    Seq_NtkShareLatchesMapping( pNtkMap, pNtk, p->vMapAnds, 1 );

    // connect the nodes
    Vec_PtrForEachEntry( p->vMapAnds, pObj, i )
    {
        // get the leaves of this gate
        vLeaves = Vec_VecEntry( p->vMapCuts, i );
        // get the BDD of the node
        pObj->pCopy->pData = Seq_FpgaMappingConnectBdd_rec( pNtk, pObj->Id << 8, NULL, -1, pObj, vLeaves );
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
int Seq_FpgaMappingCount( Abc_Ntk_t * pNtk )
{
    Abc_Seq_t * p = pNtk->pManFunc;
    Vec_Ptr_t * vLeaves;
    Abc_Obj_t * pAnd;
    int i, Counter = 0;
    Vec_PtrForEachEntry( p->vMapAnds, pAnd, i )
    {
        vLeaves = Vec_VecEntry( p->vMapCuts, i );
        Counter += Seq_FpgaMappingCount_rec( pNtk, pAnd->Id << 8, vLeaves );
    }
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Counts the number of nodes in the bag.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Seq_FpgaMappingCount_rec( Abc_Ntk_t * pNtk, unsigned SeqEdge, Vec_Ptr_t * vLeaves )
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
    return 1 + Seq_FpgaMappingCount_rec( pNtk, SeqEdge0, vLeaves ) + 
        Seq_FpgaMappingCount_rec( pNtk, SeqEdge1, vLeaves );
}

/**Function*************************************************************

  Synopsis    [Collects the edges pointing to the leaves of the cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Obj_t * Seq_FpgaMappingBuild_rec( Abc_Ntk_t * pNtkNew, Abc_Ntk_t * pNtk, unsigned SeqEdge, int fTop, int LagCut, Vec_Ptr_t * vLeaves )
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
            return pObj->pCopy;
    // continue unfolding
    assert( Abc_AigNodeIsAnd(pObj) );
    // get new sequential edges
    assert( Lag + Seq_ObjFaninL0(pObj) < 255 );
    assert( Lag + Seq_ObjFaninL1(pObj) < 255 );
    SeqEdge0 = (Abc_ObjFanin0(pObj)->Id << 8) + Lag + Seq_ObjFaninL0(pObj);
    SeqEdge1 = (Abc_ObjFanin1(pObj)->Id << 8) + Lag + Seq_ObjFaninL1(pObj);
    // call for the children    
    pObjNew = fTop? pObj->pCopy : Abc_NtkCreateNode( pNtkNew );
    // solve subproblems
    pFaninNew0 = Seq_FpgaMappingBuild_rec( pNtkNew, pNtk, SeqEdge0, 0, LagCut, vLeaves );
    pFaninNew1 = Seq_FpgaMappingBuild_rec( pNtkNew, pNtk, SeqEdge1, 0, LagCut, vLeaves );
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

  Synopsis    [Derives the BDD of the selected cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Seq_FpgaMappingBdd_rec( DdManager * dd, Abc_Ntk_t * pNtk, unsigned SeqEdge, Vec_Ptr_t * vLeaves )
{
    Abc_Obj_t * pObj, * pLeaf;
    DdNode * bFunc0, * bFunc1, * bFunc;
    unsigned SeqEdge0, SeqEdge1;
    int Lag, i;
    // get the object and the lag
    pObj = Abc_NtkObj( pNtk, SeqEdge >> 8 );
    Lag  = SeqEdge & 255;
    // if the node is the fanin of the cut, return
    Vec_PtrForEachEntry( vLeaves, pLeaf, i )
        if ( SeqEdge == (unsigned)pLeaf )
            return Cudd_bddIthVar( dd, i );
    // continue unfolding
    assert( Abc_AigNodeIsAnd(pObj) );
    // get new sequential edges
    assert( Lag + Seq_ObjFaninL0(pObj) < 255 );
    assert( Lag + Seq_ObjFaninL1(pObj) < 255 );
    SeqEdge0 = (Abc_ObjFanin0(pObj)->Id << 8) + Lag + Seq_ObjFaninL0(pObj);
    SeqEdge1 = (Abc_ObjFanin1(pObj)->Id << 8) + Lag + Seq_ObjFaninL1(pObj);
    // call for the children
    bFunc0 = Seq_FpgaMappingBdd_rec( dd, pNtk, SeqEdge0, vLeaves );    Cudd_Ref( bFunc0 );
    bFunc1 = Seq_FpgaMappingBdd_rec( dd, pNtk, SeqEdge1, vLeaves );    Cudd_Ref( bFunc1 );
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

/**Function*************************************************************

  Synopsis    [Collects the edges pointing to the leaves of the cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Seq_FpgaMappingEdges_rec( Abc_Ntk_t * pNtk, unsigned SeqEdge, Abc_Obj_t * pPrev, Vec_Ptr_t * vLeaves, Vec_Vec_t * vMapEdges )
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
    Seq_FpgaMappingEdges_rec( pNtk, SeqEdge0, pObj            , vLeaves, vMapEdges );
    Seq_FpgaMappingEdges_rec( pNtk, SeqEdge1, Abc_ObjNot(pObj), vLeaves, vMapEdges );
}

/**Function*************************************************************

  Synopsis    [Collects the edges pointing to the leaves of the cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Seq_FpgaMappingConnect_rec( Abc_Ntk_t * pNtk, unsigned SeqEdge, Abc_Obj_t * pPrev, int Edge, Abc_Obj_t * pRoot, Vec_Ptr_t * vLeaves )
{
    Seq_Lat_t * pRing;
    Abc_Obj_t * pObj, * pLeaf, * pFanin, * pFaninNew;
    unsigned SeqEdge0, SeqEdge1;
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
            Abc_ObjForEachFanin( pRoot, pFanin, k )
                if ( pFanin == pFaninNew )
                    return;
            Abc_ObjAddFanin( pRoot->pCopy, pFaninNew );
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
    Seq_FpgaMappingConnect_rec( pNtk, SeqEdge0, pObj, 0, pRoot, vLeaves );
    Seq_FpgaMappingConnect_rec( pNtk, SeqEdge1, pObj, 1, pRoot, vLeaves );
}

/**Function*************************************************************

  Synopsis    [Collects the edges pointing to the leaves of the cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Seq_FpgaMappingConnectBdd_rec( Abc_Ntk_t * pNtk, unsigned SeqEdge, Abc_Obj_t * pPrev, int Edge, Abc_Obj_t * pRoot, Vec_Ptr_t * vLeaves )
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
    bFunc0 = Seq_FpgaMappingConnectBdd_rec( pNtk, SeqEdge0, pObj, 0, pRoot, vLeaves );    Cudd_Ref( bFunc0 );
    bFunc1 = Seq_FpgaMappingConnectBdd_rec( pNtk, SeqEdge1, pObj, 1, pRoot, vLeaves );    Cudd_Ref( bFunc1 );
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


