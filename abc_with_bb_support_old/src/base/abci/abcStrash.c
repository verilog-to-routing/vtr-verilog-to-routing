/**CFile****************************************************************

  FileName    [abcStrash.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Network and node package.]

  Synopsis    [Strashing of the current network.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: abcStrash.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "abc.h"
#include "extra.h"
#include "dec.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static void Abc_NtkStrashPerform( Abc_Ntk_t * pNtk, Abc_Ntk_t * pNtkNew, int fAllNodes, int fRecord );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Reapplies structural hashing to the AIG.]

  Description [Because of the structural hashing, this procedure should not 
  change the number of nodes. It is useful to detect the bugs in the original AIG.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkRestrash( Abc_Ntk_t * pNtk, bool fCleanup )
{
    extern int timeRetime;
    Abc_Ntk_t * pNtkAig;
    Abc_Obj_t * pObj;
    int i, nNodes;//, RetValue;
    assert( Abc_NtkIsStrash(pNtk) );
//timeRetime = clock();
    // print warning about choice nodes
    if ( Abc_NtkGetChoiceNum( pNtk ) )
        printf( "Warning: The choice nodes in the original AIG are removed by strashing.\n" );
    // start the new network (constants and CIs of the old network will point to the their counterparts in the new network)
    pNtkAig = Abc_NtkStartFrom( pNtk, ABC_NTK_STRASH, ABC_FUNC_AIG );
    // restrash the nodes (assuming a topological order of the old network)
    Abc_NtkForEachNode( pNtk, pObj, i )
        pObj->pCopy = Abc_AigAnd( pNtkAig->pManFunc, Abc_ObjChild0Copy(pObj), Abc_ObjChild1Copy(pObj) );
    // finalize the network
    Abc_NtkFinalize( pNtk, pNtkAig );
    // print warning about self-feed latches
//    if ( Abc_NtkCountSelfFeedLatches(pNtkAig) )
//        printf( "Warning: The network has %d self-feeding latches.\n", Abc_NtkCountSelfFeedLatches(pNtkAig) );
    // perform cleanup if requested
    if ( fCleanup && (nNodes = Abc_AigCleanup(pNtkAig->pManFunc)) ) 
        printf( "Abc_NtkRestrash(): AIG cleanup removed %d nodes (this is a bug).\n", nNodes );
    // duplicate EXDC 
    if ( pNtk->pExdc )
        pNtkAig->pExdc = Abc_NtkDup( pNtk->pExdc );
    // make sure everything is okay
    if ( !Abc_NtkCheck( pNtkAig ) )
    {
        printf( "Abc_NtkStrash: The network check has failed.\n" );
        Abc_NtkDelete( pNtkAig );
        return NULL;
    }
//timeRetime = clock() - timeRetime;
//    if ( RetValue = Abc_NtkRemoveSelfFeedLatches(pNtkAig) )
//        printf( "Modified %d self-feeding latches. The result will not verify.\n", RetValue );
    return pNtkAig;

}

/**Function*************************************************************

  Synopsis    [Reapplies structural hashing to the AIG.]

  Description [Because of the structural hashing, this procedure should not 
  change the number of nodes. It is useful to detect the bugs in the original AIG.]
               
  SideEffects []
 
  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkRestrashZero( Abc_Ntk_t * pNtk, bool fCleanup )
{
    extern int timeRetime;
    Abc_Ntk_t * pNtkAig;
    Abc_Obj_t * pObj;
    int i, nNodes;//, RetValue;
    assert( Abc_NtkIsStrash(pNtk) );
//timeRetime = clock();
    // print warning about choice nodes
    if ( Abc_NtkGetChoiceNum( pNtk ) )
        printf( "Warning: The choice nodes in the original AIG are removed by strashing.\n" );
    // start the new network (constants and CIs of the old network will point to the their counterparts in the new network)
    pNtkAig = Abc_NtkStartFrom( pNtk, ABC_NTK_STRASH, ABC_FUNC_AIG );
    // complement the 1-values registers
    Abc_NtkForEachLatch( pNtk, pObj, i )
        if ( Abc_LatchIsInit1(pObj) )
            Abc_ObjFanout0(pObj)->pCopy = Abc_ObjNot(Abc_ObjFanout0(pObj)->pCopy);
    // restrash the nodes (assuming a topological order of the old network)
    Abc_NtkForEachNode( pNtk, pObj, i )
        pObj->pCopy = Abc_AigAnd( pNtkAig->pManFunc, Abc_ObjChild0Copy(pObj), Abc_ObjChild1Copy(pObj) );
    // finalize the network
    Abc_NtkFinalize( pNtk, pNtkAig );
    // complement the 1-valued registers
    Abc_NtkForEachLatch( pNtkAig, pObj, i )
        if ( Abc_LatchIsInit1(pObj) )
            Abc_ObjXorFaninC( Abc_ObjFanin0(pObj), 0 );
    // set all constant-0 values
    Abc_NtkForEachLatch( pNtkAig, pObj, i )
        Abc_LatchSetInit0( pObj );

    // print warning about self-feed latches
//    if ( Abc_NtkCountSelfFeedLatches(pNtkAig) )
//        printf( "Warning: The network has %d self-feeding latches.\n", Abc_NtkCountSelfFeedLatches(pNtkAig) );
    // perform cleanup if requested
    if ( fCleanup && (nNodes = Abc_AigCleanup(pNtkAig->pManFunc)) ) 
        printf( "Abc_NtkRestrash(): AIG cleanup removed %d nodes (this is a bug).\n", nNodes );
    // duplicate EXDC 
    if ( pNtk->pExdc )
        pNtkAig->pExdc = Abc_NtkDup( pNtk->pExdc );
    // make sure everything is okay
    if ( !Abc_NtkCheck( pNtkAig ) )
    {
        printf( "Abc_NtkStrash: The network check has failed.\n" );
        Abc_NtkDelete( pNtkAig );
        return NULL;
    }
//timeRetime = clock() - timeRetime;
//    if ( RetValue = Abc_NtkRemoveSelfFeedLatches(pNtkAig) )
//        printf( "Modified %d self-feeding latches. The result will not verify.\n", RetValue );
    return pNtkAig;

}

/**Function*************************************************************

  Synopsis    [Transforms logic network into structurally hashed AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkStrash( Abc_Ntk_t * pNtk, int fAllNodes, int fCleanup, int fRecord )
{
    Abc_Ntk_t * pNtkAig;
    int nNodes;
    assert( Abc_NtkIsLogic(pNtk) || Abc_NtkIsStrash(pNtk) );
    // consider the special case when the network is already structurally hashed
    if ( Abc_NtkIsStrash(pNtk) )
        return Abc_NtkRestrash( pNtk, fCleanup );
    // convert the node representation in the logic network to the AIG form
    if ( !Abc_NtkToAig(pNtk) )
    {
        printf( "Converting to AIGs has failed.\n" );
        return NULL;
    }
    // perform strashing
//    Abc_NtkCleanCopy( pNtk );
    pNtkAig = Abc_NtkStartFrom( pNtk, ABC_NTK_STRASH, ABC_FUNC_AIG );
    Abc_NtkStrashPerform( pNtk, pNtkAig, fAllNodes, fRecord );
    Abc_NtkFinalize( pNtk, pNtkAig );
    // print warning about self-feed latches
//    if ( Abc_NtkCountSelfFeedLatches(pNtkAig) )
//        printf( "Warning: The network has %d self-feeding latches.\n", Abc_NtkCountSelfFeedLatches(pNtkAig) );
    // perform cleanup if requested
    nNodes = fCleanup? Abc_AigCleanup(pNtkAig->pManFunc) : 0;
//    if ( nNodes )
//        printf( "Warning: AIG cleanup removed %d nodes (this is not a bug).\n", nNodes );
    // duplicate EXDC 
    if ( pNtk->pExdc )
        pNtkAig->pExdc = Abc_NtkDup( pNtk->pExdc );
    // make sure everything is okay
    if ( !Abc_NtkCheck( pNtkAig ) )
    {
        printf( "Abc_NtkStrash: The network check has failed.\n" );
        Abc_NtkDelete( pNtkAig );
        return NULL;
    }
    return pNtkAig;
}

/**Function*************************************************************

  Synopsis    [Appends the second network to the first.]

  Description [Modifies the first network by adding the logic of the second. 
  Performs structural hashing while appending the networks. Does not change 
  the second network. Returns 0 if the appending failed, 1 otherise.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkAppend( Abc_Ntk_t * pNtk1, Abc_Ntk_t * pNtk2, int fAddPos )
{
    Abc_Obj_t * pObj;
    char * pName;
    int i, nNewCis;
    // the first network should be an AIG
    assert( Abc_NtkIsStrash(pNtk1) );
    assert( Abc_NtkIsLogic(pNtk2) || Abc_NtkIsStrash(pNtk2) ); 
    if ( Abc_NtkIsLogic(pNtk2) && !Abc_NtkToAig(pNtk2) )
    {
        printf( "Converting to AIGs has failed.\n" );
        return 0;
    }
    // check that the networks have the same PIs
    // reorder PIs of pNtk2 according to pNtk1
    if ( !Abc_NtkCompareSignals( pNtk1, pNtk2, 1, 1 ) )
        printf( "Abc_NtkAppend(): The union of the network PIs is computed (warning).\n" );
    // perform strashing
    nNewCis = 0;
    Abc_NtkCleanCopy( pNtk2 );
    Abc_AigConst1(pNtk2)->pCopy = Abc_AigConst1(pNtk1);
    Abc_NtkForEachCi( pNtk2, pObj, i )
    {
        pName = Abc_ObjName(pObj);
        pObj->pCopy = Abc_NtkFindCi(pNtk1, Abc_ObjName(pObj));
        if ( pObj->pCopy == NULL )
        {
            pObj->pCopy = Abc_NtkDupObj(pNtk1, pObj, 1);
            nNewCis++;
        }
    }
    if ( nNewCis )
        printf( "Warning: Procedure Abc_NtkAppend() added %d new CIs.\n", nNewCis );
    // add pNtk2 to pNtk1 while strashing
    if ( Abc_NtkIsLogic(pNtk2) )
        Abc_NtkStrashPerform( pNtk2, pNtk1, 1, 0 );
    else
        Abc_NtkForEachNode( pNtk2, pObj, i )
            pObj->pCopy = Abc_AigAnd( pNtk1->pManFunc, Abc_ObjChild0Copy(pObj), Abc_ObjChild1Copy(pObj) );
    // add the COs of the second network
    if ( fAddPos )
    {
        Abc_NtkForEachPo( pNtk2, pObj, i )
        {
            Abc_NtkDupObj( pNtk1, pObj, 0 );
            Abc_ObjAddFanin( pObj->pCopy, Abc_ObjChild0Copy(pObj) );
            Abc_ObjAssignName( pObj->pCopy, Abc_ObjName(pObj->pCopy), NULL );
        }
    }
    else
    {
        Abc_Obj_t * pObjOld, * pDriverOld, * pDriverNew;
        int fCompl, iNodeId;
        // OR the choices
        Abc_NtkForEachCo( pNtk2, pObj, i )
        {
            iNodeId = Nm_ManFindIdByNameTwoTypes( pNtk1->pManName, Abc_ObjName(pObj), ABC_OBJ_PO, ABC_OBJ_BI );
            assert( iNodeId >= 0 );
            pObjOld = Abc_NtkObj( pNtk1, iNodeId );
            // derive the new driver
            pDriverOld = Abc_ObjChild0( pObjOld );
            pDriverNew = Abc_ObjChild0Copy( pObj );
            pDriverNew = Abc_AigOr( pNtk1->pManFunc, pDriverOld, pDriverNew );
            if ( Abc_ObjRegular(pDriverOld) == Abc_ObjRegular(pDriverNew) )
                continue;
            // replace the old driver by the new driver
            fCompl = Abc_ObjRegular(pDriverOld)->fPhase ^ Abc_ObjRegular(pDriverNew)->fPhase;
            Abc_ObjPatchFanin( pObjOld, Abc_ObjRegular(pDriverOld), Abc_ObjNotCond(Abc_ObjRegular(pDriverNew), fCompl) );
        }
    }
    // make sure that everything is okay
    if ( !Abc_NtkCheck( pNtk1 ) )
    {
        printf( "Abc_NtkAppend: The network check has failed.\n" );
        return 0;
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    [Prepares the network for strashing.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkStrashPerform( Abc_Ntk_t * pNtkOld, Abc_Ntk_t * pNtkNew, int fAllNodes, int fRecord )
{
    ProgressBar * pProgress;
    Vec_Ptr_t * vNodes;
    Abc_Obj_t * pNodeOld;
    int i, clk = clock();
    assert( Abc_NtkIsLogic(pNtkOld) );
    assert( Abc_NtkIsStrash(pNtkNew) );
//    vNodes = Abc_NtkDfs( pNtkOld, fAllNodes );
    vNodes = Abc_NtkDfsIter( pNtkOld, fAllNodes );
//printf( "Nodes = %d. ", Vec_PtrSize(vNodes) );
//PRT( "Time", clock() - clk );
    pProgress = Extra_ProgressBarStart( stdout, vNodes->nSize );
    Vec_PtrForEachEntry( vNodes, pNodeOld, i )
    {
        Extra_ProgressBarUpdate( pProgress, i, NULL );
        pNodeOld->pCopy = Abc_NodeStrash( pNtkNew, pNodeOld, fRecord );
    }
    Extra_ProgressBarStop( pProgress );
    Vec_PtrFree( vNodes );
}

/**Function*************************************************************

  Synopsis    [Transfers the AIG from one manager into another.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NodeStrash_rec( Abc_Aig_t * pMan, Hop_Obj_t * pObj )
{
    assert( !Hop_IsComplement(pObj) );
    if ( !Hop_ObjIsNode(pObj) || Hop_ObjIsMarkA(pObj) )
        return;
    Abc_NodeStrash_rec( pMan, Hop_ObjFanin0(pObj) ); 
    Abc_NodeStrash_rec( pMan, Hop_ObjFanin1(pObj) );
    pObj->pData = Abc_AigAnd( pMan, (Abc_Obj_t *)Hop_ObjChild0Copy(pObj), (Abc_Obj_t *)Hop_ObjChild1Copy(pObj) );
    assert( !Hop_ObjIsMarkA(pObj) ); // loop detection
    Hop_ObjSetMarkA( pObj );
}

/**Function*************************************************************

  Synopsis    [Strashes one logic node.]

  Description [Assume the network is in the AIG form]
               
  SideEffects []
 
  SeeAlso     []

***********************************************************************/
Abc_Obj_t * Abc_NodeStrash( Abc_Ntk_t * pNtkNew, Abc_Obj_t * pNodeOld, int fRecord )
{
    Hop_Man_t * pMan;
    Hop_Obj_t * pRoot;
    Abc_Obj_t * pFanin;
    int i;
    assert( Abc_ObjIsNode(pNodeOld) );
    assert( Abc_NtkHasAig(pNodeOld->pNtk) && !Abc_NtkIsStrash(pNodeOld->pNtk) );
    // get the local AIG manager and the local root node
    pMan = pNodeOld->pNtk->pManFunc;
    pRoot = pNodeOld->pData;
    // check the constant case
    if ( Abc_NodeIsConst(pNodeOld) || Hop_Regular(pRoot) == Hop_ManConst1(pMan) )
        return Abc_ObjNotCond( Abc_AigConst1(pNtkNew), Hop_IsComplement(pRoot) );
    // perform special case-strashing using the record of AIG subgraphs
    if ( fRecord && Abc_NtkRecIsRunning() && Abc_ObjFaninNum(pNodeOld) > 2 && Abc_ObjFaninNum(pNodeOld) <= Abc_NtkRecVarNum() )
    {
        extern Vec_Int_t * Abc_NtkRecMemory();
        extern int Abc_NtkRecStrashNode( Abc_Ntk_t * pNtkNew, Abc_Obj_t * pObj, unsigned * pTruth, int nVars );
        int nVars = Abc_NtkRecVarNum();
        Vec_Int_t * vMemory = Abc_NtkRecMemory();
        unsigned * pTruth = Abc_ConvertAigToTruth( pMan, Hop_Regular(pRoot), nVars, vMemory, 0 );
        assert( Extra_TruthSupportSize(pTruth, nVars) == Abc_ObjFaninNum(pNodeOld) ); // should be swept
        if ( Hop_IsComplement(pRoot) )
            Extra_TruthNot( pTruth, pTruth, nVars );
        if ( Abc_NtkRecStrashNode( pNtkNew, pNodeOld, pTruth, nVars ) )
            return pNodeOld->pCopy;
    }
    // set elementary variables
    Abc_ObjForEachFanin( pNodeOld, pFanin, i )
        Hop_IthVar(pMan, i)->pData = pFanin->pCopy;
    // strash the AIG of this node
    Abc_NodeStrash_rec( pNtkNew->pManFunc, Hop_Regular(pRoot) );
    Hop_ConeUnmark_rec( Hop_Regular(pRoot) );
    // return the final node
    return Abc_ObjNotCond( Hop_Regular(pRoot)->pData, Hop_IsComplement(pRoot) );
}







/**Function*************************************************************

  Synopsis    [Copies the topmost levels of the network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Obj_t * Abc_NtkTopmost_rec( Abc_Ntk_t * pNtkNew, Abc_Obj_t * pNode, int LevelCut )
{
    assert( !Abc_ObjIsComplement(pNode) );
    if ( pNode->pCopy )
        return pNode->pCopy;
    if ( pNode->Level <= (unsigned)LevelCut )
        return pNode->pCopy = Abc_NtkCreatePi( pNtkNew );
    Abc_NtkTopmost_rec( pNtkNew, Abc_ObjFanin0(pNode), LevelCut );
    Abc_NtkTopmost_rec( pNtkNew, Abc_ObjFanin1(pNode), LevelCut );
    return pNode->pCopy = Abc_AigAnd( pNtkNew->pManFunc, Abc_ObjChild0Copy(pNode), Abc_ObjChild1Copy(pNode) );
}

/**Function*************************************************************

  Synopsis    [Copies the topmost levels of the network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkTopmost( Abc_Ntk_t * pNtk, int nLevels )
{
    Abc_Ntk_t * pNtkNew;
    Abc_Obj_t * pObjNew, * pPoNew;
    int LevelCut;
    assert( Abc_NtkIsStrash(pNtk) );
    assert( Abc_NtkCoNum(pNtk) == 1 );
    // get the cutoff level
    LevelCut = ABC_MAX( 0, Abc_AigLevel(pNtk) - nLevels );
    // start the network
    pNtkNew = Abc_NtkAlloc( ABC_NTK_STRASH, ABC_FUNC_AIG, 1 );
    pNtkNew->pName = Extra_UtilStrsav(pNtk->pName);
    Abc_AigConst1(pNtk)->pCopy = Abc_AigConst1(pNtkNew);
    // create PIs below the cut and nodes above the cut
    Abc_NtkCleanCopy( pNtk );
    pObjNew = Abc_NtkTopmost_rec( pNtkNew, Abc_ObjFanin0(Abc_NtkPo(pNtk, 0)), LevelCut );
    pObjNew = Abc_ObjNotCond( pObjNew, Abc_ObjFaninC0(Abc_NtkPo(pNtk, 0)) );
    // add the PO node and name
    pPoNew = Abc_NtkCreatePo(pNtkNew);
    Abc_ObjAddFanin( pPoNew, pObjNew );
    Abc_NtkAddDummyPiNames( pNtkNew );
    Abc_ObjAssignName( pPoNew, Abc_ObjName(Abc_NtkPo(pNtk, 0)), NULL );
    // make sure everything is okay
    if ( !Abc_NtkCheck( pNtkNew ) )
    {
        printf( "Abc_NtkTopmost: The network check has failed.\n" );
        Abc_NtkDelete( pNtkNew );
        return NULL;
    }
    return pNtkNew;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


