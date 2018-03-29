/**CFile****************************************************************

  FileName    [sfmWin.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [SAT-based optimization using internal don't-cares.]

  Synopsis    [Structural window computation.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: sfmWin.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "sfmInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Returns the MFFC size.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Sfm_ObjRef_rec( Sfm_Ntk_t * p, int iObj )
{
    int i, iFanin, Value, Count;
    if ( Sfm_ObjIsPi(p, iObj) )
        return 0;
    assert( Sfm_ObjIsNode(p, iObj) );
    Value = Sfm_ObjRefIncrement(p, iObj);
    if ( Value > 1 )
        return 0;
    assert( Value == 1 );
    Count = 1;
    Sfm_ObjForEachFanin( p, iObj, iFanin, i )
        Count += Sfm_ObjRef_rec( p, iFanin );
    return Count;
}
int Sfm_ObjRef( Sfm_Ntk_t * p, int iObj )
{
    int i, iFanin, Count = 1;
    Sfm_ObjForEachFanin( p, iObj, iFanin, i )
        Count += Sfm_ObjRef_rec( p, iFanin );
    return Count;
}
int Sfm_ObjDeref_rec( Sfm_Ntk_t * p, int iObj )
{
    int i, iFanin, Value, Count;
    if ( Sfm_ObjIsPi(p, iObj) )
        return 0;
    assert( Sfm_ObjIsNode(p, iObj) );
    Value = Sfm_ObjRefDecrement(p, iObj);
    if ( Value > 0 )
        return 0;
    assert( Value == 0 );
    Count = 1;
    Sfm_ObjForEachFanin( p, iObj, iFanin, i )
        Count += Sfm_ObjDeref_rec( p, iFanin );
    return Count;
}
int Sfm_ObjDeref( Sfm_Ntk_t * p, int iObj )
{
    int i, iFanin, Count = 1;
    Sfm_ObjForEachFanin( p, iObj, iFanin, i )
        Count += Sfm_ObjDeref_rec( p, iFanin );
    return Count;
}
int Sfm_ObjMffcSize( Sfm_Ntk_t * p, int iObj )
{
    int Count1, Count2;
    if ( Sfm_ObjIsPi(p, iObj) )
        return 0;
    if ( Sfm_ObjFanoutNum(p, iObj) != 1 )
        return 0;
    assert( Sfm_ObjIsNode( p, iObj ) );
    Count1 = Sfm_ObjDeref( p, iObj );
    Count2 = Sfm_ObjRef( p, iObj );
    assert( Count1 == Count2 );
    return Count1;
}

/**Function*************************************************************

  Synopsis    [Working with traversal IDs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void  Sfm_NtkIncrementTravId( Sfm_Ntk_t * p )            { p->nTravIds++;                                            }       
static inline void  Sfm_ObjSetTravIdCurrent( Sfm_Ntk_t * p, int Id )   { Vec_IntWriteEntry( &p->vTravIds, Id, p->nTravIds );       }
static inline int   Sfm_ObjIsTravIdCurrent( Sfm_Ntk_t * p, int Id )    { return (Vec_IntEntry(&p->vTravIds, Id) == p->nTravIds);   }   
static inline int   Sfm_ObjIsTravIdPrevious( Sfm_Ntk_t * p, int Id )   { return (Vec_IntEntry(&p->vTravIds, Id) == p->nTravIds-1); }   

static inline void  Sfm_NtkIncrementTravId2( Sfm_Ntk_t * p )           { p->nTravIds2++;                                           }       
static inline void  Sfm_ObjSetTravIdCurrent2( Sfm_Ntk_t * p, int Id )  { Vec_IntWriteEntry( &p->vTravIds2, Id, p->nTravIds2 );     }
static inline int   Sfm_ObjIsTravIdCurrent2( Sfm_Ntk_t * p, int Id )   { return (Vec_IntEntry(&p->vTravIds2, Id) == p->nTravIds2); }   


/**Function*************************************************************

  Synopsis    [Collects used internal nodes in a topological order.]

  Description [Additionally considers objects in groups as a single object
  and collects them in a topological order together as single entity.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sfm_NtkDfs_rec( Sfm_Ntk_t * p, int iNode, Vec_Int_t * vNodes, Vec_Wec_t * vGroups, Vec_Int_t * vGroupMap, Vec_Int_t * vBoxesLeft )
{
    int i, iFanin;
    if ( Sfm_ObjIsPi(p, iNode) )
        return;
    if ( Sfm_ObjIsTravIdCurrent(p, iNode) )
        return;
    if ( Vec_IntEntry(vGroupMap, iNode) >= 0 )
    {
        int k, iGroup = Abc_Lit2Var( Vec_IntEntry(vGroupMap, iNode) );
        Vec_Int_t * vGroup = Vec_WecEntry( vGroups, iGroup );
        Vec_IntForEachEntry( vGroup, iNode, i )
            assert( Sfm_ObjIsNode(p, iNode) );
        Vec_IntForEachEntry( vGroup, iNode, i )
            Sfm_ObjSetTravIdCurrent( p, iNode );
        Vec_IntForEachEntry( vGroup, iNode, i )
            Sfm_ObjForEachFanin( p, iNode, iFanin, k )
                Sfm_NtkDfs_rec( p, iFanin, vNodes, vGroups, vGroupMap, vBoxesLeft );
        Vec_IntForEachEntry( vGroup, iNode, i )
            Vec_IntPush( vNodes, iNode );
        Vec_IntPush( vBoxesLeft, iGroup );
    }
    else
    {
        Sfm_ObjSetTravIdCurrent(p, iNode);
        Sfm_ObjForEachFanin( p, iNode, iFanin, i )
            Sfm_NtkDfs_rec( p, iFanin, vNodes, vGroups, vGroupMap, vBoxesLeft );
        Vec_IntPush( vNodes, iNode );
    }
}
Vec_Int_t * Sfm_NtkDfs( Sfm_Ntk_t * p, Vec_Wec_t * vGroups, Vec_Int_t * vGroupMap, Vec_Int_t * vBoxesLeft, int fAllBoxes )
{
    Vec_Int_t * vNodes;
    int i;
    Vec_IntClear( vBoxesLeft );
    vNodes = Vec_IntAlloc( p->nObjs );
    Sfm_NtkIncrementTravId( p );
    if ( fAllBoxes )
    {
        Vec_Int_t * vGroup;
        Vec_WecForEachLevel( vGroups, vGroup, i )
            Sfm_NtkDfs_rec( p, Vec_IntEntry(vGroup, 0), vNodes, vGroups, vGroupMap, vBoxesLeft );
    }
    Sfm_NtkForEachPo( p, i )
        Sfm_NtkDfs_rec( p, Sfm_ObjFanin(p, i, 0), vNodes, vGroups, vGroupMap, vBoxesLeft );
    return vNodes;
}

/**Function*************************************************************

  Synopsis    [Check if this fanout overlaps with TFI cone of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Sfm_NtkCheckOverlap_rec( Sfm_Ntk_t * p, int iThis, int iNode )
{
    int i, iFanin;
    if ( Sfm_ObjIsTravIdCurrent2(p, iThis) || iThis == iNode )
        return 0;
//    if ( Sfm_ObjIsTravIdCurrent(p, iThis) )
    if ( Sfm_ObjIsTravIdPrevious(p, iThis) )
        return 1;
    Sfm_ObjSetTravIdCurrent2(p, iThis);
    Sfm_ObjForEachFanin( p, iThis, iFanin, i )
        if ( Sfm_NtkCheckOverlap_rec(p, iFanin, iNode) )
            return 1;
    return 0;
}
int Sfm_NtkCheckOverlap( Sfm_Ntk_t * p, int iFan, int iNode )
{
    Sfm_NtkIncrementTravId2( p );
    return Sfm_NtkCheckOverlap_rec( p, iFan, iNode );
}

/**Function*************************************************************

  Synopsis    [Recursively collects roots of the window.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Sfm_NtkCheckRoot( Sfm_Ntk_t * p, int iNode, int nLevelMax )
{
    int i, iFanout;
    // the node is the root if one of the following is true:
    // (1) the node has more than fanouts than the limit or has no fanouts (should not happen in general)
    if ( Sfm_ObjFanoutNum(p, iNode) == 0 || Sfm_ObjFanoutNum(p, iNode) > p->pPars->nFanoutMax )
        return 1;
    // (2) the node has CO fanouts
    // (3) the node has fanouts above the cutoff level
    Sfm_ObjForEachFanout( p, iNode, iFanout, i )
        if ( Sfm_ObjIsPo(p, iFanout) || Sfm_ObjLevel(p, iFanout) > nLevelMax )//|| !Sfm_NtkCheckOverlap(p, iFanout, iNode) )
            return 1;
    return 0;
}
void Sfm_NtkComputeRoots_rec( Sfm_Ntk_t * p, int iNode, int nLevelMax, Vec_Int_t * vRoots, Vec_Int_t * vTfo )
{
    int i, iFanout;
    assert( Sfm_ObjIsNode(p, iNode) );
    if ( Sfm_ObjIsTravIdCurrent(p, iNode) )
        return;
    Sfm_ObjSetTravIdCurrent(p, iNode);
    if ( iNode != p->iPivotNode )
        Vec_IntPush( vTfo, iNode );
    // check if the node should be the root
    if ( Sfm_NtkCheckRoot( p, iNode, nLevelMax ) )
        Vec_IntPush( vRoots, iNode );
    else // if not, explore its fanouts
        Sfm_ObjForEachFanout( p, iNode, iFanout, i )
            Sfm_NtkComputeRoots_rec( p, iFanout, nLevelMax, vRoots, vTfo );
}

/**Function*************************************************************

  Synopsis    [Collects divisors of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sfm_NtkAddDivisors( Sfm_Ntk_t * p, int iNode, int nLevelMax )
{
    int i, iFanout;
    Sfm_ObjForEachFanout( p, iNode, iFanout, i )
    {
        // skip some of the fanouts if the number is large
        if ( p->pPars->nFanoutMax && i > p->pPars->nFanoutMax )
            return;
        // skip TFI nodes, PO nodes, or nodes with high logic level
        if ( Sfm_ObjIsTravIdCurrent(p, iFanout) || Sfm_ObjIsPo(p, iFanout) || Sfm_ObjLevel(p, iFanout) > nLevelMax )
            continue;
        // handle single-input nodes
        if ( Sfm_ObjFaninNum(p, iFanout) == 1 )
            Vec_IntPush( p->vDivs, iFanout );
        // visit node for the first time
        else if ( !Sfm_ObjIsTravIdCurrent2(p, iFanout) )
        {
            assert( Sfm_ObjFaninNum(p, iFanout) > 1 );
            Sfm_ObjSetTravIdCurrent2( p, iFanout );
            Sfm_ObjResetFaninCount( p, iFanout );
        }
        // visit node again
        else if ( Sfm_ObjUpdateFaninCount(p, iFanout) == 0 )
            Vec_IntPush( p->vDivs, iFanout );
    }
}

/**Function*************************************************************

  Synopsis    [Fixed object is useful when it has a non-fixed fanout.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Sfm_ObjIsUseful( Sfm_Ntk_t * p, int iNode )
{
    int i, iFanout;
    if ( !Sfm_ObjIsFixed(p, iNode) )
        return 1;
    Sfm_ObjForEachFanout( p, iNode, iFanout, i )
        if ( !Sfm_ObjIsFixed(p, iFanout) )
            return 1;
    return 0;
}

/**Function*************************************************************

  Synopsis    [Computes structural window.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Sfm_NtkCollectTfi_rec( Sfm_Ntk_t * p, int iNode, Vec_Int_t * vNodes )
{
    int i, iFanin;
    if ( Sfm_ObjIsTravIdCurrent( p, iNode ) )
        return 0;
    Sfm_ObjSetTravIdCurrent( p, iNode );
    Sfm_ObjForEachFanin( p, iNode, iFanin, i )
        if ( Sfm_NtkCollectTfi_rec( p, iFanin, vNodes ) )
            return 1;
    Vec_IntPush( vNodes, iNode );
    return p->pPars->nWinSizeMax && (Vec_IntSize(vNodes) > p->pPars->nWinSizeMax);
}
int Sfm_NtkCreateWindow( Sfm_Ntk_t * p, int iNode, int fVerbose )
{
    int i, k, iTemp;
    abctime clkDiv, clkWin = Abc_Clock();

    assert( Sfm_ObjIsNode( p, iNode ) );
    p->iPivotNode = iNode;
    Vec_IntClear( p->vNodes );  // internal
    Vec_IntClear( p->vDivs );   // divisors
    Vec_IntClear( p->vRoots );  // roots
    Vec_IntClear( p->vTfo );    // roots
    Vec_IntClear( p->vOrder );  // variable order

    // collect transitive fanin
    Sfm_NtkIncrementTravId( p );
    if ( Sfm_NtkCollectTfi_rec( p, iNode, p->vNodes ) )
    {
        p->nMaxDivs++;
        p->timeWin += Abc_Clock() - clkWin;
        return 0;
    }

    // create divisors
    clkDiv = Abc_Clock();
    Vec_IntClear( p->vDivs );
    Vec_IntAppend( p->vDivs, p->vNodes );
    Vec_IntPop( p->vDivs );
    // add non-topological divisors
    if ( Vec_IntSize(p->vDivs) < p->pPars->nWinSizeMax + 0 )
    {
        Sfm_NtkIncrementTravId2( p );
        Vec_IntForEachEntry( p->vDivs, iTemp, i )
            if ( Vec_IntSize(p->vDivs) < p->pPars->nWinSizeMax + 0 )
//                Sfm_NtkAddDivisors( p, iTemp, Sfm_ObjLevel(p, iNode) - 1 );
                Sfm_NtkAddDivisors( p, iTemp, p->nLevelMax - Sfm_ObjLevelR(p, iNode) ); 
    }
    if ( Vec_IntSize(p->vDivs) > p->pPars->nWinSizeMax )
    {
/*
        k = 0;
        Vec_IntForEachEntryStart( p->vDivs, iTemp, i, Vec_IntSize(p->vDivs) - p->pPars->nWinSizeMax )
            Vec_IntWriteEntry( p->vDivs, k++, iTemp );
        assert( k == p->pPars->nWinSizeMax );
*/
        Vec_IntShrink( p->vDivs, p->pPars->nWinSizeMax );
    }
    assert( Vec_IntSize(p->vDivs) <= p->pPars->nWinSizeMax );
    p->nMaxDivs += (int)(Vec_IntSize(p->vDivs) == p->pPars->nWinSizeMax);
    // remove node/fanins from divisors
    // mark fanins
    Sfm_NtkIncrementTravId2( p );
    Sfm_ObjSetTravIdCurrent2( p, iNode );
    Sfm_ObjForEachFanin( p, iNode, iTemp, i )
        Sfm_ObjSetTravIdCurrent2( p, iTemp );
    // compact divisors
    k = 0;
    Vec_IntForEachEntry( p->vDivs, iTemp, i )
        if ( !Sfm_ObjIsTravIdCurrent2(p, iTemp) && Sfm_ObjIsUseful(p, iTemp) )
            Vec_IntWriteEntry( p->vDivs, k++, iTemp );
    Vec_IntShrink( p->vDivs, k );
    assert( Vec_IntSize(p->vDivs) <= p->pPars->nWinSizeMax );
    clkDiv = Abc_Clock() - clkDiv;
    p->timeDiv += clkDiv;
    p->nTotalDivs += Vec_IntSize(p->vDivs);
 
    // collect TFO and window roots
    if ( p->pPars->nTfoLevMax > 0 && !Sfm_NtkCheckRoot(p, iNode, Sfm_ObjLevel(p, iNode) + p->pPars->nTfoLevMax) )
    {
        // explore transitive fanout
        Sfm_NtkIncrementTravId( p );
        Sfm_NtkComputeRoots_rec( p, iNode, Sfm_ObjLevel(p, iNode) + p->pPars->nTfoLevMax, p->vRoots, p->vTfo );
        assert( Vec_IntSize(p->vRoots) > 0 );
        assert( Vec_IntSize(p->vTfo) > 0 );
        // compute new leaves and nodes
        Sfm_NtkIncrementTravId( p );
        Vec_IntForEachEntry( p->vRoots, iTemp, i )
            if ( Sfm_NtkCollectTfi_rec( p, iTemp, p->vOrder ) )
            {
                Vec_IntClear( p->vRoots );
                Vec_IntClear( p->vTfo );
                Vec_IntClear( p->vOrder );
                break;
            }
        if ( Vec_IntSize(p->vRoots) > 0 )
        Vec_IntForEachEntry( p->vTfo, iTemp, i )
            if ( Sfm_NtkCollectTfi_rec( p, iTemp, p->vOrder ) )
            {
                Vec_IntClear( p->vRoots );
                Vec_IntClear( p->vTfo );
                Vec_IntClear( p->vOrder );
                break;
            }
        if ( Vec_IntSize(p->vRoots) > 0 )
        Vec_IntForEachEntry( p->vDivs, iTemp, i )
            if ( Sfm_NtkCollectTfi_rec( p, iTemp, p->vOrder ) )
            {
                Vec_IntClear( p->vRoots );
                Vec_IntClear( p->vTfo );
                Vec_IntClear( p->vOrder );
                break;
            }
    }

    if ( Vec_IntSize(p->vOrder) == 0 )
    {
        int Temp = p->pPars->nWinSizeMax;
        p->pPars->nWinSizeMax = 0;
        Sfm_NtkIncrementTravId( p );
        Sfm_NtkCollectTfi_rec( p, iNode, p->vOrder );
        Vec_IntForEachEntry( p->vDivs, iTemp, i )
            Sfm_NtkCollectTfi_rec( p, iTemp, p->vOrder );
        p->pPars->nWinSizeMax = Temp;
    }

    // statistics
    p->timeWin += Abc_Clock() - clkWin - clkDiv;
    if ( !fVerbose )
        return 1;

    // print stats about the window
    printf( "%6d : ", iNode );
    printf( "Leaves = %5d. ", 0 );
    printf( "Nodes = %5d. ",  Vec_IntSize(p->vNodes) );
    printf( "Roots = %5d. ",  Vec_IntSize(p->vRoots) );
    printf( "Divs = %5d. ",   Vec_IntSize(p->vDivs) );
    printf( "\n" );
    return 1;
}
void Sfm_NtkWindowTest( Sfm_Ntk_t * p, int iNode )
{
    int i;
    Sfm_NtkForEachNode( p, i )
        Sfm_NtkCreateWindow( p, i, 1 );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

