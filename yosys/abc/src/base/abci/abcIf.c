/**CFile****************************************************************

  FileName    [abcIf.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Network and node package.]

  Synopsis    [Interface with the FPGA mapping package.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - November 21, 2006.]

  Revision    [$Id: abcIf.c,v 1.00 2006/11/21 00:00:00 alanmi Exp $]

***********************************************************************/

#include "base/abc/abc.h"
#include "base/main/main.h"
#include "map/if/if.h"
#include "bool/kit/kit.h"
#include "aig/aig/aig.h"
#include "map/mio/mio.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

extern If_Man_t *  Abc_NtkToIf( Abc_Ntk_t * pNtk, If_Par_t * pPars );
static Abc_Ntk_t * Abc_NtkFromIf( If_Man_t * pIfMan, Abc_Ntk_t * pNtk );
extern Abc_Obj_t * Abc_NodeFromIf_rec( Abc_Ntk_t * pNtkNew, If_Man_t * pIfMan, If_Obj_t * pIfObj, Vec_Int_t * vCover );
static Hop_Obj_t * Abc_NodeIfToHop( Hop_Man_t * pHopMan, If_Man_t * pIfMan, If_Obj_t * pIfObj );
static Vec_Ptr_t * Abc_NtkFindGoodOrder( Abc_Ntk_t * pNtk );

extern void Abc_NtkBddReorder( Abc_Ntk_t * pNtk, int fVerbose );
extern void Abc_NtkBidecResyn( Abc_Ntk_t * pNtk, int fVerbose );
 
////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Interface with the FPGA mapping package.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void If_ManComputeSwitching( If_Man_t * pIfMan )
{
    abctime clk = Abc_Clock();
    Gia_Man_t * pNew;
    Vec_Int_t * vCopy;
    If_Obj_t * pIfObj;
    int i;
    assert( pIfMan->vSwitching == NULL );
    // create the new manager
    pNew = Gia_ManStart( If_ManObjNum(pIfMan) );
    vCopy = Vec_IntAlloc( If_ManObjNum(pIfMan) );
    // constant and inputs
    Vec_IntPush( vCopy, 1 );
    If_ManForEachCi( pIfMan, pIfObj, i )
        Vec_IntPush( vCopy, Gia_ManAppendCi(pNew) );
    // internal nodes
    If_ManForEachNode( pIfMan, pIfObj, i )
    {
        int iLit0 = Abc_LitNotCond( Vec_IntEntry(vCopy, If_ObjFanin0(pIfObj)->Id), If_ObjFaninC0(pIfObj) );
        int iLit1 = Abc_LitNotCond( Vec_IntEntry(vCopy, If_ObjFanin1(pIfObj)->Id), If_ObjFaninC1(pIfObj) );
        Vec_IntPush( vCopy, Gia_ManAppendAnd(pNew, iLit0, iLit1) );
    }
    // outputs
    If_ManForEachCo( pIfMan, pIfObj, i )
    {
        int iLit0 = Abc_LitNotCond( Vec_IntEntry(vCopy, If_ObjFanin0(pIfObj)->Id), If_ObjFaninC0(pIfObj) );
        Vec_IntPush( vCopy, Gia_ManAppendCo(pNew, iLit0) );
    }
    assert( Vec_IntSize(vCopy) == If_ManObjNum(pIfMan) );
    Vec_IntFree( vCopy );
    // compute switching activity
    pIfMan->vSwitching = Gia_ManComputeSwitchProbs( pNew, 48, 16, 0 );
    Gia_ManStop( pNew );
    if ( pIfMan->pPars->fVerbose )
        Abc_PrintTime( 1, "Computing switching activity", Abc_Clock() - clk );
}

/**Function*************************************************************

  Synopsis    [Interface with the FPGA mapping package.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkIf( Abc_Ntk_t * pNtk, If_Par_t * pPars )
{
    Abc_Ntk_t * pNtkNew, * pTemp;
    If_Man_t * pIfMan;

    assert( Abc_NtkIsStrash(pNtk) );

    // get timing information
    pPars->pTimesArr = Abc_NtkGetCiArrivalFloats(pNtk);
    pPars->pTimesReq = Abc_NtkGetCoRequiredFloats(pNtk);

    // update timing info to reflect logic level
    if ( (pPars->fDelayOpt || pPars->fDsdBalance || pPars->fUserRecLib || pPars->fUserSesLib || pPars->fUserLutDec || pPars->fUserLut2D ) && pNtk->pManTime )
    {
        int c;
        if ( pNtk->AndGateDelay == 0.0 )
        {
            if ( Abc_FrameReadLibGen() )
                pNtk->AndGateDelay = Mio_LibraryReadDelayAigNode((Mio_Library_t *)Abc_FrameReadLibGen());
            if ( pNtk->AndGateDelay == 0.0 )
            {
                pNtk->AndGateDelay = 1.0;
                printf( "The AIG-node delay is not set. Assuming unit-delay.\n" );
            }
        }
        for ( c = 0; c < Abc_NtkCiNum(pNtk); c++ )
            pPars->pTimesArr[c] /= pNtk->AndGateDelay;
        for ( c = 0; c < Abc_NtkCoNum(pNtk); c++ )
            pPars->pTimesReq[c] /= pNtk->AndGateDelay;
    }

    // set the latch paths
    if ( pPars->fLatchPaths && pPars->pTimesArr )
    {
        int c;
        for ( c = 0; c < Abc_NtkPiNum(pNtk); c++ )
            pPars->pTimesArr[c] = -ABC_INFINITY;
    }

    // create FPGA mapper
    pIfMan = Abc_NtkToIf( pNtk, pPars );    
    if ( pIfMan == NULL )
        return NULL;
    if ( pPars->fPower )
        If_ManComputeSwitching( pIfMan );

    // create DSD manager
    if ( pPars->fUseDsd )
    {
        If_DsdMan_t * p = (If_DsdMan_t *)Abc_FrameReadManDsd();
        assert( pPars->nLutSize <= If_DsdManVarNum(p) );
        assert( (pPars->pLutStruct == NULL && If_DsdManLutSize(p) == 0) || (pPars->pLutStruct && pPars->pLutStruct[0] - '0' == If_DsdManLutSize(p)) );
        pIfMan->pIfDsdMan = (If_DsdMan_t *)Abc_FrameReadManDsd();
        if ( pPars->fDsdBalance )
            If_DsdManAllocIsops( pIfMan->pIfDsdMan, pPars->nLutSize );
    }

    // perform FPGA mapping
    if ( !If_ManPerformMapping( pIfMan ) )
    {
        If_ManStop( pIfMan );
        return NULL;
    }

    // transform the result of mapping into the new network
    pNtkNew = Abc_NtkFromIf( pIfMan, pNtk );
    if ( pNtkNew == NULL )
        return NULL;
    If_ManStop( pIfMan );
    if ( pPars->fDelayOpt || pPars->fDsdBalance || pPars->fUserRecLib )
    {
        pNtkNew = Abc_NtkStrash( pTemp = pNtkNew, 0, 0, 0 );
        Abc_NtkDelete( pTemp );
    }
    else if ( pPars->fBidec && pPars->nLutSize <= 8 )
        Abc_NtkBidecResyn( pNtkNew, 0 );

    // duplicate EXDC
    if ( pNtk->pExdc )
        pNtkNew->pExdc = Abc_NtkDup( pNtk->pExdc );
    // make sure that everything is okay
    if ( !Abc_NtkCheck( pNtkNew ) )
    {
        printf( "Abc_NtkIf: The network check has failed.\n" );
        Abc_NtkDelete( pNtkNew );
        return NULL;
    }
    return pNtkNew;
}

/**Function*************************************************************

  Synopsis    [Load the network into FPGA manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline If_Obj_t * Abc_ObjIfCopy( Abc_Obj_t * pNode ) { return (If_Obj_t *)pNode->pCopy; } 
If_Man_t * Abc_NtkToIf( Abc_Ntk_t * pNtk, If_Par_t * pPars )
{
    ProgressBar * pProgress;
    If_Man_t * pIfMan;
    Vec_Ptr_t * vNodes;
    Abc_Obj_t * pNode, * pPrev;
    int i;

    assert( Abc_NtkIsStrash(pNtk) );

    // start the mapping manager and set its parameters
    pIfMan = If_ManStart( pPars );
    pIfMan->pName = Abc_UtilStrsav( Abc_NtkName(pNtk) );

    // print warning about excessive memory usage
    if ( 1.0 * Abc_NtkObjNum(pNtk) * pIfMan->nObjBytes / (1<<30) > 1.0 )
        printf( "Warning: The mapper will allocate %.1f GB for to represent the subject graph with %d AIG nodes.\n", 
            1.0 * Abc_NtkObjNum(pNtk) * pIfMan->nObjBytes / (1<<30), Abc_NtkObjNum(pNtk) );

    // create PIs and remember them in the old nodes
    Abc_NtkCleanCopy( pNtk );
    Abc_AigConst1(pNtk)->pCopy = (Abc_Obj_t *)If_ManConst1( pIfMan );
    Abc_NtkForEachCi( pNtk, pNode, i )
    {
        If_Obj_t * pIfObj = If_ManCreateCi( pIfMan );
        pNode->pCopy = (Abc_Obj_t *)pIfObj;
        // transfer logic level information
        Abc_ObjIfCopy(pNode)->Level = pNode->Level;
        // mark the largest level
        if ( pIfMan->nLevelMax < (int)pIfObj->Level )
            pIfMan->nLevelMax = (int)pIfObj->Level;
    }

    // load the AIG into the mapper
    pProgress = Extra_ProgressBarStart( stdout, Abc_NtkObjNumMax(pNtk) );
    vNodes = Abc_AigDfs( pNtk, 0, 0 );
    Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pNode, i )
    {
        Extra_ProgressBarUpdate( pProgress, i, "Initial" );
        // add the node to the mapper
        pNode->pCopy = (Abc_Obj_t *)If_ManCreateAnd( pIfMan, 
            If_NotCond( Abc_ObjIfCopy(Abc_ObjFanin0(pNode)), Abc_ObjFaninC0(pNode) ), 
            If_NotCond( Abc_ObjIfCopy(Abc_ObjFanin1(pNode)), Abc_ObjFaninC1(pNode) ) );
        // set up the choice node
        if ( Abc_AigNodeIsChoice( pNode ) )
        {
            Abc_Obj_t * pEquiv;
//            int Counter = 0;
            assert( If_ObjId(Abc_ObjIfCopy(pNode)) > If_ObjId(Abc_ObjIfCopy(Abc_ObjEquiv(pNode))) );
            for ( pPrev = pNode, pEquiv = Abc_ObjEquiv(pPrev); pEquiv; pPrev = pEquiv, pEquiv = Abc_ObjEquiv(pPrev) )
                If_ObjSetChoice( Abc_ObjIfCopy(pPrev), Abc_ObjIfCopy(pEquiv) );//, Counter++;
//            printf( "%d ", Counter );
            If_ManCreateChoice( pIfMan, Abc_ObjIfCopy(pNode) );
        }
    }
    Extra_ProgressBarStop( pProgress );
    Vec_PtrFree( vNodes );

    // set the primary outputs without copying the phase
    Abc_NtkForEachCo( pNtk, pNode, i )
        pNode->pCopy = (Abc_Obj_t *)If_ManCreateCo( pIfMan, If_NotCond( Abc_ObjIfCopy(Abc_ObjFanin0(pNode)), Abc_ObjFaninC0(pNode) ) );
    return pIfMan;
}


/**Function*************************************************************

  Synopsis    [Box mapping procedures.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Abc_MapBoxSetPrevNext( Vec_Ptr_t * vDrivers, Vec_Int_t * vMapIn, Vec_Int_t * vMapOut, int Id )
{
    Abc_Obj_t * pNode;
    pNode = (Abc_Obj_t *)Vec_PtrEntry(vDrivers, Id+2);
    Vec_IntWriteEntry( vMapIn, Abc_ObjId(Abc_ObjFanin0(pNode)), Id );
    pNode = (Abc_Obj_t *)Vec_PtrEntry(vDrivers, Id+4);
    Vec_IntWriteEntry( vMapOut, Abc_ObjId(Abc_ObjFanin0(pNode)), Id );
}
static inline int Abc_MapBox2Next( Vec_Ptr_t * vDrivers, Vec_Int_t * vMapIn, Vec_Int_t * vMapOut, int Id )
{
    Abc_Obj_t * pNode = (Abc_Obj_t *)Vec_PtrEntry(vDrivers, Id+4);
    return Vec_IntEntry( vMapIn, Abc_ObjId(Abc_ObjFanin0(pNode)) );
}
static inline int Abc_MapBox2Prev( Vec_Ptr_t * vDrivers, Vec_Int_t * vMapIn, Vec_Int_t * vMapOut, int Id )
{
    Abc_Obj_t * pNode = (Abc_Obj_t *)Vec_PtrEntry(vDrivers, Id+2);
    return Vec_IntEntry( vMapOut, Abc_ObjId(Abc_ObjFanin0(pNode)) );
}

/**Function*************************************************************

  Synopsis    [Creates the mapped network.]

  Description [Assuming the copy field of the mapped nodes are NULL.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkFromIf( If_Man_t * pIfMan, Abc_Ntk_t * pNtk )
{
    ProgressBar * pProgress;
    Abc_Ntk_t * pNtkNew;
    Abc_Obj_t * pNode, * pNodeNew;
    Vec_Int_t * vCover;
    int i, nDupGates;
    // create the new network
    if ( pIfMan->pPars->fUseBdds || pIfMan->pPars->fUseCnfs || pIfMan->pPars->fUseMv )
        pNtkNew = Abc_NtkStartFrom( pNtk, ABC_NTK_LOGIC, ABC_FUNC_BDD );
    else if ( pIfMan->pPars->fUseSops || pIfMan->pPars->fUserSesLib || pIfMan->pPars->nGateSize > 0 )
        pNtkNew = Abc_NtkStartFrom( pNtk, ABC_NTK_LOGIC, ABC_FUNC_SOP );
    else
        pNtkNew = Abc_NtkStartFrom( pNtk, ABC_NTK_LOGIC, ABC_FUNC_AIG );
    // prepare the mapping manager
    If_ManCleanNodeCopy( pIfMan );
    If_ManCleanCutData( pIfMan );
    // make the mapper point to the new network
    If_ObjSetCopy( If_ManConst1(pIfMan), Abc_NtkCreateNodeConst1(pNtkNew) );
    Abc_NtkForEachCi( pNtk, pNode, i )
        If_ObjSetCopy( If_ManCi(pIfMan, i), pNode->pCopy );

    // process the nodes in topological order
    vCover = Vec_IntAlloc( 1 << 16 );
    pProgress = Extra_ProgressBarStart( stdout, Abc_NtkCoNum(pNtk) );
    Abc_NtkForEachCo( pNtk, pNode, i )
    {
        Extra_ProgressBarUpdate( pProgress, i, "Final" );
        pNodeNew = Abc_NodeFromIf_rec( pNtkNew, pIfMan, If_ObjFanin0(If_ManCo(pIfMan, i)), vCover );
        pNodeNew = Abc_ObjNotCond( pNodeNew, If_ObjFaninC0(If_ManCo(pIfMan, i)) );
        Abc_ObjAddFanin( pNode->pCopy, pNodeNew );
    }
    Extra_ProgressBarStop( pProgress );
    Vec_IntFree( vCover );

    // remove the constant node if not used
    pNodeNew = (Abc_Obj_t *)If_ObjCopy( If_ManConst1(pIfMan) );
    if ( Abc_ObjFanoutNum(pNodeNew) == 0 && !Abc_ObjIsNone(pNodeNew) )
        Abc_NtkDeleteObj( pNodeNew );
    // minimize the node
    if ( pIfMan->pPars->fUseBdds || pIfMan->pPars->fUseCnfs || pIfMan->pPars->fUseMv )
        Abc_NtkSweep( pNtkNew, 0 );
    if ( pIfMan->pPars->fUseBdds )
        Abc_NtkBddReorder( pNtkNew, 0 );
    // decouple the PO driver nodes to reduce the number of levels
    nDupGates = Abc_NtkLogicMakeSimpleCos( pNtkNew, !pIfMan->pPars->fUseBuffs );
    if ( nDupGates && pIfMan->pPars->fVerbose && !Abc_FrameReadFlag("silentmode") )
    {
        if ( pIfMan->pPars->fUseBuffs )
            printf( "Added %d buffers/inverters to decouple the CO drivers.\n", nDupGates );
        else
            printf( "Duplicated %d gates to decouple the CO drivers.\n", nDupGates );
    }
    return pNtkNew;
}

/**Function*************************************************************

  Synopsis    [Rebuilds GIA from mini AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Hop_Obj_t * Abc_NodeBuildFromMiniInt( Hop_Man_t * pMan, Vec_Int_t * vAig, int nLeaves )
{
    assert( Vec_IntSize(vAig) > 0 );
    assert( Vec_IntEntryLast(vAig) < 2 );
    if ( Vec_IntSize(vAig) == 1 ) // const
    {
        assert( nLeaves == 0 );
        return Hop_NotCond( Hop_ManConst0(pMan), Vec_IntEntry(vAig, 0) );
    }
    if ( Vec_IntSize(vAig) == 2 ) // variable
    {
        assert( Vec_IntEntry(vAig, 0) == 0 );
        assert( nLeaves == 1 );
        return Hop_NotCond( Hop_IthVar(pMan, 0), Vec_IntEntry(vAig, 1) );
    }
    else
    {
        int i, iVar0, iVar1, iLit0, iLit1;
        Hop_Obj_t * piLit0, * piLit1, * piLit = NULL;
        assert( Vec_IntSize(vAig) & 1 );
        Vec_IntForEachEntryDouble( vAig, iLit0, iLit1, i )
        {
            iVar0 = Abc_Lit2Var( iLit0 );
            iVar1 = Abc_Lit2Var( iLit1 );
            piLit0 = Hop_NotCond( iVar0 < nLeaves ? Hop_IthVar(pMan, iVar0) : (Hop_Obj_t *)Vec_PtrEntry((Vec_Ptr_t *)vAig, iVar0 - nLeaves), Abc_LitIsCompl(iLit0) );
            piLit1 = Hop_NotCond( iVar1 < nLeaves ? Hop_IthVar(pMan, iVar1) : (Hop_Obj_t *)Vec_PtrEntry((Vec_Ptr_t *)vAig, iVar1 - nLeaves), Abc_LitIsCompl(iLit1) );
            piLit  = Hop_And( pMan, piLit0, piLit1 );
            assert( (i & 1) == 0 );
            Vec_PtrWriteEntry( (Vec_Ptr_t *)vAig, Abc_Lit2Var(i), piLit );  // overwriting entries
        }
        assert( i == Vec_IntSize(vAig) - 1 );
        piLit = Hop_NotCond( piLit, Vec_IntEntry(vAig, i) );
        Vec_IntClear( vAig ); // useless
        return piLit;
    }
}
Hop_Obj_t * Abc_NodeBuildFromMini( Hop_Man_t * pMan, If_Man_t * p, If_Cut_t * pCut, int fUseDsd )
{
    int Delay;
    if ( fUseDsd )
        Delay = If_CutDsdBalanceEval( p, pCut, p->vArray );
    else
        Delay = If_CutSopBalanceEval( p, pCut, p->vArray );
    assert( Delay >= 0 );
    return Abc_NodeBuildFromMiniInt( pMan, p->vArray, If_CutLeaveNum(pCut) );
}

/**Function*************************************************************
   Synopsis    [Implements decomposed LUT-structure of the cut.]
   Description []
                
   SideEffects []
   SeeAlso     []
 ***********************************************************************/
void Abc_DecRecordToHop( Abc_Ntk_t * pNtkNew, If_Man_t * pIfMan, If_Cut_t * pCutBest, If_Obj_t * pIfObj, Vec_Int_t * vCover, Abc_Obj_t * pNodeTop )
{
    extern Hop_Obj_t * Kit_TruthToHop( Hop_Man_t * pMan, unsigned * pTruth, int nVars, Vec_Int_t * vMemory );
    assert( !pIfMan->pPars->fUseTtPerm );

    // get the truth table
    word * pTruth = If_CutTruthW(pIfMan, pCutBest);
    int v;
    If_Obj_t * pIfLeaf;

    if ( pCutBest->nLeaves <= pIfMan->pPars->nLutDecSize )
    {
        /* add fanins */
        If_CutForEachLeaf( pIfMan, pCutBest, pIfLeaf, v )
            Abc_ObjAddFanin( pNodeTop, (Abc_Obj_t *)If_ObjCopy( pIfLeaf ) );
        
        pNodeTop->Level = Abc_ObjLevelNew( pNodeTop );

        pNodeTop->pData = Kit_TruthToHop( (Hop_Man_t *)pNtkNew->pManFunc, (unsigned *)pTruth, If_CutLeaveNum(pCutBest), vCover );
        return;
    }

    // get the delay profile
    unsigned delayProfile = pCutBest->decDelay;

    // perform LUT-decomposition and return the LUT-structure
    unsigned char decompArray[92];
    int val;
    if ( pIfMan->pPars->fUserLutDec )
    {
        val = acd_decompose( pTruth, pCutBest->nLeaves, pIfMan->pPars->nLutDecSize, &(delayProfile), decompArray );
    }
    else if ( pIfMan->pPars->fUserLut2D )
    {
        val = acd2_decompose( pTruth, pCutBest->nLeaves, pIfMan->pPars->nLutDecSize, &(delayProfile), decompArray );
    }
    else
    {
        val = acdXX_decompose( pTruth, pIfMan->pPars->nLutDecSize, pCutBest->nLeaves, decompArray );
    }
    assert( val == 0 );

    // convert the LUT-structure into a set of logic nodes in Abc_Ntk_t 
    unsigned char bytes_check = decompArray[0];
    assert( bytes_check <= 92 );

    int byte_p = 2;
    unsigned char i, j, k, num_fanins, num_words, num_bytes;
    int level, fanin;
    word *tt;
    Abc_Obj_t *pNewNodes[5];

    /* create intermediate LUTs */
    assert( decompArray[1] <= 6 );
    Abc_Obj_t * pFanin;
    for ( i = 0; i < decompArray[1]; ++i )
    {
        if ( i < decompArray[1] - 1 )
        {
            pNewNodes[i] = Abc_NtkCreateNode( pNtkNew );
        }
        else
        {
            pNewNodes[i] = pNodeTop;
        }
        num_fanins = decompArray[byte_p++];
        level = 0;
        for ( j = 0; j < num_fanins; ++j )
        {
            fanin = (int)decompArray[byte_p++];
            if ( fanin < If_CutLeaveNum(pCutBest) )
            {
                pFanin = (Abc_Obj_t *)If_ObjCopy( If_CutLeaf(pIfMan, pCutBest, fanin) );
            }
            else
            {
                assert( fanin - If_CutLeaveNum(pCutBest) < i );
                pFanin = pNewNodes[fanin - If_CutLeaveNum(pCutBest)];
            }
            Abc_ObjAddFanin( pNewNodes[i], pFanin );
            level = Abc_MaxInt( level, Abc_ObjLevel(pFanin) );
        }

        pNewNodes[i]->Level = level + (int)(Abc_ObjFaninNum(pNewNodes[i]) > 0);

        /* extract the truth table */
        tt = pIfMan->puTempW;
        num_words = ( num_fanins <= 6 ) ? 1 : ( 1 << ( num_fanins - 6 ) );
        num_bytes = ( num_fanins <= 3 ) ? 1 : ( 1 << ( Abc_MinInt( (int)num_fanins, 6 ) - 3 ) );
        for ( j = 0; j < num_words; ++j )
        {
            tt[j] = 0;
            for ( k = 0; k < num_bytes; ++k )
            {
                tt[j] |= ( (word)(decompArray[byte_p++]) ) << ( k << 3 );
            }
        }

        /* extend truth table if size < 5 */
        assert( num_fanins != 1 );
        if ( num_fanins == 2 )
        {
            tt[0] |= tt[0] << 4;
        }
        while ( num_bytes < 4 )
        {
            tt[0] |= tt[0] << ( num_bytes << 3 );
            num_bytes <<= 1;
        }

        /* add node data */
        pNewNodes[i]->pData = Kit_TruthToHop( (Hop_Man_t *)pNtkNew->pManFunc, (unsigned *)tt, (int) num_fanins, vCover );
    }

    /* check correct read */
    assert( byte_p == decompArray[0] );
}

/**Function*************************************************************

  Synopsis    [Derive one node after FPGA mapping.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Obj_t * Abc_NodeFromIf_rec( Abc_Ntk_t * pNtkNew, If_Man_t * pIfMan, If_Obj_t * pIfObj, Vec_Int_t * vCover )
{
    Abc_Obj_t * pNodeNew;
    If_Cut_t * pCutBest;
    If_Obj_t * pIfLeaf;
    int i;
    // return if the result if known
    pNodeNew = (Abc_Obj_t *)If_ObjCopy( pIfObj );
    if ( pNodeNew )
        return pNodeNew;
    assert( pIfObj->Type == IF_AND );
    // get the parameters of the best cut
    pCutBest = If_ObjCutBest( pIfObj );
    if ( pIfMan->pPars->fUserSesLib )
    {
        // create the subgraph composed of Abc_Obj_t nodes based on the given cut
        Abc_Obj_t * pFanins[IF_MAX_FUNC_LUTSIZE];
        If_CutForEachLeaf( pIfMan, pCutBest, pIfLeaf, i )
            pFanins[i] = Abc_NodeFromIf_rec(pNtkNew, pIfMan, pIfLeaf, vCover);
        pNodeNew = Abc_ExactBuildNode( If_CutTruthW(pIfMan, pCutBest), If_CutLeaveNum(pCutBest), If_CutArrTimeProfile(pIfMan, pCutBest), pFanins, pNtkNew );
        If_ObjSetCopy( pIfObj, pNodeNew );
        return pNodeNew;
    }
    // create a new node 
    pNodeNew = Abc_NtkCreateNode( pNtkNew );
//    if ( pIfMan->pPars->pLutLib && pIfMan->pPars->pLutLib->fVarPinDelays )
    if ( !pIfMan->pPars->fDelayOpt && !pIfMan->pPars->fDelayOptLut && !pIfMan->pPars->fDsdBalance && !pIfMan->pPars->fUseTtPerm && 
         !pIfMan->pPars->pLutStruct && !pIfMan->pPars->fUserLutDec && !pIfMan->pPars->fUserLut2D && !pIfMan->pPars->fUserRecLib &&
         !pIfMan->pPars->fUserSesLib && !pIfMan->pPars->nGateSize )
        If_CutRotatePins( pIfMan, pCutBest );
    if ( pIfMan->pPars->fUseCnfs || pIfMan->pPars->fUseMv )
    {
        If_CutForEachLeafReverse( pIfMan, pCutBest, pIfLeaf, i )
            Abc_ObjAddFanin( pNodeNew, Abc_NodeFromIf_rec(pNtkNew, pIfMan, pIfLeaf, vCover) );
    }
    else if ( pIfMan->pPars->fUserLutDec || pIfMan->pPars->fUserLut2D || pIfMan->pPars->fDeriveLuts )
    {
        If_CutForEachLeaf( pIfMan, pCutBest, pIfLeaf, i )
            Abc_NodeFromIf_rec(pNtkNew, pIfMan, pIfLeaf, vCover);
    }
    else
    {
        If_CutForEachLeaf( pIfMan, pCutBest, pIfLeaf, i )
            Abc_ObjAddFanin( pNodeNew, Abc_NodeFromIf_rec(pNtkNew, pIfMan, pIfLeaf, vCover) );
    }
    // set the level of the new node
    pNodeNew->Level = Abc_ObjLevelNew( pNodeNew );
    // derive the function of this node
    if ( pIfMan->pPars->fTruth )
    {
        if ( pIfMan->pPars->fUseBdds )
        { 
            // transform truth table into the BDD 
#ifdef ABC_USE_CUDD
            pNodeNew->pData = Kit_TruthToBdd( (DdManager *)pNtkNew->pManFunc, If_CutTruth(pIfMan, pCutBest), If_CutLeaveNum(pCutBest), 0 );  Cudd_Ref((DdNode *)pNodeNew->pData); 
#endif
        }
        else if ( pIfMan->pPars->fUseCnfs || pIfMan->pPars->fUseMv )
        { 
            // transform truth table into the BDD 
#ifdef ABC_USE_CUDD
            pNodeNew->pData = Kit_TruthToBdd( (DdManager *)pNtkNew->pManFunc, If_CutTruth(pIfMan, pCutBest), If_CutLeaveNum(pCutBest), 1 );  Cudd_Ref((DdNode *)pNodeNew->pData); 
#endif
        }
        else if ( pIfMan->pPars->fUseSops || pIfMan->pPars->nGateSize > 0 ) 
        {
            // transform truth table into the SOP
            int RetValue = Kit_TruthIsop( If_CutTruth(pIfMan, pCutBest), If_CutLeaveNum(pCutBest), vCover, 1 );
            assert( RetValue == 0 || RetValue == 1 );
            // check the case of constant cover
            if ( Vec_IntSize(vCover) == 0 || (Vec_IntSize(vCover) == 1 && Vec_IntEntry(vCover,0) == 0) )
            {
                assert( RetValue == 0 );
                pNodeNew->pData = Abc_SopCreateAnd( (Mem_Flex_t *)pNtkNew->pManFunc, If_CutLeaveNum(pCutBest), NULL );
                pNodeNew = (Vec_IntSize(vCover) == 0) ? Abc_NtkCreateNodeConst0(pNtkNew) : Abc_NtkCreateNodeConst1(pNtkNew);
            }
            else
            {
                // derive the AIG for that tree
                pNodeNew->pData = Abc_SopCreateFromIsop( (Mem_Flex_t *)pNtkNew->pManFunc, If_CutLeaveNum(pCutBest), vCover );
                if ( RetValue )
                    Abc_SopComplement( (char *)pNodeNew->pData );
            }
        }
        else if ( pIfMan->pPars->fDelayOpt )
            pNodeNew->pData = Abc_NodeBuildFromMini( (Hop_Man_t *)pNtkNew->pManFunc, pIfMan, pCutBest, 0 );
        else if ( pIfMan->pPars->fDsdBalance )
            pNodeNew->pData = Abc_NodeBuildFromMini( (Hop_Man_t *)pNtkNew->pManFunc, pIfMan, pCutBest, 1 );
        else if ( pIfMan->pPars->fUserRecLib )
        {
            extern Hop_Obj_t * Abc_RecToHop3( Hop_Man_t * pMan, If_Man_t * pIfMan, If_Cut_t * pCut, If_Obj_t * pIfObj );
            pNodeNew->pData = Abc_RecToHop3( (Hop_Man_t *)pNtkNew->pManFunc, pIfMan, pCutBest, pIfObj ); 
        }
        else if ( pIfMan->pPars->fUserLutDec || pIfMan->pPars->fUserLut2D || pIfMan->pPars->fDeriveLuts )
        {
            extern void Abc_DecRecordToHop( Abc_Ntk_t * pNtkNew, If_Man_t * pIfMan, If_Cut_t * pCut, If_Obj_t * pIfObj, Vec_Int_t * vMemory, Abc_Obj_t * pNodeTop );
            Abc_DecRecordToHop( pNtkNew, pIfMan, pCutBest, pIfObj, vCover, pNodeNew ); 
        }
        else
        {
            extern Hop_Obj_t * Kit_TruthToHop( Hop_Man_t * pMan, unsigned * pTruth, int nVars, Vec_Int_t * vMemory );
            word * pTruth = If_CutTruthW(pIfMan, pCutBest);
            if ( pIfMan->pPars->fUseTtPerm )
                for ( i = 0; i < (int)pCutBest->nLeaves; i++ )
                    if ( If_CutLeafBit(pCutBest, i) )
                        Abc_TtFlip( pTruth, Abc_TtWordNum(pCutBest->nLeaves), i );
            pNodeNew->pData = Kit_TruthToHop( (Hop_Man_t *)pNtkNew->pManFunc, (unsigned *)pTruth, If_CutLeaveNum(pCutBest), vCover );
//            if ( pIfMan->pPars->fUseBat )
//                Bat_ManFuncPrintCell( *pTruth );
        }
        // complement the node if the cut was complemented
        if ( pCutBest->fCompl && !pIfMan->pPars->fDelayOpt && !pIfMan->pPars->fDsdBalance )
            Abc_NodeComplement( pNodeNew );
    }
    else
    {
        pNodeNew->pData = Abc_NodeIfToHop( (Hop_Man_t *)pNtkNew->pManFunc, pIfMan, pIfObj );
    }
    If_ObjSetCopy( pIfObj, pNodeNew );
    return pNodeNew;
}

/**Function*************************************************************

  Synopsis    [Recursively derives the truth table for the cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Hop_Obj_t * Abc_NodeIfToHop_rec( Hop_Man_t * pHopMan, If_Man_t * pIfMan, If_Obj_t * pIfObj, Vec_Ptr_t * vVisited )
{
    If_Cut_t * pCut;
    Hop_Obj_t * gFunc, * gFunc0, * gFunc1;
    // get the best cut
    pCut = If_ObjCutBest(pIfObj);
    // if the cut is visited, return the result
    if ( If_CutData(pCut) )
        return (Hop_Obj_t *)If_CutData(pCut);
    // compute the functions of the children
    gFunc0 = Abc_NodeIfToHop_rec( pHopMan, pIfMan, pIfObj->pFanin0, vVisited );
    gFunc1 = Abc_NodeIfToHop_rec( pHopMan, pIfMan, pIfObj->pFanin1, vVisited );
    // get the function of the cut
    gFunc  = Hop_And( pHopMan, Hop_NotCond(gFunc0, pIfObj->fCompl0), Hop_NotCond(gFunc1, pIfObj->fCompl1) );  
    assert( If_CutData(pCut) == NULL );
    If_CutSetData( pCut, gFunc );
    // add this cut to the visited list
    Vec_PtrPush( vVisited, pCut );
    return gFunc;
}


/**Function*************************************************************

  Synopsis    [Recursively derives the truth table for the cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Hop_Obj_t * Abc_NodeIfToHop2_rec( Hop_Man_t * pHopMan, If_Man_t * pIfMan, If_Obj_t * pIfObj, Vec_Ptr_t * vVisited )
{
    If_Cut_t * pCut;
    If_Obj_t * pTemp;
    Hop_Obj_t * gFunc, * gFunc0, * gFunc1;
    // get the best cut
    pCut = If_ObjCutBest(pIfObj);
    // if the cut is visited, return the result
    if ( If_CutData(pCut) )
        return (Hop_Obj_t *)If_CutData(pCut);
    // mark the node as visited
    Vec_PtrPush( vVisited, pCut );
    // insert the worst case
    If_CutSetData( pCut, (void *)1 );
    // skip in case of primary input
    if ( If_ObjIsCi(pIfObj) )
        return (Hop_Obj_t *)If_CutData(pCut);
    // compute the functions of the children
    for ( pTemp = pIfObj; pTemp; pTemp = pTemp->pEquiv )
    {
        gFunc0 = Abc_NodeIfToHop2_rec( pHopMan, pIfMan, pTemp->pFanin0, vVisited );
        if ( gFunc0 == (void *)1 )
            continue;
        gFunc1 = Abc_NodeIfToHop2_rec( pHopMan, pIfMan, pTemp->pFanin1, vVisited );
        if ( gFunc1 == (void *)1 )
            continue;
        // both branches are solved
        gFunc = Hop_And( pHopMan, Hop_NotCond(gFunc0, pTemp->fCompl0), Hop_NotCond(gFunc1, pTemp->fCompl1) ); 
        if ( pTemp->fPhase != pIfObj->fPhase )
            gFunc = Hop_Not(gFunc);
        If_CutSetData( pCut, gFunc );
        break;
    }
    return (Hop_Obj_t *)If_CutData(pCut);
}

/**Function*************************************************************

  Synopsis    [Derives the truth table for one cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Hop_Obj_t * Abc_NodeIfToHop( Hop_Man_t * pHopMan, If_Man_t * pIfMan, If_Obj_t * pIfObj )
{
    If_Cut_t * pCut;
    Hop_Obj_t * gFunc;
    If_Obj_t * pLeaf;
    int i;
    // get the best cut
    pCut = If_ObjCutBest(pIfObj);
    assert( pCut->nLeaves > 1 );
    // set the leaf variables
    If_CutForEachLeaf( pIfMan, pCut, pLeaf, i )
        If_CutSetData( If_ObjCutBest(pLeaf), Hop_IthVar(pHopMan, i) );
    // recursively compute the function while collecting visited cuts
    Vec_PtrClear( pIfMan->vTemp );
    gFunc = Abc_NodeIfToHop2_rec( pHopMan, pIfMan, pIfObj, pIfMan->vTemp ); 
    if ( gFunc == (void *)1 )
    {
        printf( "Abc_NodeIfToHop(): Computing local AIG has failed.\n" );
        return NULL;
    }
    // clean the cuts
    If_CutForEachLeaf( pIfMan, pCut, pLeaf, i )
        If_CutSetData( If_ObjCutBest(pLeaf), NULL );
    Vec_PtrForEachEntry( If_Cut_t *, pIfMan->vTemp, pCut, i )
        If_CutSetData( pCut, NULL );
    return gFunc;
}


/**Function*************************************************************

  Synopsis    [Comparison for two nodes with the flow.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_ObjCompareFlow( Abc_Obj_t ** ppNode0, Abc_Obj_t ** ppNode1 )
{
    float Flow0 = Abc_Int2Float((int)(ABC_PTRINT_T)(*ppNode0)->pCopy);
    float Flow1 = Abc_Int2Float((int)(ABC_PTRINT_T)(*ppNode1)->pCopy);
    if ( Flow0 > Flow1 )
        return -1;
    if ( Flow0 < Flow1 )
        return 1;
    return 0;
}

/**Function*************************************************************

  Synopsis    [Orders AIG nodes so that nodes from larger cones go first.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkFindGoodOrder_rec( Abc_Obj_t * pNode, Vec_Ptr_t * vNodes )
{
    if ( !Abc_ObjIsNode(pNode) )
        return;
    assert( Abc_ObjIsNode( pNode ) );
    // if this node is already visited, skip
    if ( Abc_NodeIsTravIdCurrent( pNode ) )
        return;
    // mark the node as visited
    Abc_NodeSetTravIdCurrent( pNode );
    // visit the transitive fanin of the node
    Abc_NtkFindGoodOrder_rec( Abc_ObjFanin0(pNode), vNodes );
    Abc_NtkFindGoodOrder_rec( Abc_ObjFanin1(pNode), vNodes );
    // add the node after the fanins have been added
    Vec_PtrPush( vNodes, pNode );
}

/**Function*************************************************************

  Synopsis    [Orders AIG nodes so that nodes from larger cones go first.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Abc_NtkFindGoodOrder( Abc_Ntk_t * pNtk )
{
    Vec_Ptr_t * vNodes, * vCos;
    Abc_Obj_t * pNode, * pFanin0, * pFanin1;
    float Flow0, Flow1;
    int i;

    // initialize the flow
    Abc_AigConst1(pNtk)->pCopy = NULL;
    Abc_NtkForEachCi( pNtk, pNode, i )
        pNode->pCopy = NULL;
    // compute the flow
    Abc_AigForEachAnd( pNtk, pNode, i )
    {
        pFanin0 = Abc_ObjFanin0(pNode);
        pFanin1 = Abc_ObjFanin1(pNode);
        Flow0 = Abc_Int2Float((int)(ABC_PTRINT_T)pFanin0->pCopy)/Abc_ObjFanoutNum(pFanin0);
        Flow1 = Abc_Int2Float((int)(ABC_PTRINT_T)pFanin1->pCopy)/Abc_ObjFanoutNum(pFanin1);
        pNode->pCopy = (Abc_Obj_t *)(ABC_PTRINT_T)Abc_Float2Int(Flow0 + Flow1+(float)1.0);
    }
    // find the flow of the COs
    vCos = Vec_PtrAlloc( Abc_NtkCoNum(pNtk) );
    Abc_NtkForEachCo( pNtk, pNode, i )
    {
        pNode->pCopy = Abc_ObjFanin0(pNode)->pCopy;
//        pNode->pCopy = (Abc_Obj_t *)Abc_Float2Int((float)Abc_ObjFanin0(pNode)->Level);
        Vec_PtrPush( vCos, pNode );
    }

    // sort nodes in the increasing order of the flow
    qsort( (Abc_Obj_t **)Vec_PtrArray(vCos), (size_t)Abc_NtkCoNum(pNtk), 
        sizeof(Abc_Obj_t *), (int (*)(const void *, const void *))Abc_ObjCompareFlow );
    // verify sorting
    pFanin0 = (Abc_Obj_t *)Vec_PtrEntry(vCos, 0);
    pFanin1 = (Abc_Obj_t *)Vec_PtrEntryLast(vCos);
    assert( Abc_Int2Float((int)(ABC_PTRINT_T)pFanin0->pCopy) >= Abc_Int2Float((int)(ABC_PTRINT_T)pFanin1->pCopy) );

    // collect the nodes in the topological order from the new array
    Abc_NtkIncrementTravId( pNtk );
    vNodes = Vec_PtrAlloc( 100 );
    Vec_PtrForEachEntry( Abc_Obj_t *, vCos, pNode, i )
        Abc_NtkFindGoodOrder_rec( Abc_ObjFanin0(pNode), vNodes );
    Vec_PtrFree( vCos );
    return vNodes;
}


/**Function*************************************************************

  Synopsis    [Sets PO drivers.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkMarkMux( Abc_Obj_t * pDriver, Abc_Obj_t ** ppNode1, Abc_Obj_t ** ppNode2 )
{
    Abc_Obj_t * pNodeC, * pNodeT, * pNodeE;
    If_Obj_t * pIfObj;

    *ppNode1 = NULL;
    *ppNode2 = NULL;
    if ( pDriver == NULL )
        return;
    if ( !Abc_NodeIsMuxType(pDriver) )
        return;

    pNodeC = Abc_NodeRecognizeMux( pDriver, &pNodeT, &pNodeE );

    pIfObj = If_Regular( (If_Obj_t *)Abc_ObjFanin0(pDriver)->pCopy );
    if ( If_ObjIsAnd(pIfObj) )
        pIfObj->fSkipCut = 1;
    pIfObj = If_Regular( (If_Obj_t *)Abc_ObjFanin1(pDriver)->pCopy );
    if ( If_ObjIsAnd(pIfObj) )
        pIfObj->fSkipCut = 1;

    pIfObj = If_Regular( (If_Obj_t *)Abc_ObjRegular(pNodeC)->pCopy );
    if ( If_ObjIsAnd(pIfObj) )
        pIfObj->fSkipCut = 1;

/*
    pIfObj = If_Regular( (If_Obj_t *)Abc_ObjRegular(pNodeT)->pCopy );
    if ( If_ObjIsAnd(pIfObj) )
        pIfObj->fSkipCut = 1;
    pIfObj = If_Regular( (If_Obj_t *)Abc_ObjRegular(pNodeE)->pCopy );
    if ( If_ObjIsAnd(pIfObj) )
        pIfObj->fSkipCut = 1;
*/
    *ppNode1 = Abc_ObjRegular(pNodeC);
    *ppNode2 = Abc_ObjRegular(pNodeT);
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

