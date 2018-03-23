/**CFile****************************************************************

  FileName    [abcFraig.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Network and node package.]

  Synopsis    [Procedures interfacing with the FRAIG package.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: abcFraig.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "base/abc/abc.h"
#include "proof/fraig/fraig.h"
#include "base/main/main.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

extern Abc_Ntk_t *    Abc_NtkFromFraig( Fraig_Man_t * pMan, Abc_Ntk_t * pNtk );
static Abc_Ntk_t *    Abc_NtkFromFraig2( Fraig_Man_t * pMan, Abc_Ntk_t * pNtk );
static Abc_Obj_t *    Abc_NodeFromFraig_rec( Abc_Ntk_t * pNtkNew, Fraig_Node_t * pNodeFraig );
static void           Abc_NtkFromFraig2_rec( Abc_Ntk_t * pNtkNew, Abc_Obj_t * pNode, Vec_Ptr_t * vNodeReprs );
extern Fraig_Node_t * Abc_NtkToFraigExdc( Fraig_Man_t * pMan, Abc_Ntk_t * pNtkMain, Abc_Ntk_t * pNtkExdc );
static void           Abc_NtkFraigRemapUsingExdc( Fraig_Man_t * pMan, Abc_Ntk_t * pNtk );

static int            Abc_NtkFraigTrustCheck( Abc_Ntk_t * pNtk );
static void           Abc_NtkFraigTrustOne( Abc_Ntk_t * pNtk, Abc_Ntk_t * pNtkNew );
static Abc_Obj_t *    Abc_NodeFraigTrust( Abc_Ntk_t * pNtkNew, Abc_Obj_t * pNode );
 
////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Interfaces the network with the FRAIG package.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkFraig( Abc_Ntk_t * pNtk, void * pParams, int fAllNodes, int fExdc )
{
    Fraig_Params_t * pPars = (Fraig_Params_t *)pParams;
    Abc_Ntk_t * pNtkNew;
    Fraig_Man_t * pMan; 
    // check if EXDC is present
    if ( fExdc && pNtk->pExdc == NULL )
        fExdc = 0, printf( "Warning: Networks has no EXDC.\n" );
    // perform fraiging
    pMan = (Fraig_Man_t *)Abc_NtkToFraig( pNtk, pParams, fAllNodes, fExdc ); 
    // add algebraic choices
//    if ( pPars->fChoicing )
//        Fraig_ManAddChoices( pMan, 0, 6 );
    // prove the miter if asked to
    if ( pPars->fTryProve )
        Fraig_ManProveMiter( pMan );
    // reconstruct FRAIG in the new network
    if ( fExdc ) 
        pNtkNew = Abc_NtkFromFraig2( pMan, pNtk );
    else
        pNtkNew = Abc_NtkFromFraig( pMan, pNtk );
    Fraig_ManFree( pMan );
    if ( pNtk->pExdc )
        pNtkNew->pExdc = Abc_NtkDup( pNtk->pExdc );
    // make sure that everything is okay
    if ( !Abc_NtkCheck( pNtkNew ) )
    {
        printf( "Abc_NtkFraig: The network check has failed.\n" );
        Abc_NtkDelete( pNtkNew );
        return NULL;
    }
    return pNtkNew;
}

/**Function*************************************************************

  Synopsis    [Transforms the strashed network into FRAIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void * Abc_NtkToFraig( Abc_Ntk_t * pNtk, void * pParams, int fAllNodes, int fExdc )
{
    int fInternal = ((Fraig_Params_t *)pParams)->fInternal;
    Fraig_Man_t * pMan;
    ProgressBar * pProgress = NULL;
    Vec_Ptr_t * vNodes;
    Abc_Obj_t * pNode;
    int i;

    assert( Abc_NtkIsStrash(pNtk) );

    // create the FRAIG manager
    pMan = Fraig_ManCreate( (Fraig_Params_t *)pParams );

    // map the constant node
    Abc_NtkCleanCopy( pNtk );
    Abc_AigConst1(pNtk)->pCopy = (Abc_Obj_t *)Fraig_ManReadConst1(pMan);
    // create PIs and remember them in the old nodes
    Abc_NtkForEachCi( pNtk, pNode, i )
        pNode->pCopy = (Abc_Obj_t *)Fraig_ManReadIthVar(pMan, i);
 
    // perform strashing
    vNodes = Abc_AigDfs( pNtk, fAllNodes, 0 );
    if ( !fInternal )
        pProgress = Extra_ProgressBarStart( stdout, vNodes->nSize );
    Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pNode, i )
    {
        if ( Abc_ObjFaninNum(pNode) == 0 )
            continue;
        if ( pProgress ) 
            Extra_ProgressBarUpdate( pProgress, i, NULL );
        pNode->pCopy = (Abc_Obj_t *)Fraig_NodeAnd( pMan, 
                Fraig_NotCond( Abc_ObjFanin0(pNode)->pCopy, (int)Abc_ObjFaninC0(pNode) ),
                Fraig_NotCond( Abc_ObjFanin1(pNode)->pCopy, (int)Abc_ObjFaninC1(pNode) ) );
    }
    if ( pProgress )
        Extra_ProgressBarStop( pProgress );
    Vec_PtrFree( vNodes );

    // use EXDC to change the mapping of nodes into FRAIG nodes
    if ( fExdc )
        Abc_NtkFraigRemapUsingExdc( pMan, pNtk );

    // set the primary outputs
    Abc_NtkForEachCo( pNtk, pNode, i )
        Fraig_ManSetPo( pMan, (Fraig_Node_t *)Abc_ObjNotCond( Abc_ObjFanin0(pNode)->pCopy, Abc_ObjFaninC0(pNode) ) );
    return pMan;
}

/**Function*************************************************************

  Synopsis    [Derives EXDC node for the given network.]

  Description [Assumes that EXDCs of all POs are the same.
  Returns the EXDC of the first PO.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fraig_Node_t * Abc_NtkToFraigExdc( Fraig_Man_t * pMan, Abc_Ntk_t * pNtkMain, Abc_Ntk_t * pNtkExdc )
{
    Abc_Ntk_t * pNtkStrash;
    Abc_Obj_t * pObj;
    Fraig_Node_t * gResult;
    char ** ppNames;
    int i, k;
    // strash the EXDC network
    pNtkStrash = Abc_NtkStrash( pNtkExdc, 0, 0, 0 );
    Abc_NtkCleanCopy( pNtkStrash );
    Abc_AigConst1(pNtkStrash)->pCopy = (Abc_Obj_t *)Fraig_ManReadConst1(pMan);
    // set the mapping of the PI nodes
    ppNames = Abc_NtkCollectCioNames( pNtkMain, 0 );
    Abc_NtkForEachCi( pNtkStrash, pObj, i )
    {
        for ( k = 0; k < Abc_NtkCiNum(pNtkMain); k++ )
            if ( strcmp( Abc_ObjName(pObj), ppNames[k] ) == 0 )
            {
                pObj->pCopy = (Abc_Obj_t *)Fraig_ManReadIthVar(pMan, k);
                break;
            }
        assert( pObj->pCopy != NULL );
    }
    ABC_FREE( ppNames );
    // build FRAIG for each node
    Abc_AigForEachAnd( pNtkStrash, pObj, i )
        pObj->pCopy = (Abc_Obj_t *)Fraig_NodeAnd( pMan, 
                Fraig_NotCond( Abc_ObjFanin0(pObj)->pCopy, (int)Abc_ObjFaninC0(pObj) ),
                Fraig_NotCond( Abc_ObjFanin1(pObj)->pCopy, (int)Abc_ObjFaninC1(pObj) ) );
    // get the EXDC to be returned
    pObj = Abc_NtkPo( pNtkStrash, 0 );
    gResult = Fraig_NotCond( Abc_ObjFanin0(pObj)->pCopy, (int)Abc_ObjFaninC0(pObj) );
    Abc_NtkDelete( pNtkStrash );
    return gResult;
}


/**Function*************************************************************

  Synopsis    [Changes mapping of the old nodes into FRAIG nodes using EXDC.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkFraigRemapUsingExdc( Fraig_Man_t * pMan, Abc_Ntk_t * pNtk )
{
    Fraig_Node_t * gNodeNew, * gNodeExdc;
    stmm_table * tTable;
    stmm_generator * gen;
    Abc_Obj_t * pNode, * pNodeBest;
    Abc_Obj_t * pClass, ** ppSlot;
    Vec_Ptr_t * vNexts;
    int i;

    // get the global don't-cares
    assert( pNtk->pExdc );
    gNodeExdc = Abc_NtkToFraigExdc( pMan, pNtk, pNtk->pExdc );

    // save the next pointers
    vNexts = Vec_PtrStart( Abc_NtkObjNumMax(pNtk) );
    Abc_NtkForEachNode( pNtk, pNode, i )
        Vec_PtrWriteEntry( vNexts, pNode->Id, pNode->pNext );

    // find the classes of AIG nodes which have FRAIG nodes assigned
    Abc_NtkCleanNext( pNtk );
    tTable = stmm_init_table(stmm_ptrcmp,stmm_ptrhash);
    Abc_NtkForEachNode( pNtk, pNode, i )
        if ( pNode->pCopy )
        {
            gNodeNew = Fraig_NodeAnd( pMan, (Fraig_Node_t *)pNode->pCopy, Fraig_Not(gNodeExdc) );
            if ( !stmm_find_or_add( tTable, (char *)Fraig_Regular(gNodeNew), (char ***)&ppSlot ) )
                *ppSlot = NULL;
            pNode->pNext = *ppSlot;
            *ppSlot = pNode;
        }

    // for reach non-trival class, find the node with minimum level, and replace other nodes by it
    Abc_AigSetNodePhases( pNtk );
    stmm_foreach_item( tTable, gen, (char **)&gNodeNew, (char **)&pClass )
    {
        if ( pClass->pNext == NULL )
            continue;
        // find the node with minimum level
        pNodeBest = pClass;
        for ( pNode = pClass->pNext; pNode; pNode = pNode->pNext )
            if ( pNodeBest->Level > pNode->Level )
                 pNodeBest = pNode;
        // remap the class nodes
        for ( pNode = pClass; pNode; pNode = pNode->pNext )
            pNode->pCopy = Abc_ObjNotCond( pNodeBest->pCopy, pNode->fPhase ^ pNodeBest->fPhase );
    }
    stmm_free_table( tTable );

    // restore the next pointers
    Abc_NtkCleanNext( pNtk );
    Abc_NtkForEachNode( pNtk, pNode, i )
        pNode->pNext = (Abc_Obj_t *)Vec_PtrEntry( vNexts, pNode->Id );
    Vec_PtrFree( vNexts );
}

/**Function*************************************************************

  Synopsis    [Transforms FRAIG into strashed network with choices.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkFromFraig( Fraig_Man_t * pMan, Abc_Ntk_t * pNtk )
{
    ProgressBar * pProgress;
    Abc_Ntk_t * pNtkNew;
    Abc_Obj_t * pNode, * pNodeNew;
    int i;
    // create the new network
    pNtkNew = Abc_NtkStartFrom( pNtk, ABC_NTK_STRASH, ABC_FUNC_AIG );
    // make the mapper point to the new network
    Abc_NtkForEachCi( pNtk, pNode, i )
        Fraig_NodeSetData1( Fraig_ManReadIthVar(pMan, i), (Fraig_Node_t *)pNode->pCopy );
    // set the constant node
    Fraig_NodeSetData1( Fraig_ManReadConst1(pMan), (Fraig_Node_t *)Abc_AigConst1(pNtkNew) );
    // process the nodes in topological order
    pProgress = Extra_ProgressBarStart( stdout, Abc_NtkCoNum(pNtk) );
    Abc_NtkForEachCo( pNtk, pNode, i )
    {
        Extra_ProgressBarUpdate( pProgress, i, NULL );
        pNodeNew = Abc_NodeFromFraig_rec( pNtkNew, Fraig_ManReadOutputs(pMan)[i] );
        Abc_ObjAddFanin( pNode->pCopy, pNodeNew );
    }
    Extra_ProgressBarStop( pProgress );
    Abc_NtkReassignIds( pNtkNew );
    return pNtkNew;
}

/**Function*************************************************************

  Synopsis    [Transforms into AIG one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Obj_t * Abc_NodeFromFraig_rec( Abc_Ntk_t * pNtkNew, Fraig_Node_t * pNodeFraig )
{
    Abc_Obj_t * pRes, * pRes0, * pRes1, * pResMin, * pResCur;
    Fraig_Node_t * pNodeTemp, * pNodeFraigR = Fraig_Regular(pNodeFraig);
    void ** ppTail;
    // check if the node was already considered
    if ( (pRes = (Abc_Obj_t *)Fraig_NodeReadData1(pNodeFraigR)) )
        return Abc_ObjNotCond( pRes, Fraig_IsComplement(pNodeFraig) );
    // solve the children
    pRes0 = Abc_NodeFromFraig_rec( pNtkNew, Fraig_NodeReadOne(pNodeFraigR) );
    pRes1 = Abc_NodeFromFraig_rec( pNtkNew, Fraig_NodeReadTwo(pNodeFraigR) );
    // derive the new node
    pRes = Abc_AigAnd( (Abc_Aig_t *)pNtkNew->pManFunc, pRes0, pRes1 );
    pRes->fPhase = Fraig_NodeReadSimInv( pNodeFraigR );
    // if the node has an equivalence class, find its representative
    if ( Fraig_NodeReadRepr(pNodeFraigR) == NULL && Fraig_NodeReadNextE(pNodeFraigR) != NULL )
    {
        // go through the FRAIG nodes belonging to this equivalence class
        // and find the representative node (the node with the smallest level)
        pResMin = pRes;
        for ( pNodeTemp = Fraig_NodeReadNextE(pNodeFraigR); pNodeTemp; pNodeTemp = Fraig_NodeReadNextE(pNodeTemp) )
        {
            assert( Fraig_NodeReadData1(pNodeTemp) == NULL );
            pResCur = Abc_NodeFromFraig_rec( pNtkNew, pNodeTemp );
            if ( pResMin->Level > pResCur->Level )
                pResMin = pResCur;
        }
        // link the nodes in such a way that representative goes first
        ppTail = &pResMin->pData;
        if ( pRes != pResMin )
        {
            *ppTail = pRes;
            ppTail = &pRes->pData;
        }
        for ( pNodeTemp = Fraig_NodeReadNextE(pNodeFraigR); pNodeTemp; pNodeTemp = Fraig_NodeReadNextE(pNodeTemp) )
        {
            pResCur = (Abc_Obj_t *)Fraig_NodeReadData1(pNodeTemp);
            assert( pResCur );
            if ( pResMin == pResCur )
                continue;
            *ppTail = pResCur;
            ppTail = &pResCur->pData;
        }
        assert( *ppTail == NULL );

        // update the phase of the node
        pRes = Abc_ObjNotCond( pResMin, (pRes->fPhase ^ pResMin->fPhase) );
    }
    Fraig_NodeSetData1( pNodeFraigR, (Fraig_Node_t *)pRes );
    return Abc_ObjNotCond( pRes, Fraig_IsComplement(pNodeFraig) );
}

/**Function*************************************************************

  Synopsis    [Transforms FRAIG into strashed network without choices.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkFromFraig2( Fraig_Man_t * pMan, Abc_Ntk_t * pNtk )
{
    ProgressBar * pProgress;
    stmm_table * tTable;
    Vec_Ptr_t * vNodeReprs;
    Abc_Ntk_t * pNtkNew;
    Abc_Obj_t * pNode, * pRepr, ** ppSlot;
    int i;

    // map the nodes into their lowest level representives
    tTable = stmm_init_table(stmm_ptrcmp,stmm_ptrhash);
    pNode = Abc_AigConst1(pNtk);
    if ( !stmm_find_or_add( tTable, (char *)Fraig_Regular(pNode->pCopy), (char ***)&ppSlot ) )
        *ppSlot = pNode;
    Abc_NtkForEachCi( pNtk, pNode, i )
        if ( !stmm_find_or_add( tTable, (char *)Fraig_Regular(pNode->pCopy), (char ***)&ppSlot ) )
            *ppSlot = pNode;
    Abc_NtkForEachNode( pNtk, pNode, i )
        if ( pNode->pCopy )
        {
            if ( !stmm_find_or_add( tTable, (char *)Fraig_Regular(pNode->pCopy), (char ***)&ppSlot ) )
                *ppSlot = pNode;
            else if ( (*ppSlot)->Level > pNode->Level )
                *ppSlot = pNode;
        }
    // save representatives for each node
    vNodeReprs = Vec_PtrStart( Abc_NtkObjNumMax(pNtk) );
    Abc_NtkForEachNode( pNtk, pNode, i )
        if ( pNode->pCopy )
        {           
            if ( !stmm_lookup( tTable, (char *)Fraig_Regular(pNode->pCopy), (char **)&pRepr ) )
                assert( 0 );
            if ( pNode != pRepr )
                Vec_PtrWriteEntry( vNodeReprs, pNode->Id, pRepr );
        }
    stmm_free_table( tTable );

    // create the new network
    pNtkNew = Abc_NtkStartFrom( pNtk, ABC_NTK_STRASH, ABC_FUNC_AIG );

    // perform strashing
    Abc_AigSetNodePhases( pNtk );
    Abc_NtkIncrementTravId( pNtk );
    pProgress = Extra_ProgressBarStart( stdout, Abc_NtkCoNum(pNtk) );
    Abc_NtkForEachCo( pNtk, pNode, i )
    {
        Extra_ProgressBarUpdate( pProgress, i, NULL );
        Abc_NtkFromFraig2_rec( pNtkNew, Abc_ObjFanin0(pNode), vNodeReprs );
    }
    Extra_ProgressBarStop( pProgress );
    Vec_PtrFree( vNodeReprs );

    // finalize the network
    Abc_NtkFinalize( pNtk, pNtkNew );
    return pNtkNew;
}


/**Function*************************************************************

  Synopsis    [Transforms into AIG one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkFromFraig2_rec( Abc_Ntk_t * pNtkNew, Abc_Obj_t * pNode, Vec_Ptr_t * vNodeReprs )
{
    Abc_Obj_t * pRepr;
    // skip the PIs and constants
    if ( Abc_ObjFaninNum(pNode) < 2 )
        return;
    // if this node is already visited, skip
    if ( Abc_NodeIsTravIdCurrent( pNode ) )
        return;
    // mark the node as visited
    Abc_NodeSetTravIdCurrent( pNode );
    assert( Abc_ObjIsNode( pNode ) );
    // get the node's representative
    if ( (pRepr = (Abc_Obj_t *)Vec_PtrEntry(vNodeReprs, pNode->Id)) )
    {
        Abc_NtkFromFraig2_rec( pNtkNew, pRepr, vNodeReprs );
        pNode->pCopy = Abc_ObjNotCond( pRepr->pCopy, pRepr->fPhase ^ pNode->fPhase );
        return;
    }
    Abc_NtkFromFraig2_rec( pNtkNew, Abc_ObjFanin0(pNode), vNodeReprs );
    Abc_NtkFromFraig2_rec( pNtkNew, Abc_ObjFanin1(pNode), vNodeReprs );
    pNode->pCopy = Abc_AigAnd( (Abc_Aig_t *)pNtkNew->pManFunc, Abc_ObjChild0Copy(pNode), Abc_ObjChild1Copy(pNode) );
}



/**Function*************************************************************

  Synopsis    [Interfaces the network with the FRAIG package.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkFraigTrust( Abc_Ntk_t * pNtk )
{
    Abc_Ntk_t * pNtkNew;

    if ( !Abc_NtkIsSopLogic(pNtk) )
    {
        printf( "Abc_NtkFraigTrust: Trust mode works for netlists and logic SOP networks.\n" );
        return NULL;
    }

    if ( !Abc_NtkFraigTrustCheck(pNtk) )
    {
        printf( "Abc_NtkFraigTrust: The network does not look like an AIG with choice nodes.\n" );
        return NULL;
    }
    
    // perform strashing
    pNtkNew = Abc_NtkStartFrom( pNtk, ABC_NTK_STRASH, ABC_FUNC_AIG );
    Abc_NtkFraigTrustOne( pNtk, pNtkNew );
    Abc_NtkFinalize( pNtk, pNtkNew );
    Abc_NtkReassignIds( pNtkNew );

    // print a warning about choice nodes
    printf( "Warning: The resulting AIG contains %d choice nodes.\n", Abc_NtkGetChoiceNum( pNtkNew ) );

    // make sure that everything is okay
    if ( !Abc_NtkCheck( pNtkNew ) )
    {
        printf( "Abc_NtkFraigTrust: The network check has failed.\n" );
        Abc_NtkDelete( pNtkNew );
        return NULL;
    }
    return pNtkNew;
}

/**Function*************************************************************

  Synopsis    [Checks whether the node can be processed in the trust mode.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkFraigTrustCheck( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pNode;
    int i, nFanins;
    Abc_NtkForEachNode( pNtk, pNode, i )
    {
        nFanins = Abc_ObjFaninNum(pNode);
        if ( nFanins < 2 )
            continue;
        if ( nFanins == 2 && Abc_SopIsAndType((char *)pNode->pData) )
            continue;
        if ( !Abc_SopIsOrType((char *)pNode->pData) )
            return 0;
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    [Interfaces the network with the FRAIG package.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkFraigTrustOne( Abc_Ntk_t * pNtk, Abc_Ntk_t * pNtkNew )
{
    ProgressBar * pProgress;
    Vec_Ptr_t * vNodes;
    Abc_Obj_t * pNode, * pNodeNew, * pObj;
    int i;

    // perform strashing
    vNodes = Abc_NtkDfs( pNtk, 0 );
    pProgress = Extra_ProgressBarStart( stdout, vNodes->nSize );
    Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pNode, i )
    {
        Extra_ProgressBarUpdate( pProgress, i, NULL );
        // get the node
        assert( Abc_ObjIsNode(pNode) );
         // strash the node
        pNodeNew = Abc_NodeFraigTrust( pNtkNew, pNode );
        // get the old object
        if ( Abc_NtkIsNetlist(pNtk) )
            pObj = Abc_ObjFanout0( pNode ); // the fanout net 
        else 
            pObj = pNode; // the node itself
        // make sure the node is not yet strashed
        assert( pObj->pCopy == NULL );
        // mark the old object with the new AIG node
        pObj->pCopy = pNodeNew;
    }
    Vec_PtrFree( vNodes );
    Extra_ProgressBarStop( pProgress );
}

/**Function*************************************************************

  Synopsis    [Transforms one node into a FRAIG in the trust mode.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Obj_t * Abc_NodeFraigTrust( Abc_Ntk_t * pNtkNew, Abc_Obj_t * pNode )
{
    Abc_Obj_t * pSum, * pFanin;
    void ** ppTail;
    int i, nFanins, fCompl;

    assert( Abc_ObjIsNode(pNode) );
    // get the number of node's fanins
    nFanins = Abc_ObjFaninNum( pNode );
    assert( nFanins == Abc_SopGetVarNum((char *)pNode->pData) );
    // check if it is a constant
    if ( nFanins == 0 )
        return Abc_ObjNotCond( Abc_AigConst1(pNtkNew), Abc_SopIsConst0((char *)pNode->pData) );
    if ( nFanins == 1 )
        return Abc_ObjNotCond( Abc_ObjFanin0(pNode)->pCopy, Abc_SopIsInv((char *)pNode->pData) );
    if ( nFanins == 2 && Abc_SopIsAndType((char *)pNode->pData) )
        return Abc_AigAnd( (Abc_Aig_t *)pNtkNew->pManFunc, 
            Abc_ObjNotCond( Abc_ObjFanin0(pNode)->pCopy, !Abc_SopGetIthCareLit((char *)pNode->pData,0) ),
            Abc_ObjNotCond( Abc_ObjFanin1(pNode)->pCopy, !Abc_SopGetIthCareLit((char *)pNode->pData,1) )  );
    assert( Abc_SopIsOrType((char *)pNode->pData) );
    fCompl = Abc_SopGetIthCareLit((char *)pNode->pData,0);
    // get the root of the choice node (the first fanin)
    pSum = Abc_ObjFanin0(pNode)->pCopy;
    // connect other fanins
    ppTail = &pSum->pData;
    Abc_ObjForEachFanin( pNode, pFanin, i )
    {
        if ( i == 0 )
            continue;
        *ppTail = pFanin->pCopy;
        ppTail = &pFanin->pCopy->pData;
        // set the complemented bit of this cut
        if ( fCompl ^ Abc_SopGetIthCareLit((char *)pNode->pData, i) )
            pFanin->pCopy->fPhase = 1;
    }
    assert( *ppTail == NULL );
    return pSum;
}




/**Function*************************************************************

  Synopsis    [Interfaces the network with the FRAIG package.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkFraigStore( Abc_Ntk_t * pNtkAdd )
{
    Vec_Ptr_t * vStore;
    Abc_Ntk_t * pNtk;
    // create the network to be stored
    pNtk = Abc_NtkStrash( pNtkAdd, 0, 0, 0 );
    if ( pNtk == NULL )
    {
        printf( "Abc_NtkFraigStore: Initial strashing has failed.\n" );
        return 0;
    }
    // get the network currently stored
    vStore = Abc_FrameReadStore();
    if ( Vec_PtrSize(vStore) > 0 )
    {
        // check that the networks have the same PIs
        extern int Abc_NodeCompareCiCo( Abc_Ntk_t * pNtkOld, Abc_Ntk_t * pNtkNew );
        if ( !Abc_NodeCompareCiCo(pNtk, (Abc_Ntk_t *)Vec_PtrEntry(vStore, 0)) )
        {
            // reorder PIs of pNtk2 according to pNtk1
            if ( !Abc_NtkCompareSignals( pNtk, (Abc_Ntk_t *)Vec_PtrEntry(vStore, 0), 1, 1 ) )
            {
                printf( "Trying to store the network with different primary inputs.\n" );
                printf( "The previously stored networks are deleted and this one is added.\n" );
                Abc_NtkFraigStoreClean();
            }
        }
    }
    Vec_PtrPush( vStore, pNtk );
//    printf( "The number of AIG nodes added to storage = %5d.\n", Abc_NtkNodeNum(pNtk) );
    return 1;
}

/**Function*************************************************************

  Synopsis    [Interfaces the network with the FRAIG package.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkFraigRestore( int nPatsRand, int nPatsDyna, int nBTLimit )
{
    extern Abc_Ntk_t * Abc_NtkFraigPartitioned( Vec_Ptr_t * vStore, void * pParams );
    Fraig_Params_t Params;
    Vec_Ptr_t * vStore;
    Abc_Ntk_t * pNtk, * pFraig;
    int nWords1, nWords2, nWordsMin;
//    abctime clk = Abc_Clock();

    // get the stored network
    vStore = Abc_FrameReadStore();
    if ( Vec_PtrSize(vStore) == 0 )
    {
        printf( "There are no network currently in storage.\n" );
        return NULL;
    }
//    printf( "Currently stored %d networks will be fraiged.\n", Vec_PtrSize(vStore) );
    pNtk = (Abc_Ntk_t *)Vec_PtrEntry( vStore, 0 );

    // swap the first and last network
    // this should lead to the primary choice being "better" because of synthesis
    if ( Vec_PtrSize(vStore) > 1 )
    {
        pNtk = (Abc_Ntk_t *)Vec_PtrPop( vStore );
        Vec_PtrPush( vStore, Vec_PtrEntry(vStore,0) );
        Vec_PtrWriteEntry( vStore, 0, pNtk );
    }

    // to determine the number of simulation patterns
    // use the following strategy
    // at least 64 words (32 words random and 32 words dynamic)
    // no more than 256M for one circuit (128M + 128M)
    nWords1 = 32;
    nWords2 = (1<<27) / (Abc_NtkNodeNum(pNtk) + Abc_NtkCiNum(pNtk));
    nWordsMin = Abc_MinInt( nWords1, nWords2 );

    // set parameters for fraiging
    Fraig_ParamsSetDefault( &Params );
    Params.nPatsRand  = nPatsRand ? nPatsRand : nWordsMin * 32;    // the number of words of random simulation info
    Params.nPatsDyna  = nPatsDyna ? nPatsDyna : nWordsMin * 32;    // the number of words of dynamic simulation info
    Params.nBTLimit   = nBTLimit;          // the max number of backtracks to perform
    Params.fFuncRed   =    1;              // performs only one level hashing
    Params.fFeedBack  =    1;              // enables solver feedback
    Params.fDist1Pats =    1;              // enables distance-1 patterns
    Params.fDoSparse  =    1;              // performs equiv tests for sparse functions 
    Params.fChoicing  =    1;              // enables recording structural choices
    Params.fTryProve  =    0;              // tries to solve the final miter
    Params.fInternal  =    1;              // does not show progress bar
    Params.fVerbose   =    0;              // the verbosiness flag

    // perform partitioned computation of structural choices
    pFraig = Abc_NtkFraigPartitioned( vStore, &Params );
    Abc_NtkFraigStoreClean();
//ABC_PRT( "Total choicing time", Abc_Clock() - clk );
    return pFraig;
}

/**Function*************************************************************

  Synopsis    [Interfaces the network with the FRAIG package.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkFraigStoreClean()
{
    Vec_Ptr_t * vStore;
    Abc_Ntk_t * pNtk;
    int i;
    vStore = Abc_FrameReadStore();
    Vec_PtrForEachEntry( Abc_Ntk_t *, vStore, pNtk, i )
        Abc_NtkDelete( pNtk );
    Vec_PtrClear( vStore );
}

/**Function*************************************************************

  Synopsis    [Checks the correctness of stored networks.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkFraigStoreCheck( Abc_Ntk_t * pFraig )
{
    Abc_Obj_t * pNode0, * pNode1;
    int nPoOrig, nPoFinal, nStored; 
    int i, k;
    // check that the PO functions are correct
    nPoFinal = Abc_NtkPoNum(pFraig);
    nStored  = Abc_FrameReadStoreSize();
    assert( nPoFinal % nStored == 0 );
    nPoOrig  = nPoFinal / nStored;
    for ( i = 0; i < nPoOrig; i++ )
    {
        pNode0 = Abc_ObjFanin0( Abc_NtkPo(pFraig, i) ); 
        for ( k = 1; k < nStored; k++ )
        {
            pNode1 = Abc_ObjFanin0( Abc_NtkPo(pFraig, k*nPoOrig+i) ); 
            if ( pNode0 != pNode1 )
                printf( "Verification for PO #%d of network #%d has failed. The PO function is not used.\n", i+1, k+1 );
        }
    }
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

