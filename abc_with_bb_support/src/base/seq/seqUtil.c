/**CFile****************************************************************

  FileName    [seqUtil.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Construction and manipulation of sequential AIGs.]

  Synopsis    [Various utilities working with sequential AIGs.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: seqUtil.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "seqInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Returns the maximum latch number on any of the fanouts.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Seq_NtkLevelMax( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pNode;
    int i, Result;
    assert( Abc_NtkIsSeq(pNtk) );
    Result = 0;
    Abc_NtkForEachPo( pNtk, pNode, i )
    {
        pNode = Abc_ObjFanin0(pNode);
        if ( Result < (int)pNode->Level )
            Result = pNode->Level;
    }
    Abc_SeqForEachCutsetNode( pNtk, pNode, i )
    {
        if ( Result < (int)pNode->Level )
            Result = pNode->Level;
    }
    return Result;
}

/**Function*************************************************************

  Synopsis    [Returns the maximum latch number on any of the fanouts.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Seq_ObjFanoutLMax( Abc_Obj_t * pObj )
{
    Abc_Obj_t * pFanout;
    int i, nLatchCur, nLatchRes;
    if ( Abc_ObjFanoutNum(pObj) == 0 )
        return 0;
    nLatchRes = 0;
    Abc_ObjForEachFanout( pObj, pFanout, i )
    {
        nLatchCur = Seq_ObjFanoutL(pObj, pFanout);
        if ( nLatchRes < nLatchCur )
            nLatchRes = nLatchCur;
    }
    assert( nLatchRes >= 0 );
    return nLatchRes;
}

/**Function*************************************************************

  Synopsis    [Returns the minimum latch number on any of the fanouts.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Seq_ObjFanoutLMin( Abc_Obj_t * pObj )
{
    Abc_Obj_t * pFanout;
    int i, nLatchCur, nLatchRes;
    if ( Abc_ObjFanoutNum(pObj) == 0 )
        return 0;
    nLatchRes = ABC_INFINITY;
    Abc_ObjForEachFanout( pObj, pFanout, i )
    {
        nLatchCur = Seq_ObjFanoutL(pObj, pFanout);
        if ( nLatchRes > nLatchCur )
            nLatchRes = nLatchCur;
    }
    assert( nLatchRes < ABC_INFINITY );
    return nLatchRes;
}

/**Function*************************************************************

  Synopsis    [Returns the sum of latches on the fanout edges.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Seq_ObjFanoutLSum( Abc_Obj_t * pObj )
{
    Abc_Obj_t * pFanout;
    int i, nSum = 0;
    Abc_ObjForEachFanout( pObj, pFanout, i )
        nSum += Seq_ObjFanoutL(pObj, pFanout);
    return nSum;
}

/**Function*************************************************************

  Synopsis    [Returns the sum of latches on the fanin edges.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Seq_ObjFaninLSum( Abc_Obj_t * pObj )
{
    Abc_Obj_t * pFanin;
    int i, nSum = 0;
    Abc_ObjForEachFanin( pObj, pFanin, i )
        nSum += Seq_ObjFaninL(pObj, i);
    return nSum;
}

/**Function*************************************************************

  Synopsis    [Generates the printable edge label with the initial state.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Seq_ObjFaninGetInitPrintable( Abc_Obj_t * pObj, int Edge )
{
    static char Buffer[1000];
    Abc_InitType_t Init;
    int nLatches, i;
    nLatches = Seq_ObjFaninL( pObj, Edge );
    for ( i = 0; i < nLatches; i++ )
    {
        Init = Seq_LatInit( Seq_NodeGetLat(pObj, Edge, i) );
        if ( Init == ABC_INIT_NONE )
            Buffer[i] = '_';
        else if ( Init == ABC_INIT_ZERO )
            Buffer[i] = '0';
        else if ( Init == ABC_INIT_ONE )
            Buffer[i] = '1';
        else if ( Init == ABC_INIT_DC )
            Buffer[i] = 'x';
        else assert( 0 );
    }
    Buffer[nLatches] = 0;
    return Buffer;
}

/**Function*************************************************************

  Synopsis    [Sets the given value to all the latches of the edge.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Seq_NodeLatchSetValues( Abc_Obj_t * pObj, int Edge, Abc_InitType_t Init )
{ 
    Seq_Lat_t * pLat, * pRing;
    int c; 
    pRing = Seq_NodeGetRing(pObj, Edge);
    if ( pRing == NULL ) 
        return; 
    for ( c = 0, pLat = pRing; !c || pLat != pRing; c++, pLat = pLat->pNext )
        Seq_LatSetInit( pLat, Init );
}

/**Function*************************************************************

  Synopsis    [Sets the given value to all the latches of the edge.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Seq_NtkLatchSetValues( Abc_Ntk_t * pNtk, Abc_InitType_t Init )
{ 
    Abc_Obj_t * pObj;
    int i;
    assert( Abc_NtkIsSeq( pNtk ) );
    Abc_NtkForEachPo( pNtk, pObj, i )
        Seq_NodeLatchSetValues( pObj, 0, Init );
    Abc_NtkForEachNode( pNtk, pObj, i )
    {
        Seq_NodeLatchSetValues( pObj, 0, Init );
        Seq_NodeLatchSetValues( pObj, 1, Init );
    }
}


/**Function*************************************************************

  Synopsis    [Counts the number of latches in the sequential AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Seq_NtkLatchNum( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pObj;
    int i, Counter;
    assert( Abc_NtkIsSeq( pNtk ) );
    Counter = 0;
    Abc_NtkForEachNode( pNtk, pObj, i )
        Counter += Seq_ObjFaninLSum( pObj );
    Abc_NtkForEachPo( pNtk, pObj, i )
        Counter += Seq_ObjFaninLSum( pObj );
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Counts the number of latches in the sequential AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Seq_NtkLatchNumMax( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pObj;
    int i, Max, Cur;
    assert( Abc_NtkIsSeq( pNtk ) );
    Max = 0;
    Abc_AigForEachAnd( pNtk, pObj, i )
    {
        Cur = Seq_ObjFaninLMax( pObj );
        if ( Max < Cur )
            Max = Cur;
    }
    Abc_NtkForEachPo( pNtk, pObj, i )
    {
        Cur = Seq_ObjFaninL0( pObj );
        if ( Max < Cur )
            Max = Cur;
    }
    return Max;
}

/**Function*************************************************************

  Synopsis    [Counts the number of latches in the sequential AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Seq_NtkLatchNumShared( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pObj;
    int i, Counter;
    assert( Abc_NtkIsSeq( pNtk ) );
    Counter = 0;
    Abc_NtkForEachPi( pNtk, pObj, i )
        Counter += Seq_ObjFanoutLMax( pObj );
    Abc_NtkForEachNode( pNtk, pObj, i )
        Counter += Seq_ObjFanoutLMax( pObj );
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Counts the number of latches in the sequential AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Seq_ObjLatchGetInitNums( Abc_Obj_t * pObj, int Edge, int * pInits )
{
    Abc_InitType_t Init;
    int nLatches, i;
    nLatches = Seq_ObjFaninL( pObj, Edge );
    for ( i = 0; i < nLatches; i++ )
    {
        Init = Seq_NodeGetInitOne( pObj, Edge, i );
        pInits[Init]++;
    }
}

/**Function*************************************************************

  Synopsis    [Counts the number of latches in the sequential AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Seq_NtkLatchGetInitNums( Abc_Ntk_t * pNtk, int * pInits )
{
    Abc_Obj_t * pObj;
    int i;
    assert( Abc_NtkIsSeq( pNtk ) );
    for ( i = 0; i < 4; i++ )    
        pInits[i] = 0;
    Abc_NtkForEachPo( pNtk, pObj, i )
        Seq_ObjLatchGetInitNums( pObj, 0, pInits );
    Abc_NtkForEachNode( pNtk, pObj, i )
    {
        if ( Abc_ObjFaninNum(pObj) > 0 )
            Seq_ObjLatchGetInitNums( pObj, 0, pInits );
        if ( Abc_ObjFaninNum(pObj) > 1 )
            Seq_ObjLatchGetInitNums( pObj, 1, pInits );
    }
}

/**Function*************************************************************

  Synopsis    [Report nodes with equal fanins.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Seq_NtkLatchGetEqualFaninNum( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pObj;
    int i, Counter;
    assert( Abc_NtkIsSeq( pNtk ) );
    Counter = 0;
    Abc_AigForEachAnd( pNtk, pObj, i )
        if ( Abc_ObjFaninId0(pObj) == Abc_ObjFaninId1(pObj) )
            Counter++;
    if ( Counter )
        printf( "The number of nodes with equal fanins = %d.\n", Counter );
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Returns the maximum latch number on any of the fanouts.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Seq_NtkCountNodesAboveLimit( Abc_Ntk_t * pNtk, int Limit )
{
    Abc_Obj_t * pNode;
    int i, Counter;
    assert( !Abc_NtkIsSeq(pNtk) );
    Counter = 0;
    Abc_NtkForEachNode( pNtk, pNode, i )
        if ( Abc_ObjFaninNum(pNode) > Limit )
            Counter++;
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Computes area flows.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Seq_MapComputeAreaFlows( Abc_Ntk_t * pNtk, int fVerbose )
{
    Abc_Seq_t * p = pNtk->pManFunc;
    Abc_Obj_t * pObj;
    float AFlow;
    int i, c;

    assert( Abc_NtkIsSeq(pNtk) );

    Vec_IntFill( p->vAFlows, p->nSize, Abc_Float2Int( (float)0.0 ) );

    // update all values iteratively
    for ( c = 0; c < 7; c++ )
    {
        Abc_AigForEachAnd( pNtk, pObj, i )
        {
            AFlow = (float)1.0 + Seq_NodeGetFlow( Abc_ObjFanin0(pObj) ) + Seq_NodeGetFlow( Abc_ObjFanin1(pObj) );
            AFlow /= Abc_ObjFanoutNum(pObj);
            pObj->pNext = (void *)Abc_Float2Int( AFlow );
        }
        Abc_AigForEachAnd( pNtk, pObj, i )
        {
            AFlow = Abc_Int2Float( (int)pObj->pNext );
            pObj->pNext = NULL;
            Seq_NodeSetFlow( pObj, AFlow );

//            printf( "%5d : %6.1f\n", pObj->Id, Seq_NodeGetFlow(pObj) );
        }
//        printf( "\n" );
    }
    return 1;
}


/**Function*************************************************************

  Synopsis    [Collects all the internal nodes reachable from POs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Seq_NtkReachNodesFromPos_rec( Abc_Obj_t * pAnd, Vec_Ptr_t * vNodes )
{
    // skip if this is a non-PI node
    if ( !Abc_AigNodeIsAnd(pAnd) )
        return;
    // skip a visited node
    if ( Abc_NodeIsTravIdCurrent(pAnd) )
        return;
    Abc_NodeSetTravIdCurrent(pAnd);
    // visit the fanin nodes
    Seq_NtkReachNodesFromPos_rec( Abc_ObjFanin0(pAnd), vNodes );
    Seq_NtkReachNodesFromPos_rec( Abc_ObjFanin1(pAnd), vNodes );
    // add this node
    Vec_PtrPush( vNodes, pAnd );
}

/**Function*************************************************************

  Synopsis    [Collects all the internal nodes reachable from POs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Seq_NtkReachNodesFromPis_rec( Abc_Obj_t * pAnd, Vec_Ptr_t * vNodes )
{
    Abc_Obj_t * pFanout;
    int k;
    // skip if this is a non-PI node
    if ( !Abc_AigNodeIsAnd(pAnd) )
        return;
    // skip a visited node
    if ( Abc_NodeIsTravIdCurrent(pAnd) )
        return;
    Abc_NodeSetTravIdCurrent(pAnd);
    // visit the fanin nodes
    Abc_ObjForEachFanout( pAnd, pFanout, k )
        Seq_NtkReachNodesFromPis_rec( pFanout, vNodes );
    // add this node
    Vec_PtrPush( vNodes, pAnd );
}

/**Function*************************************************************

  Synopsis    [Collects all the internal nodes reachable from POs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Seq_NtkReachNodes( Abc_Ntk_t * pNtk, int fFromPos )
{
    Vec_Ptr_t * vNodes;
    Abc_Obj_t * pObj, * pFanout;
    int i, k;
    assert( Abc_NtkIsSeq(pNtk) );
    vNodes = Vec_PtrAlloc( 1000 );
    Abc_NtkIncrementTravId( pNtk );
    if ( fFromPos )
    {
        // traverse the cone of each PO
        Abc_NtkForEachPo( pNtk, pObj, i )
            Seq_NtkReachNodesFromPos_rec( Abc_ObjFanin0(pObj), vNodes );
    }
    else
    {
        // tranvers the reverse cone of the constant node
        pObj = Abc_AigConst1( pNtk );
        Abc_ObjForEachFanout( pObj, pFanout, k )
            Seq_NtkReachNodesFromPis_rec( pFanout, vNodes );
        // tranvers the reverse cone of the PIs
        Abc_NtkForEachPi( pNtk, pObj, i )
            Abc_ObjForEachFanout( pObj, pFanout, k )
                Seq_NtkReachNodesFromPis_rec( pFanout, vNodes );
    }
    return vNodes;
}

/**Function*************************************************************

  Synopsis    [Perform sequential cleanup.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Seq_NtkCleanup( Abc_Ntk_t * pNtk, int fVerbose )
{
    Vec_Ptr_t * vNodesPo, * vNodesPi;
    int Counter = 0;
    assert( Abc_NtkIsSeq(pNtk) );
    // collect the nodes reachable from POs and PIs
    vNodesPo = Seq_NtkReachNodes( pNtk, 1 );
    vNodesPi = Seq_NtkReachNodes( pNtk, 0 );
    printf( "Total nodes = %6d. Reachable from POs = %6d. Reachable from PIs = %6d.\n", 
        Abc_NtkNodeNum(pNtk), Vec_PtrSize(vNodesPo), Vec_PtrSize(vNodesPi) );
    if ( Abc_NtkNodeNum(pNtk) > Vec_PtrSize(vNodesPo) )
    {
//        Counter = Abc_NtkReduceNodes( pNtk, vNodesPo );
        Counter = 0;
        if ( fVerbose )
            printf( "Cleanup removed %d nodes that are not reachable from the POs.\n", Counter );
    }
    Vec_PtrFree( vNodesPo );
    Vec_PtrFree( vNodesPi );
    return Counter;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


