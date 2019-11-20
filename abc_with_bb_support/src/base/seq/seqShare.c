/**CFile****************************************************************

  FileName    [seqShare.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Construction and manipulation of sequential AIGs.]

  Synopsis    [Latch sharing at the fanout stems.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: seqShare.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "seqInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static void Seq_NodeShareFanouts( Abc_Obj_t * pNode, Vec_Ptr_t * vNodes );
static void Seq_NodeShareOne( Abc_Obj_t * pNode, Abc_InitType_t Init, Vec_Ptr_t * vNodes );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Transforms the sequential AIG to take fanout sharing into account.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Seq_NtkShareFanouts( Abc_Ntk_t * pNtk )
{
    Vec_Ptr_t * vNodes;
    Abc_Obj_t * pObj;
    int i;
    vNodes = Vec_PtrAlloc( 10 );
    // share the PI latches
    Abc_NtkForEachPi( pNtk, pObj, i )
        Seq_NodeShareFanouts( pObj, vNodes );
    // share the node latches
    Abc_NtkForEachNode( pNtk, pObj, i )
        Seq_NodeShareFanouts( pObj, vNodes );
    Vec_PtrFree( vNodes );
}

/**Function*************************************************************

  Synopsis    [Transforms the node to take fanout sharing into account.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Seq_NodeShareFanouts( Abc_Obj_t * pNode, Vec_Ptr_t * vNodes )
{
    Abc_Obj_t * pFanout;
    Abc_InitType_t Type;
    int nLatches[4], i;
    // skip the node with only one fanout
    if ( Abc_ObjFanoutNum(pNode) < 2 )
        return;
    // clean the the fanout counters
    for ( i = 0; i < 4; i++ )
        nLatches[i] = 0;
    // find the number of fanouts having latches of each type
    Abc_ObjForEachFanout( pNode, pFanout, i )  
    {
        if ( Seq_ObjFanoutL(pNode, pFanout) == 0 )
            continue;
        Type = Seq_NodeGetInitLast( pFanout, Abc_ObjFanoutEdgeNum(pNode, pFanout) );
        nLatches[Type]++;
    }
    // decide what to do
    if ( nLatches[ABC_INIT_ZERO] > 1 && nLatches[ABC_INIT_ONE] > 1 ) // 0-group and 1-group
    {
        Seq_NodeShareOne( pNode, ABC_INIT_ZERO, vNodes );     // shares 0 and DC
        Seq_NodeShareOne( pNode, ABC_INIT_ONE,  vNodes );     // shares 1 and DC
    }
    else if ( nLatches[ABC_INIT_ZERO] > 1 ) // 0-group
        Seq_NodeShareOne( pNode, ABC_INIT_ZERO, vNodes );     // shares 0 and DC
    else if ( nLatches[ABC_INIT_ONE] > 1 ) // 1-group
        Seq_NodeShareOne( pNode, ABC_INIT_ONE,  vNodes );     // shares 1 and DC
    else if ( nLatches[ABC_INIT_DC] > 1 ) // DC-group
    {
        if ( nLatches[ABC_INIT_ZERO] > 0 )
            Seq_NodeShareOne( pNode, ABC_INIT_ZERO, vNodes ); // shares 0 and DC
        else 
            Seq_NodeShareOne( pNode, ABC_INIT_ONE,  vNodes ); // shares 1 and DC
    }
}

/**Function*************************************************************

  Synopsis    [Transforms the node to take fanout sharing into account.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Seq_NodeShareOne( Abc_Obj_t * pNode, Abc_InitType_t Init, Vec_Ptr_t * vNodes )
{
    Vec_Int_t * vNums  = Seq_ObjLNums( pNode );
    Vec_Ptr_t * vInits = Seq_NodeLats( pNode );
    Abc_Obj_t * pFanout, * pBuffer;
    Abc_InitType_t Type, InitNew;
    int i;
    // collect the fanouts that satisfy the property (have initial value Init or DC)
    InitNew = ABC_INIT_DC;
    Vec_PtrClear( vNodes );
    Abc_ObjForEachFanout( pNode, pFanout, i )
    {
        if ( Seq_ObjFanoutL(pNode, pFanout) == 0 )
            continue;
        Type = Seq_NodeGetInitLast( pFanout, Abc_ObjFanoutEdgeNum(pNode, pFanout) );
        if ( Type == Init )
            InitNew = Init;
        if ( Type == Init || Type == ABC_INIT_DC )
        {
            Vec_PtrPush( vNodes, pFanout );
            Seq_NodeDeleteLast( pFanout, Abc_ObjFanoutEdgeNum(pNode, pFanout) );
        }
    }
    // create the new buffer
    pBuffer = Abc_NtkCreateNode( pNode->pNtk );
    Abc_ObjAddFanin( pBuffer, pNode );

    // grow storage for initial states
    Vec_PtrGrow( vInits, 2 * pBuffer->Id + 2 );
    for ( i = Vec_PtrSize(vInits); i < 2 * (int)pBuffer->Id + 2; i++ )
        Vec_PtrPush( vInits, NULL );
    // grow storage for numbers of latches
    Vec_IntGrow( vNums, 2 * pBuffer->Id + 2 );
    for ( i = Vec_IntSize(vNums); i < 2 * (int)pBuffer->Id + 2; i++ )
        Vec_IntPush( vNums, 0 );
    // insert the new latch
    Seq_NodeInsertFirst( pBuffer, 0, InitNew );

    // redirect the fanouts
    Vec_PtrForEachEntry( vNodes, pFanout, i )
        Abc_ObjPatchFanin( pFanout, pNode, pBuffer );
}





/**Function*************************************************************

  Synopsis    [Maps virtual latches into real latches.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline unsigned Seq_NtkShareLatchesKey( Abc_Obj_t * pObj, Abc_InitType_t Init )
{
    return (pObj->Id << 2) | Init;
}

/**Function*************************************************************

  Synopsis    [Maps virtual latches into real latches.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Obj_t * Seq_NtkShareLatches_rec( Abc_Ntk_t * pNtk, Abc_Obj_t * pObj, Seq_Lat_t * pRing, int nLatch, stmm_table * tLatchMap )
{
    Abc_Obj_t * pLatch, * pFanin;
    Abc_InitType_t Init;
    unsigned Key;
    if ( nLatch == 0 )
        return pObj;
    assert( pRing->pLatch == NULL );
    // get the latch on the previous level
    pFanin = Seq_NtkShareLatches_rec( pNtk, pObj, Seq_LatNext(pRing), nLatch - 1, tLatchMap );

    // get the initial state
    Init = Seq_LatInit( pRing );
    // check if the latch with this initial state exists
    Key = Seq_NtkShareLatchesKey( pFanin, Init );
    if ( stmm_lookup( tLatchMap, (char *)Key, (char **)&pLatch ) )
        return pRing->pLatch = pLatch;

    // does not exist
    if ( Init != ABC_INIT_DC )
    {
        // check if the don't-care exists
        Key = Seq_NtkShareLatchesKey( pFanin, ABC_INIT_DC );
        if ( stmm_lookup( tLatchMap, (char *)Key, (char **)&pLatch ) ) // yes
        {
            // update the table
            stmm_delete( tLatchMap, (char **)&Key, (char **)&pLatch );
            Key = Seq_NtkShareLatchesKey( pFanin, Init );
            stmm_insert( tLatchMap, (char *)Key, (char *)pLatch );
            // change don't-care to the given value
            pLatch->pData = (void *)Init;
            return pRing->pLatch = pLatch;
        }

        // add the latch with this value
        pLatch = Abc_NtkCreateLatch( pNtk );
        pLatch->pData = (void *)Init;
        Abc_ObjAddFanin( pLatch, pFanin );
        // add it to the table
        Key = Seq_NtkShareLatchesKey( pFanin, Init );
        stmm_insert( tLatchMap, (char *)Key, (char *)pLatch );
        return pRing->pLatch = pLatch;
    }
    // the init value is the don't-care

    // check if care values exist
    Key = Seq_NtkShareLatchesKey( pFanin, ABC_INIT_ZERO );
    if ( stmm_lookup( tLatchMap, (char *)Key, (char **)&pLatch ) )
    {
        Seq_LatSetInit( pRing, ABC_INIT_ZERO );
        return pRing->pLatch = pLatch;
    }
    Key = Seq_NtkShareLatchesKey( pFanin, ABC_INIT_ONE );
    if ( stmm_lookup( tLatchMap, (char *)Key, (char **)&pLatch ) )
    {
        Seq_LatSetInit( pRing, ABC_INIT_ONE );
        return pRing->pLatch = pLatch;
    }

    // create the don't-care latch
    pLatch = Abc_NtkCreateLatch( pNtk );
    pLatch->pData = (void *)ABC_INIT_DC;
    Abc_ObjAddFanin( pLatch, pFanin );
    // add it to the table
    Key = Seq_NtkShareLatchesKey( pFanin, ABC_INIT_DC );
    stmm_insert( tLatchMap, (char *)Key, (char *)pLatch );
    return pRing->pLatch = pLatch;
}

/**Function*************************************************************

  Synopsis    [Maps virtual latches into real latches.]

  Description [Creates new latches and assigns them to virtual latches
  on the edges of a sequential AIG. The nodes of the new network should
  be created before this procedure is called.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Seq_NtkShareLatches( Abc_Ntk_t * pNtkNew, Abc_Ntk_t * pNtk )
{ 
    Abc_Obj_t * pObj, * pFanin;
    stmm_table * tLatchMap;
    int i;
    assert( Abc_NtkIsSeq( pNtk ) );
    tLatchMap = stmm_init_table( stmm_ptrcmp, stmm_ptrhash );
    Abc_AigForEachAnd( pNtk, pObj, i )
    {
        pFanin = Abc_ObjFanin0(pObj);
        Seq_NtkShareLatches_rec( pNtkNew, pFanin->pCopy, Seq_NodeGetRing(pObj,0), Seq_NodeCountLats(pObj,0), tLatchMap );
        pFanin = Abc_ObjFanin1(pObj);
        Seq_NtkShareLatches_rec( pNtkNew, pFanin->pCopy, Seq_NodeGetRing(pObj,1), Seq_NodeCountLats(pObj,1), tLatchMap );
    }
    Abc_NtkForEachPo( pNtk, pObj, i )
        Seq_NtkShareLatches_rec( pNtkNew, Abc_ObjFanin0(pObj)->pCopy, Seq_NodeGetRing(pObj,0), Seq_NodeCountLats(pObj,0), tLatchMap );
    stmm_free_table( tLatchMap );
}

/**Function*************************************************************

  Synopsis    [Maps virtual latches into real latches.]

  Description [Creates new latches and assigns them to virtual latches
  on the edges of a sequential AIG. The nodes of the new network should
  be created before this procedure is called.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Seq_NtkShareLatchesMapping( Abc_Ntk_t * pNtkNew, Abc_Ntk_t * pNtk, Vec_Ptr_t * vMapAnds, int fFpga )
{ 
    Seq_Match_t * pMatch;
    Abc_Obj_t * pObj, * pFanout;
    stmm_table * tLatchMap;
    Vec_Ptr_t * vNodes;
    int i, k;
    assert( Abc_NtkIsSeq( pNtk ) );

    // start the table
    tLatchMap = stmm_init_table( stmm_ptrcmp, stmm_ptrhash );

    // create the array of all nodes with sharable fanouts
    vNodes = Vec_PtrAlloc( 100 );
    Vec_PtrPush( vNodes, Abc_AigConst1(pNtk) );
    Abc_NtkForEachPi( pNtk, pObj, i )
        Vec_PtrPush( vNodes, pObj );
    if ( fFpga )
    {
        Vec_PtrForEachEntry( vMapAnds, pObj, i )
            Vec_PtrPush( vNodes, pObj );
    }
    else 
    {
        Vec_PtrForEachEntry( vMapAnds, pMatch, i )
            Vec_PtrPush( vNodes, pMatch->pAnd );
    }

    // process nodes used in the mapping
    Vec_PtrForEachEntry( vNodes, pObj, i )
    {
        // make sure the label is clean
        Abc_ObjForEachFanout( pObj, pFanout, k )
            assert( pFanout->fMarkC == 0 );
        Abc_ObjForEachFanout( pObj, pFanout, k )
        {
            if ( pFanout->fMarkC )
                continue;
            pFanout->fMarkC = 1;
            if ( Abc_ObjFaninId0(pFanout) == pObj->Id )
                Seq_NtkShareLatches_rec( pNtkNew, pObj->pCopy, Seq_NodeGetRing(pFanout,0), Seq_NodeCountLats(pFanout,0), tLatchMap );
            if ( Abc_ObjFaninId1(pFanout) == pObj->Id )
                Seq_NtkShareLatches_rec( pNtkNew, pObj->pCopy, Seq_NodeGetRing(pFanout,1), Seq_NodeCountLats(pFanout,1), tLatchMap );
        }
        // clean the label
        Abc_ObjForEachFanout( pObj, pFanout, k )
            pFanout->fMarkC = 0;
    }
    stmm_free_table( tLatchMap );
    // return to the old array
    Vec_PtrFree( vNodes );
}

/**Function*************************************************************

  Synopsis    [Clean the latches after sharing them.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Seq_NtkShareLatchesClean( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pObj;
    int i;
    assert( Abc_NtkIsSeq( pNtk ) );
    Abc_AigForEachAnd( pNtk, pObj, i )
    {
        Seq_NodeCleanLats( pObj, 0 );
        Seq_NodeCleanLats( pObj, 1 );
    }
    Abc_NtkForEachPo( pNtk, pObj, i )
        Seq_NodeCleanLats( pObj, 0 );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
