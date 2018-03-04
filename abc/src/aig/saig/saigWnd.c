/**CFile****************************************************************

  FileName    [saigWnd.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Sequential AIG package.]

  Synopsis    [Sequential windowing.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: saigWnd.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "saig.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Returns the array of PI/internal nodes.]

  Description [Marks all the visited nodes with the current ID.
  Does not collect constant node and PO/LI nodes.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Saig_ManWindowOutline_rec( Aig_Man_t * p, Aig_Obj_t * pObj, int nDist, Vec_Ptr_t * vNodes, int * pDists )
{
    Aig_Obj_t * pMatch, * pFanout;
    int fCollected, iFanout = -1, i;
    if ( nDist == 0 )
        return;
    if ( pDists[pObj->Id] >= nDist )
        return;
    pDists[pObj->Id] = nDist;
    fCollected = Aig_ObjIsTravIdCurrent( p, pObj );
    Aig_ObjSetTravIdCurrent( p, pObj );    
    if ( Aig_ObjIsConst1(pObj) )
        return;
    if ( Saig_ObjIsPo(p, pObj) )
        return;
    if ( Saig_ObjIsLi(p, pObj) )
    {
        pMatch = Saig_ObjLiToLo( p, pObj );
        if ( !Aig_ObjIsTravIdCurrent( p, pMatch ) )
            Saig_ManWindowOutline_rec( p, pMatch, nDist, vNodes, pDists );
        Saig_ManWindowOutline_rec( p, Aig_ObjFanin0(pObj), nDist-1, vNodes, pDists );
        return;
    }
    if ( !fCollected )
        Vec_PtrPush( vNodes, pObj );
    if ( Saig_ObjIsPi(p, pObj) )
        return;
    if ( Saig_ObjIsLo(p, pObj) )
    {
        pMatch = Saig_ObjLoToLi( p, pObj );
        if ( !Aig_ObjIsTravIdCurrent( p, pMatch ) )
            Saig_ManWindowOutline_rec( p, pMatch, nDist, vNodes, pDists );
        Aig_ObjForEachFanout( p, pObj, pFanout, iFanout, i )
            Saig_ManWindowOutline_rec( p, pFanout, nDist-1, vNodes, pDists );
        return;
    }
    assert( Aig_ObjIsNode(pObj) );
    Saig_ManWindowOutline_rec( p, Aig_ObjFanin0(pObj), nDist-1, vNodes, pDists );
    Saig_ManWindowOutline_rec( p, Aig_ObjFanin1(pObj), nDist-1, vNodes, pDists );
    Aig_ObjForEachFanout( p, pObj, pFanout, iFanout, i )
        Saig_ManWindowOutline_rec( p, pFanout, nDist-1, vNodes, pDists );
}

/**Function*************************************************************

  Synopsis    [Returns the array of PI/internal nodes.]

  Description [Marks all the visited nodes with the current ID.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Saig_ManWindowOutline( Aig_Man_t * p, Aig_Obj_t * pObj, int nDist )
{
    Vec_Ptr_t * vNodes;
    Aig_Obj_t * pObjLi, * pObjLo;
    int * pDists, i;
    pDists = ABC_CALLOC( int, Aig_ManObjNumMax(p) );
    vNodes = Vec_PtrAlloc( 1000 );
    Aig_ManIncrementTravId( p );
    Saig_ManWindowOutline_rec( p, pObj, nDist, vNodes, pDists );
    Vec_PtrSort( vNodes, (int (*)(void))Aig_ObjCompareIdIncrease );
    // make sure LI/LO are labeled/unlabeled mutually
    Saig_ManForEachLiLo( p, pObjLi, pObjLo, i )
        assert( Aig_ObjIsTravIdCurrent(p, pObjLi) == 
                Aig_ObjIsTravIdCurrent(p, pObjLo) );
    ABC_FREE( pDists );
    return vNodes;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if the node has unlabeled fanout.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Obj_t * Saig_ObjHasUnlabeledFanout( Aig_Man_t * p, Aig_Obj_t * pObj )
{
    Aig_Obj_t * pFanout;
    int iFanout = -1, i;
    Aig_ObjForEachFanout( p, pObj, pFanout, iFanout, i )
        if ( Saig_ObjIsPo(p, pFanout) || !Aig_ObjIsTravIdCurrent(p, pFanout) )
            return pFanout;
    return NULL;
}

/**Function*************************************************************

  Synopsis    [Collects primary inputs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Saig_ManWindowCollectPis( Aig_Man_t * p, Vec_Ptr_t * vNodes )
{
    Vec_Ptr_t * vNodesPi;
    Aig_Obj_t * pObj, * pMatch, * pFanin;
    int i;
    vNodesPi = Vec_PtrAlloc( 1000 );
    Vec_PtrForEachEntry( Aig_Obj_t *, vNodes, pObj, i )
    {
        if ( Saig_ObjIsPi(p, pObj) )
        {
            assert( pObj->pData == NULL );
            Vec_PtrPush( vNodesPi, pObj );
        }
        else if ( Saig_ObjIsLo(p, pObj) )
        {
            pMatch = Saig_ObjLoToLi( p, pObj );
            pFanin = Aig_ObjFanin0(pMatch);
            if ( !Aig_ObjIsTravIdCurrent(p, pFanin) && pFanin->pData == NULL )
                Vec_PtrPush( vNodesPi, pFanin );
        }
        else
        {
            assert( Aig_ObjIsNode(pObj) );
            pFanin = Aig_ObjFanin0(pObj);
            if ( !Aig_ObjIsTravIdCurrent(p, pFanin) && pFanin->pData == NULL )
                Vec_PtrPush( vNodesPi, pFanin );
            pFanin = Aig_ObjFanin1(pObj);
            if ( !Aig_ObjIsTravIdCurrent(p, pFanin) && pFanin->pData == NULL )
                Vec_PtrPush( vNodesPi, pFanin );
        }
    }
    return vNodesPi;
}

/**Function*************************************************************

  Synopsis    [Collects primary outputs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Saig_ManWindowCollectPos( Aig_Man_t * p, Vec_Ptr_t * vNodes, Vec_Ptr_t ** pvPointers )
{
    Vec_Ptr_t * vNodesPo;
    Aig_Obj_t * pObj, * pPointer;
    int i;
    vNodesPo = Vec_PtrAlloc( 1000 );
    if ( pvPointers )
        *pvPointers = Vec_PtrAlloc( 1000 );
    Vec_PtrForEachEntry( Aig_Obj_t *, vNodes, pObj, i )
    {
        if ( (pPointer = Saig_ObjHasUnlabeledFanout(p, pObj)) )
        {
            Vec_PtrPush( vNodesPo, pObj );
            if ( pvPointers )
                Vec_PtrPush( *pvPointers, pPointer );
        }
    }
    return vNodesPo;
}

/**Function*************************************************************

  Synopsis    [Extracts the window AIG from the AIG manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Saig_ManWindowExtractNodes( Aig_Man_t * p, Vec_Ptr_t * vNodes )
{
    Aig_Man_t * pNew;
    Aig_Obj_t * pObj, * pMatch;
    Vec_Ptr_t * vNodesPi, * vNodesPo;
    int i, nRegCount;
    Aig_ManCleanData( p ); 
    // create the new manager
    pNew = Aig_ManStart( Vec_PtrSize(vNodes) );
    pNew->pName = Abc_UtilStrsav( "wnd" );
    pNew->pSpec = NULL;
    // map constant nodes
    pObj = Aig_ManConst1( p );
    pObj->pData = Aig_ManConst1( pNew );
    // create real PIs
    vNodesPi = Saig_ManWindowCollectPis( p, vNodes );
    Vec_PtrForEachEntry( Aig_Obj_t *, vNodesPi, pObj, i )
        pObj->pData = Aig_ObjCreateCi(pNew);
    Vec_PtrFree( vNodesPi );
    // create register outputs
    Vec_PtrForEachEntry( Aig_Obj_t *, vNodes, pObj, i )
    {
        if ( Saig_ObjIsLo(p, pObj) )
            pObj->pData = Aig_ObjCreateCi(pNew);
    }
    // create internal nodes
    Vec_PtrForEachEntry( Aig_Obj_t *, vNodes, pObj, i )
    {
        if ( Aig_ObjIsNode(pObj) )
            pObj->pData = Aig_And( pNew, Aig_ObjChild0Copy(pObj), Aig_ObjChild1Copy(pObj) );
    }
    // create POs
    vNodesPo = Saig_ManWindowCollectPos( p, vNodes, NULL );
    Vec_PtrForEachEntry( Aig_Obj_t *, vNodesPo, pObj, i )
        Aig_ObjCreateCo( pNew, (Aig_Obj_t *)pObj->pData );
    Vec_PtrFree( vNodesPo );
    // create register inputs
    nRegCount = 0;
    Vec_PtrForEachEntry( Aig_Obj_t *, vNodes, pObj, i )
    {
        if ( Saig_ObjIsLo(p, pObj) )
        {
            pMatch = Saig_ObjLoToLi( p, pObj );
            Aig_ObjCreateCo( pNew, Aig_ObjChild0Copy(pMatch) );
            nRegCount++;
        }
    }
    Aig_ManSetRegNum( pNew, nRegCount );
    Aig_ManCleanup( pNew );
    return pNew;
}

static void Saig_ManWindowInsertSmall_rec( Aig_Man_t * pNew, Aig_Obj_t * pObjSmall, 
     Vec_Ptr_t * vBigNode2SmallPo, Vec_Ptr_t * vSmallPi2BigNode );

/**Function*************************************************************

  Synopsis    [Adds nodes for the big manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Saig_ManWindowInsertBig_rec( Aig_Man_t * pNew, Aig_Obj_t * pObjBig, 
         Vec_Ptr_t * vBigNode2SmallPo, Vec_Ptr_t * vSmallPi2BigNode )
{
    Aig_Obj_t * pMatch;
    if ( pObjBig->pData )
        return;
    if ( (pMatch = (Aig_Obj_t *)Vec_PtrEntry( vBigNode2SmallPo, pObjBig->Id )) )
    {
        Saig_ManWindowInsertSmall_rec( pNew, Aig_ObjFanin0(pMatch), vBigNode2SmallPo, vSmallPi2BigNode );
        pObjBig->pData = Aig_ObjChild0Copy(pMatch);
        return;        
    }
    assert( Aig_ObjIsNode(pObjBig) );
    Saig_ManWindowInsertBig_rec( pNew, Aig_ObjFanin0(pObjBig), vBigNode2SmallPo, vSmallPi2BigNode );
    Saig_ManWindowInsertBig_rec( pNew, Aig_ObjFanin1(pObjBig), vBigNode2SmallPo, vSmallPi2BigNode );
    pObjBig->pData = Aig_And( pNew, Aig_ObjChild0Copy(pObjBig), Aig_ObjChild1Copy(pObjBig) );
}

/**Function*************************************************************

  Synopsis    [Adds nodes for the small manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Saig_ManWindowInsertSmall_rec( Aig_Man_t * pNew, Aig_Obj_t * pObjSmall, 
         Vec_Ptr_t * vBigNode2SmallPo, Vec_Ptr_t * vSmallPi2BigNode )
{
    Aig_Obj_t * pMatch;
    if ( pObjSmall->pData )
        return;
    if ( (pMatch = (Aig_Obj_t *)Vec_PtrEntry( vSmallPi2BigNode, pObjSmall->Id )) )
    {
        Saig_ManWindowInsertBig_rec( pNew, pMatch, vBigNode2SmallPo, vSmallPi2BigNode );
        pObjSmall->pData = pMatch->pData;
        return;        
    }
    assert( Aig_ObjIsNode(pObjSmall) );
    Saig_ManWindowInsertSmall_rec( pNew, Aig_ObjFanin0(pObjSmall), vBigNode2SmallPo, vSmallPi2BigNode );
    Saig_ManWindowInsertSmall_rec( pNew, Aig_ObjFanin1(pObjSmall), vBigNode2SmallPo, vSmallPi2BigNode );
    pObjSmall->pData = Aig_And( pNew, Aig_ObjChild0Copy(pObjSmall), Aig_ObjChild1Copy(pObjSmall) );
}

/**Function*************************************************************

  Synopsis    [Extracts the network from the AIG manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Saig_ManWindowInsertNodes( Aig_Man_t * p, Vec_Ptr_t * vNodes, Aig_Man_t * pWnd )
{
    Aig_Man_t * pNew;
    Vec_Ptr_t * vBigNode2SmallPo, * vSmallPi2BigNode;
    Vec_Ptr_t * vNodesPi, * vNodesPo;
    Aig_Obj_t * pObj;
    int i;

    // set mapping of small PIs into big nodes
    vSmallPi2BigNode = Vec_PtrStart( Aig_ManObjNumMax(pWnd) ); 
    vNodesPi = Saig_ManWindowCollectPis( p, vNodes );
    Vec_PtrForEachEntry( Aig_Obj_t *, vNodesPi, pObj, i )
        Vec_PtrWriteEntry( vSmallPi2BigNode, Aig_ManCi(pWnd, i)->Id, pObj );
    assert( i == Saig_ManPiNum(pWnd) );
    Vec_PtrFree( vNodesPi );

    // set mapping of big nodes into small POs
    vBigNode2SmallPo = Vec_PtrStart( Aig_ManObjNumMax(p) ); 
    vNodesPo = Saig_ManWindowCollectPos( p, vNodes, NULL );
    Vec_PtrForEachEntry( Aig_Obj_t *, vNodesPo, pObj, i )
        Vec_PtrWriteEntry( vBigNode2SmallPo, pObj->Id, Aig_ManCo(pWnd, i) );
    assert( i == Saig_ManPoNum(pWnd) );
    Vec_PtrFree( vNodesPo );

    // create the new manager
    Aig_ManCleanData( p ); 
    Aig_ManCleanData( pWnd ); 
    pNew = Aig_ManStart( Aig_ManObjNumMax(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    // map constant nodes
    pObj = Aig_ManConst1( p );
    pObj->pData = Aig_ManConst1( pNew );
    pObj = Aig_ManConst1( pWnd );
    pObj->pData = Aig_ManConst1( pNew );

    // create real PIs
    Aig_ManForEachCi( p, pObj, i )
        if ( Saig_ObjIsPi(p, pObj) || !Aig_ObjIsTravIdCurrent(p, pObj) )
            pObj->pData = Aig_ObjCreateCi(pNew);
    // create additional latch outputs
    Saig_ManForEachLo( pWnd, pObj, i )
        pObj->pData = Aig_ObjCreateCi(pNew);

    // create internal nodes starting from the big
    Aig_ManForEachCo( p, pObj, i )
        if ( Saig_ObjIsPo(p, pObj) || !Aig_ObjIsTravIdCurrent(p, pObj) )
        {
            Saig_ManWindowInsertBig_rec( pNew, Aig_ObjFanin0(pObj), vBigNode2SmallPo, vSmallPi2BigNode );
            pObj->pData = Aig_ObjCreateCo( pNew, Aig_ObjChild0Copy(pObj) );
        }
    // create internal nodes starting from the small
    Saig_ManForEachLi( pWnd, pObj, i )
    {
        Saig_ManWindowInsertSmall_rec( pNew, Aig_ObjFanin0(pObj), vBigNode2SmallPo, vSmallPi2BigNode );
        pObj->pData = Aig_ObjCreateCo( pNew, Aig_ObjChild0Copy(pObj) );
    }
    Vec_PtrFree( vBigNode2SmallPo );
    Vec_PtrFree( vSmallPi2BigNode );
    // set the new number of registers
    assert( Aig_ManCiNum(pNew) - Aig_ManCiNum(p) == Aig_ManCoNum(pNew) - Aig_ManCoNum(p) );
    Aig_ManSetRegNum( pNew, Aig_ManRegNum(p) + (Aig_ManCiNum(pNew) - Aig_ManCiNum(p)) );
    Aig_ManCleanup( pNew );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Find a good object.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Obj_t * Saig_ManFindPivot( Aig_Man_t * p )
{
    Aig_Obj_t * pObj;
    int i, Counter;
    if ( Aig_ManRegNum(p) > 0 )
    {
        if ( Aig_ManRegNum(p) == 1 )
            return Saig_ManLo( p, 0 );
        Saig_ManForEachLo( p, pObj, i )
        {
            if ( i == Aig_ManRegNum(p)/2 )
                return pObj;
        }
    }
    else
    {
        Counter = 0;
        assert( Aig_ManNodeNum(p) > 1 );
        Aig_ManForEachNode( p, pObj, i )
        {
            if ( Counter++ == Aig_ManNodeNum(p)/2 )
                return pObj;
        }
    }
    return NULL;
}

/**Function*************************************************************

  Synopsis    [Computes sequential window of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Saig_ManWindowExtract( Aig_Man_t * p, Aig_Obj_t * pObj, int nDist )
{
    Aig_Man_t * pWnd;
    Vec_Ptr_t * vNodes;
    Aig_ManFanoutStart( p );
    vNodes = Saig_ManWindowOutline( p, pObj, nDist );
    pWnd = Saig_ManWindowExtractNodes( p, vNodes );
    Vec_PtrFree( vNodes );
    Aig_ManFanoutStop( p );
    return pWnd;
}

/**Function*************************************************************

  Synopsis    [Computes sequential window of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Saig_ManWindowInsert( Aig_Man_t * p, Aig_Obj_t * pObj, int nDist, Aig_Man_t * pWnd )
{
    Aig_Man_t * pNew, * pWndTest;
    Vec_Ptr_t * vNodes;
    Aig_ManFanoutStart( p );

    vNodes = Saig_ManWindowOutline( p, pObj, nDist );
    pWndTest = Saig_ManWindowExtractNodes( p, vNodes );
    if ( Saig_ManPiNum(pWndTest) != Saig_ManPiNum(pWnd) ||
         Saig_ManPoNum(pWndTest) != Saig_ManPoNum(pWnd) )
    {
        printf( "The window cannot be reinserted because PI/PO counts do not match.\n" );
        Aig_ManStop( pWndTest );
        Vec_PtrFree( vNodes );
        Aig_ManFanoutStop( p );
        return NULL;
    }
    Aig_ManStop( pWndTest );
    Vec_PtrFree( vNodes );

    // insert the nodes
    Aig_ManCleanData( p ); 
    vNodes = Saig_ManWindowOutline( p, pObj, nDist );
    pNew = Saig_ManWindowInsertNodes( p, vNodes, pWnd );
    Vec_PtrFree( vNodes );
    Aig_ManFanoutStop( p );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Tests the above computation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Saig_ManWindowTest( Aig_Man_t * p )
{
    int nDist = 3;
    Aig_Man_t * pWnd, * pNew;
    Aig_Obj_t * pPivot;
    pPivot = Saig_ManFindPivot( p );
    assert( pPivot != NULL );
    pWnd = Saig_ManWindowExtract( p, pPivot, nDist );
    pNew = Saig_ManWindowInsert( p, pPivot, nDist, pWnd );
    Aig_ManStop( pWnd );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Collects the nodes that are not linked to each other.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Saig_ManCollectedDiffNodes( Aig_Man_t * p0, Aig_Man_t * p1 )
{
    Vec_Ptr_t * vNodes;
    Aig_Obj_t * pObj0, * pObj1;
    int i;
    // collect nodes that are not linked
    Aig_ManIncrementTravId( p0 );
    vNodes = Vec_PtrAlloc( 1000 );
    Aig_ManForEachObj( p0, pObj0, i )
    {
        pObj1 = Aig_ObjRepr( p0, pObj0 );
        if ( pObj1 != NULL )
        {
            assert( pObj0 == Aig_ObjRepr( p1, pObj1 ) );   
            continue;
        }
        // mark and collect unmatched objects
        Aig_ObjSetTravIdCurrent( p0, pObj0 ); 
        if ( Aig_ObjIsNode(pObj0) || Aig_ObjIsCi(pObj0) )
            Vec_PtrPush( vNodes, pObj0 );
    }
    // make sure LI/LO are labeled/unlabeled mutually
    Saig_ManForEachLiLo( p0, pObj0, pObj1, i )
        assert( Aig_ObjIsTravIdCurrent(p0, pObj0) == 
                Aig_ObjIsTravIdCurrent(p0, pObj1) );
    return vNodes;
}
         
/**Function*************************************************************

  Synopsis    [Creates PIs of the miter.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Saig_ManWindowCreatePis( Aig_Man_t * pNew, Aig_Man_t * p0, Aig_Man_t * p1, Vec_Ptr_t * vNodes0 )
{
    Aig_Obj_t * pObj, * pMatch, * pFanin;
    int i, Counter = 0;
    Vec_PtrForEachEntry( Aig_Obj_t *, vNodes0, pObj, i )
    {
        if ( Saig_ObjIsLo(p0, pObj) )
        {
            pMatch = Saig_ObjLoToLi( p0, pObj );
            pFanin = Aig_ObjFanin0(pMatch);
            if ( !Aig_ObjIsTravIdCurrent(p0, pFanin) && pFanin->pData == NULL )
            {
                pFanin->pData = Aig_ObjCreateCi(pNew);
                pMatch = Aig_ObjRepr( p0, pFanin );
                assert( pFanin == Aig_ObjRepr( p1, pMatch ) );
                assert( pMatch != NULL );
                pMatch->pData = pFanin->pData;
                Counter++;
            }
        }
        else
        {
            assert( Aig_ObjIsNode(pObj) );
            pFanin = Aig_ObjFanin0(pObj);
            if ( !Aig_ObjIsTravIdCurrent(p0, pFanin) && pFanin->pData == NULL )
            {
                pFanin->pData = Aig_ObjCreateCi(pNew);
                pMatch = Aig_ObjRepr( p0, pFanin );
                assert( pFanin == Aig_ObjRepr( p1, pMatch ) );
                assert( pMatch != NULL );
                pMatch->pData = pFanin->pData;
                Counter++;
            }
            pFanin = Aig_ObjFanin1(pObj);
            if ( !Aig_ObjIsTravIdCurrent(p0, pFanin) && pFanin->pData == NULL )
            {
                pFanin->pData = Aig_ObjCreateCi(pNew);
                pMatch = Aig_ObjRepr( p0, pFanin );
                assert( pFanin == Aig_ObjRepr( p1, pMatch ) );
                assert( pMatch != NULL );
                pMatch->pData = pFanin->pData;
                Counter++;
            }
        }
    }
//    printf( "Added %d primary inputs.\n", Counter );
}
        
/**Function*************************************************************

  Synopsis    [Creates POs of the miter.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Saig_ManWindowCreatePos( Aig_Man_t * pNew, Aig_Man_t * p0, Aig_Man_t * p1 )
{
    Aig_Obj_t * pObj0, * pObj1, * pMiter;
    Aig_Obj_t * pFanin0, * pFanin1;
    int i;
    Aig_ManForEachObj( p0, pObj0, i )
    {
        if ( Aig_ObjIsTravIdCurrent(p0, pObj0) )
            continue;
        if ( Aig_ObjIsConst1(pObj0) )
            continue;
        if ( Aig_ObjIsCi(pObj0) )
            continue;
        pObj1 = Aig_ObjRepr( p0, pObj0 );
        assert( pObj0 == Aig_ObjRepr( p1, pObj1 ) );
        if ( Aig_ObjIsCo(pObj0) )
        {
            pFanin0 = Aig_ObjFanin0(pObj0);
            pFanin1 = Aig_ObjFanin0(pObj1);
            assert( Aig_ObjIsTravIdCurrent(p0, pFanin0) ==
                    Aig_ObjIsTravIdCurrent(p1, pFanin1) );
            if ( Aig_ObjIsTravIdCurrent(p0, pFanin0) )
            {
                pMiter = Aig_Exor( pNew, (Aig_Obj_t *)pFanin0->pData, (Aig_Obj_t *)pFanin1->pData );
                Aig_ObjCreateCo( pNew, pMiter );
            }
        }
        else
        {
            assert( Aig_ObjIsNode(pObj0) );

            pFanin0 = Aig_ObjFanin0(pObj0);
            pFanin1 = Aig_ObjFanin0(pObj1);
            assert( Aig_ObjIsTravIdCurrent(p0, pFanin0) ==
                    Aig_ObjIsTravIdCurrent(p1, pFanin1) );
            if ( Aig_ObjIsTravIdCurrent(p0, pFanin0) )
            {
                pMiter = Aig_Exor( pNew, (Aig_Obj_t *)pFanin0->pData, (Aig_Obj_t *)pFanin1->pData );
                Aig_ObjCreateCo( pNew, pMiter );
            }

            pFanin0 = Aig_ObjFanin1(pObj0);
            pFanin1 = Aig_ObjFanin1(pObj1);
            assert( Aig_ObjIsTravIdCurrent(p0, pFanin0) ==
                    Aig_ObjIsTravIdCurrent(p1, pFanin1) );
            if ( Aig_ObjIsTravIdCurrent(p0, pFanin0) )
            {
                pMiter = Aig_Exor( pNew, (Aig_Obj_t *)pFanin0->pData, (Aig_Obj_t *)pFanin1->pData );
                Aig_ObjCreateCo( pNew, pMiter );
            }
        }
    }
}


/**Function*************************************************************

  Synopsis    [Extracts the window AIG from the AIG manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Saig_ManWindowExtractMiter( Aig_Man_t * p0, Aig_Man_t * p1 )
{
    Aig_Man_t * pNew;
    Aig_Obj_t * pObj0, * pObj1, * pMatch0, * pMatch1;
    Vec_Ptr_t * vNodes0, * vNodes1;
    int i, nRegCount;
    // add matching of POs and LIs
    Saig_ManForEachPo( p0, pObj0, i )
    {
        pObj1 = Aig_ManCo( p1, i );
        Aig_ObjSetRepr( p0, pObj0, pObj1 );
        Aig_ObjSetRepr( p1, pObj1, pObj0 );
    }
    Saig_ManForEachLi( p0, pObj0, i )
    {
        pMatch0 = Saig_ObjLiToLo( p0, pObj0 );
        pMatch1 = Aig_ObjRepr( p0, pMatch0 );
        if ( pMatch1 == NULL )
            continue;
        assert( pMatch0 == Aig_ObjRepr( p1, pMatch1 ) );
        pObj1 = Saig_ObjLoToLi( p1, pMatch1 );
        Aig_ObjSetRepr( p0, pObj0, pObj1 );
        Aig_ObjSetRepr( p1, pObj1, pObj0 );
    }
    // clean the markings
    Aig_ManCleanData( p0 ); 
    Aig_ManCleanData( p1 ); 
    // collect nodes that are not linked
    vNodes0 = Saig_ManCollectedDiffNodes( p0, p1 );
    vNodes1 = Saig_ManCollectedDiffNodes( p1, p0 );
    // create the new manager
    pNew = Aig_ManStart( Vec_PtrSize(vNodes0) + Vec_PtrSize(vNodes1) );
    pNew->pName = Abc_UtilStrsav( "wnd" );
    pNew->pSpec = NULL;
    // map constant nodes
    pObj0 = Aig_ManConst1( p0 );
    pObj0->pData = Aig_ManConst1( pNew );
    pObj1 = Aig_ManConst1( p1 );
    pObj1->pData = Aig_ManConst1( pNew );
    // create real PIs
    Saig_ManWindowCreatePis( pNew, p0, p1, vNodes0 );
    Saig_ManWindowCreatePis( pNew, p1, p0, vNodes1 );
    // create register outputs
    Vec_PtrForEachEntry( Aig_Obj_t *, vNodes0, pObj0, i )
    {
        if ( Saig_ObjIsLo(p0, pObj0) )
            pObj0->pData = Aig_ObjCreateCi(pNew);
    }
    Vec_PtrForEachEntry( Aig_Obj_t *, vNodes1, pObj1, i )
    {
        if ( Saig_ObjIsLo(p1, pObj1) )
            pObj1->pData = Aig_ObjCreateCi(pNew);
    }
    // create internal nodes
    Vec_PtrForEachEntry( Aig_Obj_t *, vNodes0, pObj0, i )
    {
        if ( Aig_ObjIsNode(pObj0) )
            pObj0->pData = Aig_And( pNew, Aig_ObjChild0Copy(pObj0), Aig_ObjChild1Copy(pObj0) );
    }
    Vec_PtrForEachEntry( Aig_Obj_t *, vNodes1, pObj1, i )
    {
        if ( Aig_ObjIsNode(pObj1) )
            pObj1->pData = Aig_And( pNew, Aig_ObjChild0Copy(pObj1), Aig_ObjChild1Copy(pObj1) );
    }
    // create POs
    Saig_ManWindowCreatePos( pNew, p0, p1 );
//    Saig_ManWindowCreatePos( pNew, p1, p0 );
    // create register inputs
    nRegCount = 0;
    Vec_PtrForEachEntry( Aig_Obj_t *, vNodes0, pObj0, i )
    {
        if ( Saig_ObjIsLo(p0, pObj0) )
        {
            pMatch0 = Saig_ObjLoToLi( p0, pObj0 );
            Aig_ObjCreateCo( pNew, Aig_ObjChild0Copy(pMatch0) );
            nRegCount++;
        }
    }
    Vec_PtrForEachEntry( Aig_Obj_t *, vNodes1, pObj1, i )
    {
        if ( Saig_ObjIsLo(p1, pObj1) )
        {
            pMatch1 = Saig_ObjLoToLi( p1, pObj1 );
            Aig_ObjCreateCo( pNew, Aig_ObjChild0Copy(pMatch1) );
            nRegCount++;
        }
    }
    Aig_ManSetRegNum( pNew, nRegCount );
    Aig_ManCleanup( pNew );
    Vec_PtrFree( vNodes0 );
    Vec_PtrFree( vNodes1 );
    return pNew;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

