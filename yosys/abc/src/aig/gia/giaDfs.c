/**CFile****************************************************************

  FileName    [giaDfs.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Scalable AIG package.]

  Synopsis    [DFS procedures.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: giaDfs.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "gia.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Counts the support size of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManCollectCis_rec( Gia_Man_t * p, Gia_Obj_t * pObj, Vec_Int_t * vSupp )
{
    if ( Gia_ObjIsTravIdCurrent(p, pObj) )
        return;
    Gia_ObjSetTravIdCurrent(p, pObj);
    if ( Gia_ObjIsCi(pObj) )
    {
        Vec_IntPush( vSupp, Gia_ObjId(p, pObj) );
        return;
    }
    assert( Gia_ObjIsAnd(pObj) );
    Gia_ManCollectCis_rec( p, Gia_ObjFanin0(pObj), vSupp );
    Gia_ManCollectCis_rec( p, Gia_ObjFanin1(pObj), vSupp );
}

/**Function*************************************************************

  Synopsis    [Collects support nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManCollectCis( Gia_Man_t * p, int * pNodes, int nNodes, Vec_Int_t * vSupp )
{
    Gia_Obj_t * pObj;
    int i; 
    Vec_IntClear( vSupp );
    Gia_ManIncrementTravId( p );
    Gia_ObjSetTravIdCurrent( p, Gia_ManConst0(p) );
    for ( i = 0; i < nNodes; i++ )
    {
        pObj = Gia_ManObj( p, pNodes[i] );
        if ( Gia_ObjIsCo(pObj) )
            Gia_ManCollectCis_rec( p, Gia_ObjFanin0(pObj), vSupp );
        else
            Gia_ManCollectCis_rec( p, pObj, vSupp );
    }
}

/**Function*************************************************************

  Synopsis    [Counts the support size of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManCollectAnds_rec( Gia_Man_t * p, int iObj, Vec_Int_t * vNodes )
{
    Gia_Obj_t * pObj;
    if ( Gia_ObjIsTravIdCurrentId(p, iObj) )
        return;
    Gia_ObjSetTravIdCurrentId(p, iObj);
    pObj = Gia_ManObj( p, iObj );
    if ( Gia_ObjIsCi(pObj) )
        return;
    assert( Gia_ObjIsAnd(pObj) );
    Gia_ManCollectAnds_rec( p, Gia_ObjFaninId0(pObj, iObj), vNodes );
    Gia_ManCollectAnds_rec( p, Gia_ObjFaninId1(pObj, iObj), vNodes );
    Vec_IntPush( vNodes, iObj );
}

/**Function*************************************************************

  Synopsis    [Collects support nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManCollectAnds( Gia_Man_t * p, int * pNodes, int nNodes, Vec_Int_t * vNodes, Vec_Int_t * vLeaves )
{
    int i, iLeaf; 
//    Gia_ManIncrementTravId( p );
    Gia_ObjSetTravIdCurrentId( p, 0 );
    if ( vLeaves )
        Vec_IntForEachEntry( vLeaves, iLeaf, i )
            Gia_ObjSetTravIdCurrentId( p, iLeaf );
    Vec_IntClear( vNodes );
    for ( i = 0; i < nNodes; i++ )
    {
        Gia_Obj_t * pObj = Gia_ManObj( p, pNodes[i] );
        if ( Gia_ObjIsCo(pObj) )
            Gia_ManCollectAnds_rec( p, Gia_ObjFaninId0(pObj, pNodes[i]), vNodes );
        else if ( Gia_ObjIsAnd(pObj) )
            Gia_ManCollectAnds_rec( p, pNodes[i], vNodes );
    }
}

/**Function*************************************************************

  Synopsis    [Collects support nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Gia_ManCollectAndsAll( Gia_Man_t * p )
{
    Gia_Obj_t * pObj; int i;
    Vec_Int_t * vNodes = Vec_IntAlloc( Gia_ManAndNum(p) );
    Gia_ManForEachAnd( p, pObj, i )
        Vec_IntPush( vNodes, i );
    return vNodes;
}

/**Function*************************************************************

  Synopsis    [Counts the support size of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManCollectNodesCis_rec( Gia_Man_t * p, Gia_Obj_t * pObj, Vec_Int_t * vNodes )
{
    if ( Gia_ObjIsTravIdCurrent(p, pObj) )
        return;
    Gia_ObjSetTravIdCurrent(p, pObj);
    if ( Gia_ObjIsCi(pObj) )
    {
        Vec_IntPush( vNodes, Gia_ObjId(p, pObj) );
        return;
    }
    assert( Gia_ObjIsAnd(pObj) );
    Gia_ManCollectNodesCis_rec( p, Gia_ObjFanin0(pObj), vNodes );
    Gia_ManCollectNodesCis_rec( p, Gia_ObjFanin1(pObj), vNodes );
    Vec_IntPush( vNodes, Gia_ObjId(p, pObj) );
}

/**Function*************************************************************

  Synopsis    [Collects support nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Gia_ManCollectNodesCis( Gia_Man_t * p, int * pNodes, int nNodes )
{
    Vec_Int_t * vNodes;
    Gia_Obj_t * pObj;
    int i; 
    vNodes = Vec_IntAlloc( 10000 );
    Gia_ManIncrementTravId( p );
    Gia_ObjSetTravIdCurrent( p, Gia_ManConst0(p) );
    for ( i = 0; i < nNodes; i++ )
    {
        pObj = Gia_ManObj( p, pNodes[i] );
        if ( Gia_ObjIsCo(pObj) )
            Gia_ManCollectNodesCis_rec( p, Gia_ObjFanin0(pObj), vNodes );
        else
            Gia_ManCollectNodesCis_rec( p, pObj, vNodes );
    }
    return vNodes;
}

/**Function*************************************************************

  Synopsis    [Collects support nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManCollectTest( Gia_Man_t * p )
{
    Vec_Int_t * vNodes;
    Gia_Obj_t * pObj;
    int i, iNode;
    abctime clk = Abc_Clock();
    vNodes = Vec_IntAlloc( 100 );
    Gia_ManIncrementTravId( p );
    Gia_ManForEachCo( p, pObj, i )
    {
        iNode = Gia_ObjId(p, pObj);
        Gia_ManCollectAnds( p, &iNode, 1, vNodes, NULL );
    }
    Vec_IntFree( vNodes );
    ABC_PRT( "DFS from each output", Abc_Clock() - clk );
}

/**Function*************************************************************

  Synopsis    [Collects support nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManSuppSize_rec( Gia_Man_t * p, Gia_Obj_t * pObj )
{
    if ( Gia_ObjIsTravIdCurrent(p, pObj) )
        return 0;
    Gia_ObjSetTravIdCurrent(p, pObj);
    if ( Gia_ObjIsCi(pObj) )
        return 1;
    assert( Gia_ObjIsAnd(pObj) );
    return Gia_ManSuppSize_rec( p, Gia_ObjFanin0(pObj) ) +
        Gia_ManSuppSize_rec( p, Gia_ObjFanin1(pObj) );
}

/**Function*************************************************************

  Synopsis    [Computes support size of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManSuppSizeOne( Gia_Man_t * p, Gia_Obj_t * pObj )
{
    Gia_ManIncrementTravId( p );
    return Gia_ManSuppSize_rec( p, pObj );
}

/**Function*************************************************************

  Synopsis    [Computes support size of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManSuppSizeTest( Gia_Man_t * p )
{
    Gia_Obj_t * pObj;
    int i, Counter = 0;
    abctime clk = Abc_Clock();
    Gia_ManForEachObj( p, pObj, i )
        if ( Gia_ObjIsAnd(pObj) )
            Counter += (Gia_ManSuppSizeOne(p, pObj) <= 16);
    printf( "Nodes with small support %d (out of %d)\n", Counter, Gia_ManAndNum(p) );
    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Collects support nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManSuppSize( Gia_Man_t * p, int * pNodes, int nNodes )
{
    Gia_Obj_t * pObj;
    int i, Counter = 0; 
    Gia_ManIncrementTravId( p );
    Gia_ObjSetTravIdCurrent( p, Gia_ManConst0(p) );
    for ( i = 0; i < nNodes; i++ )
    {
        pObj = Gia_ManObj( p, pNodes[i] );
        if ( Gia_ObjIsCo(pObj) )
            Counter += Gia_ManSuppSize_rec( p, Gia_ObjFanin0(pObj) );
        else
            Counter += Gia_ManSuppSize_rec( p, pObj );
    }
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Collects support nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManConeSize_rec( Gia_Man_t * p, Gia_Obj_t * pObj )
{
    if ( Gia_ObjIsTravIdCurrent(p, pObj) )
        return 0;
    Gia_ObjSetTravIdCurrent(p, pObj);
    if ( Gia_ObjIsCi(pObj) )
        return 0;
    assert( Gia_ObjIsAnd(pObj) );
    return 1 + Gia_ManConeSize_rec( p, Gia_ObjFanin0(pObj) ) +
        Gia_ManConeSize_rec( p, Gia_ObjFanin1(pObj) );
}

/**Function*************************************************************

  Synopsis    [Collects support nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManConeSize( Gia_Man_t * p, int * pNodes, int nNodes )
{
    Gia_Obj_t * pObj;
    int i, Counter = 0; 
    Gia_ManIncrementTravId( p );
    Gia_ObjSetTravIdCurrent( p, Gia_ManConst0(p) );
    for ( i = 0; i < nNodes; i++ )
    {
        pObj = Gia_ManObj( p, pNodes[i] );
        if ( Gia_ObjIsCo(pObj) )
            Counter += Gia_ManConeSize_rec( p, Gia_ObjFanin0(pObj) );
        else
            Counter += Gia_ManConeSize_rec( p, pObj );
    }
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Levelizes the nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Vec_t * Gia_ManLevelize( Gia_Man_t * p )
{ 
    Gia_Obj_t * pObj;
    Vec_Vec_t * vLevels;
    int nLevels, Level, i;
    nLevels = Gia_ManLevelNum( p );
    vLevels = Vec_VecStart( nLevels + 1 );
    Gia_ManForEachAnd( p, pObj, i )
    {
        Level = Gia_ObjLevel( p, pObj );
        assert( Level <= nLevels );
        Vec_VecPush( vLevels, Level, pObj );
    }
    return vLevels;
}

/**Function*************************************************************

  Synopsis    [Levelizes the nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Wec_t * Gia_ManLevelizeR( Gia_Man_t * p )
{ 
    Gia_Obj_t * pObj;
    Vec_Wec_t * vLevels;
    int nLevels, Level, i;
    nLevels = Gia_ManLevelRNum( p );
    vLevels = Vec_WecStart( nLevels + 1 );
    Gia_ManForEachObj( p, pObj, i )
    {
        if ( i == 0 || (!Gia_ObjIsCo(pObj) && !Gia_ObjLevel(p, pObj)) )
            continue;
        Level = Gia_ObjLevel( p, pObj );
        assert( Level <= nLevels );
        Vec_WecPush( vLevels, Level, i );
    }
    return vLevels;
}
/**Function*************************************************************

  Synopsis    [Computes reverse topological order.]

  Description [Assumes that levels are already assigned.
  The levels of CO nodes may not be assigned.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Gia_ManOrderReverse( Gia_Man_t * p )
{
    Gia_Obj_t * pObj;
    Vec_Vec_t * vLevels;
    Vec_Ptr_t * vLevel;
    Vec_Int_t * vResult;
    int i, k;
    vLevels = Vec_VecStart( 100 );
    // make sure levels are assigned
    Gia_ManForEachAnd( p, pObj, i )
        assert( Gia_ObjLevel(p, pObj) > 0 );
    // add CO nodes based on the level of their fanin
    Gia_ManForEachCo( p, pObj, i )
        Vec_VecPush( vLevels, Gia_ObjLevel(p, Gia_ObjFanin0(pObj)), pObj );
    // add other nodes based on their level
    Gia_ManForEachObj( p, pObj, i )
        if ( !Gia_ObjIsCo(pObj) )
            Vec_VecPush( vLevels, Gia_ObjLevel(p, pObj), pObj );
    // put the nodes in the reverse topological order
    vResult = Vec_IntAlloc( Gia_ManObjNum(p) );
    Vec_VecForEachLevelReverse( vLevels, vLevel, i )
        Vec_PtrForEachEntry( Gia_Obj_t *, vLevel, pObj, k )
            Vec_IntPush( vResult, Gia_ObjId(p, pObj) );
    Vec_VecFree( vLevels );
    return vResult;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManCollectSeq_rec( Gia_Man_t * p, int Id, Vec_Int_t * vRoots, Vec_Int_t * vObjs )
{
    Gia_Obj_t * pObj;
    if ( Gia_ObjIsTravIdCurrentId( p, Id ) )
        return;
    Gia_ObjSetTravIdCurrentId( p, Id );
    pObj = Gia_ManObj( p, Id );
    if ( Gia_ObjIsAnd(pObj) )
    {
        Gia_ManCollectSeq_rec( p, Gia_ObjFaninId0(pObj, Id), vRoots, vObjs );
        Gia_ManCollectSeq_rec( p, Gia_ObjFaninId1(pObj, Id), vRoots, vObjs );
    }
    else if ( Gia_ObjIsCi(pObj) )
    {
        if ( Gia_ObjIsRo(p, pObj) )
            Vec_IntPush( vRoots, Gia_ObjId(p, Gia_ObjRoToRi(p, pObj)) );
    }
    else if ( Gia_ObjIsCo(pObj) )
        Gia_ManCollectSeq_rec( p, Gia_ObjFaninId0(pObj, Id), vRoots, vObjs );
    else assert( 0 );
    Vec_IntPush( vObjs, Id );
}
Vec_Int_t * Gia_ManCollectSeq( Gia_Man_t * p, int * pPos, int nPos )
{
    Vec_Int_t * vObjs, * vRoots;
    int i, iRoot;
    // collect roots
    vRoots = Vec_IntAlloc( 100 );
    for ( i = 0; i < nPos; i++ )
        Vec_IntPush( vRoots, Gia_ObjId(p, Gia_ManPo(p, pPos[i])) );
    // start trav IDs
    Gia_ManIncrementTravId( p );
    Gia_ObjSetTravIdCurrentId( p, 0 );
    // collect objects
    vObjs = Vec_IntAlloc( 1000 );
    Vec_IntPush( vObjs, 0 );
    Vec_IntForEachEntry( vRoots, iRoot, i )
        Gia_ManCollectSeq_rec( p, iRoot, vRoots, vObjs );
    Vec_IntFree( vRoots );
    return vObjs;
}
void Gia_ManCollectSeqTest( Gia_Man_t * p )
{
    Vec_Int_t * vObjs;
    int i;
    abctime clk = Abc_Clock();
    for ( i = 0; i < Gia_ManPoNum(p); i++ )
    {
        if ( i % 10000 == 0 )
            printf( "%8d finished...\r", i );

        vObjs = Gia_ManCollectSeq( p, &i, 1 );
//        printf( "%d ", Vec_IntSize(vObjs) );
        Vec_IntFree( vObjs );
    }
    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );

}

/**Function*************************************************************

  Synopsis    [Collect TFI nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManCollectTfi_rec( Gia_Man_t * p, int iObj, Vec_Int_t * vNodes )
{
    Gia_Obj_t * pObj;
    if ( Gia_ObjIsTravIdCurrentId(p, iObj) )
        return;
    Gia_ObjSetTravIdCurrentId(p, iObj);
    pObj = Gia_ManObj( p, iObj );
    if ( Gia_ObjIsCi(pObj) )
        return;
    assert( Gia_ObjIsAnd(pObj) );
    Gia_ManCollectTfi_rec( p, Gia_ObjFaninId0(pObj, iObj), vNodes );
    Gia_ManCollectTfi_rec( p, Gia_ObjFaninId1(pObj, iObj), vNodes );
    Vec_IntPush( vNodes, iObj );
}
void Gia_ManCollectTfi( Gia_Man_t * p, Vec_Int_t * vRoots, Vec_Int_t * vNodes )
{
    int i, iRoot; 
    Vec_IntClear( vNodes );
    Gia_ManIncrementTravId( p );
    Vec_IntForEachEntry( vRoots, iRoot, i )
        Gia_ManCollectTfi_rec( p, iRoot, vNodes );
}

/**Function*************************************************************

  Synopsis    [Collect TFI nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManCollectTfo_rec( Gia_Man_t * p, int iObj, Vec_Int_t * vNodes )
{
    Gia_Obj_t * pObj; int i, iFan;
    if ( Gia_ObjIsTravIdCurrentId(p, iObj) )
        return;
    Gia_ObjSetTravIdCurrentId(p, iObj);
    pObj = Gia_ManObj( p, iObj );
    if ( Gia_ObjIsCo(pObj) )
        return;
    assert( Gia_ObjIsAnd(pObj) );
    Gia_ObjForEachFanoutStaticId( p, iObj, iFan, i )
        Gia_ManCollectTfo_rec( p, iFan, vNodes );
    Vec_IntPush( vNodes, iObj );
}
void Gia_ManCollectTfo( Gia_Man_t * p, Vec_Int_t * vRoots, Vec_Int_t * vNodes )
{
    int i, iRoot; 
    Vec_IntClear( vNodes );
    Gia_ManIncrementTravId( p );
    Vec_IntForEachEntry( vRoots, iRoot, i )
        Gia_ManCollectTfo_rec( p, iRoot, vNodes );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

