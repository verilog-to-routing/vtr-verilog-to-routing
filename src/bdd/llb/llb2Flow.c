/**CFile****************************************************************

  FileName    [llb2Flow.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [BDD based reachability.]

  Synopsis    [Flow computation.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: llb2Flow.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "llbInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static inline int         Llb_ObjSetPath( Aig_Obj_t * pObj, Aig_Obj_t * pNext ) { pObj->pData = (void *)pNext; return 1;  }
static inline Aig_Obj_t * Llb_ObjGetPath( Aig_Obj_t * pObj )                    { return (Aig_Obj_t *)pObj->pData;        }
static inline Aig_Obj_t * Llb_ObjGetFanoutPath( Aig_Man_t * p, Aig_Obj_t * pObj ) 
{ 
    Aig_Obj_t * pFanout;
    int i, iFanout = -1;
    assert( Llb_ObjGetPath(pObj) ); 
    Aig_ObjForEachFanout( p, pObj, pFanout, iFanout, i )
        if ( Llb_ObjGetPath(pFanout) == pObj )
            return pFanout;
    return NULL;
}

extern Vec_Ptr_t * Llb_ManCutSupp( Aig_Man_t * p, Vec_Ptr_t * vLower, Vec_Ptr_t * vUpper );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////


/**Function*************************************************************

  Synopsis    [For each cut, returns PIs that can be quantified.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Llb_ManCutSupps( Aig_Man_t * p, Vec_Ptr_t * vResult )
{
    Vec_Ptr_t * vSupps, * vOne, * vLower, * vUpper;
    int i;
    vSupps = Vec_PtrAlloc( 100 );
    Vec_PtrPush( vSupps, Vec_PtrAlloc(0) );
    vLower = (Vec_Ptr_t *)Vec_PtrEntry( vResult, 0 );
    Vec_PtrForEachEntryStart( Vec_Ptr_t *, vResult, vUpper, i, 1 )
    {
        vOne  = Llb_ManCutSupp( p, vLower, vUpper );
        Vec_PtrPush( vSupps, vOne );
        vLower = vUpper;
    }
    assert( Vec_PtrSize(vSupps) == Vec_PtrSize(vResult) );
    return vSupps;
}

/**Function*************************************************************

  Synopsis    [For each cut, returns PIs that can be quantified.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Llb_ManCutMap( Aig_Man_t * p, Vec_Ptr_t * vResult, Vec_Ptr_t * vSupps )
{
    int fShowMatrix = 1;
    Vec_Ptr_t * vMaps, * vOne;
    Vec_Int_t * vMap, * vPrev, * vNext;
    Aig_Obj_t * pObj;
    int * piFirst, * piLast;
    int i, k, CounterPlus, CounterMinus, Counter;

    vMaps = Vec_PtrAlloc( 100 );
    Vec_PtrForEachEntry( Vec_Ptr_t *, vResult, vOne, i )
    {
        vMap = Vec_IntStart( Aig_ManObjNumMax(p) );
        Vec_PtrForEachEntry( Aig_Obj_t *, vOne, pObj, k )
        {
            if ( !Saig_ObjIsPi(p, pObj) )
                Vec_IntWriteEntry( vMap, pObj->Id, 1 );
//            else
//printf( "*" );
//printf( "%d ", pObj->Id );
        }
        Vec_PtrPush( vMaps, vMap );
//printf( "\n" );
    }
    Vec_PtrPush( vMaps, Vec_IntStart( Aig_ManObjNumMax(p) ) );
    assert( Vec_PtrSize(vMaps) == Vec_PtrSize(vResult)+1 );

    // collect the first and last PIs
    piFirst = ABC_ALLOC( int, Saig_ManPiNum(p) );
    piLast  = ABC_ALLOC( int, Saig_ManPiNum(p) );
    Saig_ManForEachPi( p, pObj, i )
        piFirst[i] = piLast[i] = -1;
    Vec_PtrForEachEntry( Vec_Ptr_t *, vSupps, vOne, i )
    {
        Vec_PtrForEachEntry( Aig_Obj_t *, vOne, pObj, k )
        {
            if ( !Saig_ObjIsPi(p, pObj) )
                continue;
            if ( piFirst[Aig_ObjCioId(pObj)] == -1 )
                 piFirst[Aig_ObjCioId(pObj)] = i;
            piLast[Aig_ObjCioId(pObj)] = i;
        }
    }
    // PIs feeding into the flops should be extended to the last frame
    Saig_ManForEachLi( p, pObj, i )
    {
        if ( !Saig_ObjIsPi(p, Aig_ObjFanin0(pObj)) )
            continue;
        piLast[Aig_ObjCioId(Aig_ObjFanin0(pObj))] = Vec_PtrSize(vMaps)-1;
    }

    // set the PI map
    Saig_ManForEachPi( p, pObj, i )
    {
        if ( piFirst[i] == -1 )
            continue;
        if ( piFirst[i] == piLast[i] )
        {
            vMap = (Vec_Int_t *)Vec_PtrEntry( vMaps, piFirst[i] );
            Vec_IntWriteEntry( vMap, pObj->Id, 2 );
            continue;
        }

        // set support for all in between        
        for ( k = piFirst[i]; k <= piLast[i]; k++ )
        {
            vMap = (Vec_Int_t *)Vec_PtrEntry( vMaps, k );
            Vec_IntWriteEntry( vMap, pObj->Id, 1 );
        }
    }
    ABC_FREE( piFirst );
    ABC_FREE( piLast );


    // find all that will appear here
    Counter = Aig_ManRegNum(p);
    printf( "%d ", Counter );
    Vec_PtrForEachEntryStart( Vec_Int_t *, vMaps, vMap, i, 1 )
    {
        vPrev = (Vec_Int_t *)Vec_PtrEntry( vMaps, i-1 );
        vNext = (i == Vec_PtrSize(vMaps)-1)? NULL: (Vec_Int_t *)Vec_PtrEntry( vMaps, i+1 );

        CounterPlus = CounterMinus = 0;
        Aig_ManForEachObj( p, pObj, k )
        {
            if ( Saig_ObjIsPi(p, pObj) )
            {
                if ( Vec_IntEntry(vPrev, k) == 0 && Vec_IntEntry(vMap, k) == 1 )
                    CounterPlus++;
                if ( Vec_IntEntry(vMap, k) == 1 && (vNext == NULL || Vec_IntEntry(vNext, k) == 0) )
                    CounterMinus++;
            }
            else
            {
                if ( Vec_IntEntry(vPrev, k) == 0 && Vec_IntEntry(vMap, k) == 1 )
                    CounterPlus++;
                if ( Vec_IntEntry(vPrev, k) == 1 && Vec_IntEntry(vMap, k) == 0 )
                    CounterMinus++;
            }
        }
        Counter = Counter + CounterPlus - CounterMinus;
        printf( "%d=%d ", i, Counter );
    }
    printf( "\n" );

    if ( !fShowMatrix )
        return vMaps;
    Aig_ManForEachObj( p, pObj, i )
    {
        if ( !Aig_ObjIsCi(pObj) && !Aig_ObjIsNode(pObj) )
            continue;
        Vec_PtrForEachEntry( Vec_Int_t *, vMaps, vMap, k )
            if ( Vec_IntEntry(vMap, i) )
                break;
        if ( k == Vec_PtrSize(vMaps) )
            continue;
        printf( "Obj = %4d : ", i );
        if ( Saig_ObjIsPi(p,pObj) )
            printf( "pi  " );
        else if ( Saig_ObjIsLo(p,pObj) )
            printf( "lo  " );
        else if ( Aig_ObjIsNode(pObj) )
            printf( "and " );

        Vec_PtrForEachEntry( Vec_Int_t *, vMaps, vMap, k )
            printf( "%d", Vec_IntEntry(vMap, i) );
        printf( "\n" );
    }
    return vMaps;
}

/**Function*************************************************************

  Synopsis    [Counts the number of PIs in the cut]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Llb_ManCutPiNum( Aig_Man_t * p, Vec_Ptr_t * vMinCut )
{
    Aig_Obj_t * pObj;
    int i, Counter = 0;
    Vec_PtrForEachEntry( Aig_Obj_t *, vMinCut, pObj, i )
        if ( Saig_ObjIsPi(p,pObj) )
            Counter++;
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Counts the number of LOs in the cut]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Llb_ManCutLoNum( Aig_Man_t * p, Vec_Ptr_t * vMinCut )
{
    Aig_Obj_t * pObj;
    int i, Counter = 0;
    Vec_PtrForEachEntry( Aig_Obj_t *, vMinCut, pObj, i )
        if ( Saig_ObjIsLo(p,pObj) )
            Counter++;
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Counts the number of LIs in the cut]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Llb_ManCutLiNum( Aig_Man_t * p, Vec_Ptr_t * vMinCut )
{
    Aig_Obj_t * pFanout;
    Aig_Obj_t * pObj;
    int i, k, iFanout = -1, Counter = 0;
    Vec_PtrForEachEntry( Aig_Obj_t *, vMinCut, pObj, i )
    {
        if ( Aig_ObjIsCi(pObj) )
            continue;
        Aig_ObjForEachFanout( p, pObj, pFanout, iFanout, k )
        {
            if ( Saig_ObjIsLi(p, pFanout) )
            {
                Counter++;
                break;
            }
        }
    }
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Computes volume of the cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Llb_ManCutVolume_rec( Aig_Man_t * p, Aig_Obj_t * pObj )
{
    if ( Aig_ObjIsTravIdCurrent(p, pObj) )
        return 0;
    Aig_ObjSetTravIdCurrent(p, pObj);
    assert( Aig_ObjIsNode(pObj) );
    return 1 + Llb_ManCutVolume_rec(p, Aig_ObjFanin0(pObj)) + 
        Llb_ManCutVolume_rec(p, Aig_ObjFanin1(pObj));
}

/**Function*************************************************************

  Synopsis    [Computes volume of the cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Llb_ManCutVolume( Aig_Man_t * p, Vec_Ptr_t * vLower, Vec_Ptr_t * vUpper )
{
    Aig_Obj_t * pObj;
    int i, Counter = 0;
    // mark the lower cut with the traversal ID
    Aig_ManIncrementTravId(p);
    Vec_PtrForEachEntry( Aig_Obj_t *, vLower, pObj, i )
        Aig_ObjSetTravIdCurrent( p, pObj );
    // count the upper cut
    Vec_PtrForEachEntry( Aig_Obj_t *, vUpper, pObj, i )
        Counter += Llb_ManCutVolume_rec( p, pObj );
    return Counter;
}


/**Function*************************************************************

  Synopsis    [Computes volume of the cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Llb_ManCutNodes_rec( Aig_Man_t * p, Aig_Obj_t * pObj, Vec_Ptr_t * vNodes )
{
    if ( Aig_ObjIsTravIdCurrent(p, pObj) )
        return;
    Aig_ObjSetTravIdCurrent(p, pObj);
    assert( Aig_ObjIsNode(pObj) );
    Llb_ManCutNodes_rec(p, Aig_ObjFanin0(pObj), vNodes);
    Llb_ManCutNodes_rec(p, Aig_ObjFanin1(pObj), vNodes);
    Vec_PtrPush( vNodes, pObj );
}

/**Function*************************************************************

  Synopsis    [Computes volume of the cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Llb_ManCutNodes( Aig_Man_t * p, Vec_Ptr_t * vLower, Vec_Ptr_t * vUpper )
{
    Vec_Ptr_t * vNodes;
    Aig_Obj_t * pObj;
    int i;
    // mark the lower cut with the traversal ID
    Aig_ManIncrementTravId(p);
    Vec_PtrForEachEntry( Aig_Obj_t *, vLower, pObj, i )
        Aig_ObjSetTravIdCurrent( p, pObj );
    // count the upper cut
    vNodes = Vec_PtrAlloc( 100 );
    Vec_PtrForEachEntry( Aig_Obj_t *, vUpper, pObj, i )
        Llb_ManCutNodes_rec( p, pObj, vNodes );
    return vNodes;
}

/**Function*************************************************************

  Synopsis    [Computes volume of the cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Llb_ManCutSupp( Aig_Man_t * p, Vec_Ptr_t * vLower, Vec_Ptr_t * vUpper )
{
    Vec_Ptr_t * vNodes, * vSupp;
    Aig_Obj_t * pObj;
    int i;
    vNodes = Llb_ManCutNodes( p, vLower, vUpper );
    // mark support of the nodes
    Aig_ManIncrementTravId(p);
    Vec_PtrForEachEntry( Aig_Obj_t *, vNodes, pObj, i )
    {
        assert( Aig_ObjIsNode(pObj) );
        Aig_ObjSetTravIdCurrent( p, Aig_ObjFanin0(pObj) );
        Aig_ObjSetTravIdCurrent( p, Aig_ObjFanin1(pObj) );
    }
    Vec_PtrFree( vNodes );
    // collect the support nodes
    vSupp = Vec_PtrAlloc( 100 );
    Vec_PtrForEachEntry( Aig_Obj_t *, vLower, pObj, i )
        if ( Aig_ObjIsTravIdCurrent(p, pObj) )
            Vec_PtrPush( vSupp, pObj );
    return vSupp;

}

/**Function*************************************************************

  Synopsis    [Computes volume of the cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Llb_ManCutRange( Aig_Man_t * p, Vec_Ptr_t * vLower, Vec_Ptr_t * vUpper )
{
    Vec_Ptr_t * vRange;
    Aig_Obj_t * pObj;
    int i;
    // mark the lower cut with the traversal ID
    Aig_ManIncrementTravId(p);
    Vec_PtrForEachEntry( Aig_Obj_t *, vLower, pObj, i )
        Aig_ObjSetTravIdCurrent( p, pObj );
    // collect the upper ones that are not marked
    vRange = Vec_PtrAlloc( 100 );
    Vec_PtrForEachEntry( Aig_Obj_t *, vUpper, pObj, i )
        if ( !Aig_ObjIsTravIdCurrent(p, pObj) )
            Vec_PtrPush( vRange, pObj );
    return vRange;
}




/**Function*************************************************************

  Synopsis    [Prints the given cluster.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Llb_ManCutPrint( Aig_Man_t * p, Vec_Ptr_t * vLower, Vec_Ptr_t * vUpper )
{
    Vec_Ptr_t * vSupp, * vRange;
    int Pis, Ffs, And;

    Pis = Llb_ManCutPiNum(p, vLower);
    Ffs = Llb_ManCutLoNum(p, vLower);
    And = Vec_PtrSize(vLower) - Pis - Ffs;
    printf( "Leaf: %3d=%3d+%3d+%3d  ",  Vec_PtrSize(vLower), Pis, Ffs, And );

    Pis = Llb_ManCutPiNum(p, vUpper);
    Ffs = Llb_ManCutLiNum(p, vUpper);
    And = Vec_PtrSize(vUpper) - Pis - Ffs;
    printf( "Root: %3d=%3d+%3d+%3d  ", Vec_PtrSize(vUpper), Pis, Ffs, And );

    vSupp = Llb_ManCutSupp( p, vLower, vUpper );
    Pis = Llb_ManCutPiNum(p, vSupp);
    Ffs = Llb_ManCutLoNum(p, vSupp);
    And = Vec_PtrSize(vSupp) - Pis - Ffs;
    printf( "Supp: %3d=%3d+%3d+%3d  ", Vec_PtrSize(vSupp), Pis, Ffs, And );

    vRange = Llb_ManCutRange( p, vLower, vUpper );
    Pis = Llb_ManCutPiNum(p, vRange);
    Ffs = Llb_ManCutLiNum(p, vRange);
    And = Vec_PtrSize(vRange) - Pis - Ffs;
    printf( "Range: %3d=%3d+%3d+%3d  ", Vec_PtrSize(vRange), Pis, Ffs, And );

    printf( "S =%3d. V =%3d.\n", 
        Vec_PtrSize(vSupp)+Vec_PtrSize(vRange), Llb_ManCutVolume(p, vLower, vUpper) );
    Vec_PtrFree( vSupp );
    Vec_PtrFree( vRange );
/*   
    {
        Aig_Obj_t * pObj;
        int i;
        printf( "Lower: " );
        Vec_PtrForEachEntry( Aig_Obj_t *, vLower, pObj, i )
            printf( " %d", pObj->Id );
        printf( "     " );
        printf( "Upper: " );
        Vec_PtrForEachEntry( Aig_Obj_t *, vUpper, pObj, i )
            printf( " %d", pObj->Id );
        printf( "\n" );
    }
*/
}

/**Function*************************************************************

  Synopsis    [Prints the given cluster.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Llb_ManResultPrint( Aig_Man_t * p, Vec_Ptr_t * vResult )
{
    Vec_Ptr_t * vLower, * vUpper = NULL;
    int i;
    Vec_PtrForEachEntryReverse( Vec_Ptr_t *, vResult, vLower, i )
    {
        if ( i < Vec_PtrSize(vResult) - 1 )
            Llb_ManCutPrint( p, vLower, vUpper );
        vUpper = vLower;
    }
}

/**Function*************************************************************

  Synopsis    [Tries to find an augmenting path originating in this node.]

  Description [This procedure works for directed graphs only!]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Llb_ManFlowBwdPath2_rec( Aig_Man_t * p, Aig_Obj_t * pObj )
{
    Aig_Obj_t * pFanout;
    assert( Aig_ObjIsNode(pObj) || Aig_ObjIsCi(pObj) || Aig_ObjIsConst1(pObj) );
    // skip visited nodes
    if ( Aig_ObjIsTravIdCurrent(p, pObj) )
        return 0;
    Aig_ObjSetTravIdCurrent(p, pObj);
    // process node without flow
    if ( !Llb_ObjGetPath(pObj) )
    {
        // start the path if we reached a terminal node
        if ( pObj->fMarkA )
            return Llb_ObjSetPath( pObj, (Aig_Obj_t *)1 );
        // explore the fanins
//        Abc_ObjForEachFanin( pObj, pFanin, i )
//            if ( Abc_NtkMaxFlowBwdPath2_rec(pFanin) )
//                return Abc_ObjSetPath( pObj, pFanin );
        if ( Aig_ObjIsNode(pObj) )
        {
            if ( Llb_ManFlowBwdPath2_rec( p, Aig_ObjFanin0(pObj) ) )
                return Llb_ObjSetPath( pObj, Aig_ObjFanin0(pObj) );
            if ( Llb_ManFlowBwdPath2_rec( p, Aig_ObjFanin1(pObj) ) )
                return Llb_ObjSetPath( pObj, Aig_ObjFanin1(pObj) );
        }
        return 0;
    }
    // pObj has flow - find the fanout with flow
    pFanout = Llb_ObjGetFanoutPath( p, pObj );
    if ( pFanout == NULL )
        return 0;
    // go through the fanins of the fanout with flow
//    Abc_ObjForEachFanin( pFanout, pFanin, i )
//        if ( Abc_NtkMaxFlowBwdPath2_rec( pFanin ) )
//            return Abc_ObjSetPath( pFanout, pFanin );
    assert( Aig_ObjIsNode(pFanout) );
    if ( Llb_ManFlowBwdPath2_rec( p, Aig_ObjFanin0(pFanout) ) )
        return Llb_ObjSetPath( pFanout, Aig_ObjFanin0(pFanout) );
    if ( Llb_ManFlowBwdPath2_rec( p, Aig_ObjFanin1(pFanout) ) )
        return Llb_ObjSetPath( pFanout, Aig_ObjFanin1(pFanout) );
    // try the fanout
    if ( Llb_ManFlowBwdPath2_rec( p, pFanout ) )
        return Llb_ObjSetPath( pFanout, NULL );
    return 0;
}


/**Function*************************************************************

  Synopsis    [Cleans markB.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Llb_ManFlowLabelTfi_rec( Aig_Man_t * p, Aig_Obj_t * pObj )
{
    if ( Aig_ObjIsTravIdCurrent(p, pObj) )
        return;
    Aig_ObjSetTravIdCurrent(p, pObj);
    if ( Aig_ObjIsCi(pObj) || Aig_ObjIsConst1(pObj) )
        return;
    assert( Aig_ObjIsNode(pObj) );
    Llb_ManFlowLabelTfi_rec( p, Aig_ObjFanin0(pObj) );
    Llb_ManFlowLabelTfi_rec( p, Aig_ObjFanin1(pObj) );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Llb_ManFlowUpdateCut( Aig_Man_t * p, Vec_Ptr_t * vMinCut )
{
    Aig_Obj_t * pObj;
    int i;
    // label the TFI of the cut nodes
    Aig_ManIncrementTravId(p);
    Vec_PtrForEachEntry( Aig_Obj_t *, vMinCut, pObj, i )
        Llb_ManFlowLabelTfi_rec( p, pObj );
    // collect labeled fanins of non-labeled nodes
    Vec_PtrClear( vMinCut );
    Aig_ManIncrementTravId(p);
    Aig_ManForEachObj( p, pObj, i )
    {
        if ( !Aig_ObjIsCo(pObj) && !Aig_ObjIsNode(pObj) )
            continue;
        if ( Aig_ObjIsTravIdCurrent(p, pObj) || Aig_ObjIsTravIdPrevious(p, pObj) )
            continue;
        if ( Aig_ObjIsTravIdPrevious(p, Aig_ObjFanin0(pObj)) )
        {
            Aig_ObjSetTravIdCurrent(p, Aig_ObjFanin0(pObj));
            Vec_PtrPush( vMinCut, Aig_ObjFanin0(pObj) );
        }
        if ( Aig_ObjIsNode(pObj) && Aig_ObjIsTravIdPrevious(p, Aig_ObjFanin1(pObj)) )
        {
            Aig_ObjSetTravIdCurrent(p, Aig_ObjFanin1(pObj));
            Vec_PtrPush( vMinCut, Aig_ObjFanin1(pObj) );
        }
    }
}

/**Function*************************************************************

  Synopsis    [Find minimum-volume minumum cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Llb_ManFlowMinCut( Aig_Man_t * p )
{
    Vec_Ptr_t * vMinCut;
    Aig_Obj_t * pObj;
    int i;
    // collect the cut nodes
    vMinCut = Vec_PtrAlloc( Aig_ManRegNum(p) );
    Aig_ManForEachObj( p, pObj, i )
    {
        // node without flow is not a cut node
        if ( !Llb_ObjGetPath(pObj) )
            continue;
        // unvisited node is below the cut
        if ( !Aig_ObjIsTravIdCurrent(p, pObj) )
            continue;
        // add terminal with flow or node whose path is not visited
        if ( pObj->fMarkA || !Aig_ObjIsTravIdCurrent( p, Llb_ObjGetPath(pObj) ) )
            Vec_PtrPush( vMinCut, pObj );
    }
    return vMinCut;
}

/**Function*************************************************************

  Synopsis    [Verifies the min-cut is indeed a cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Llb_ManFlowVerifyCut_rec( Aig_Man_t * p, Aig_Obj_t * pObj )
{
    // skip visited nodes
    if ( Aig_ObjIsTravIdCurrent(p, pObj) )
        return 1;
    Aig_ObjSetTravIdCurrent(p, pObj);
    // visit the node
    if ( Aig_ObjIsConst1(pObj) )
        return 1;
    if ( Aig_ObjIsCi(pObj) )
        return 0;
    // explore the fanins
    assert( Aig_ObjIsNode(pObj) );
    if ( !Llb_ManFlowVerifyCut_rec(p, Aig_ObjFanin0(pObj)) )
        return 0;
    if ( !Llb_ManFlowVerifyCut_rec(p, Aig_ObjFanin1(pObj)) )
        return 0;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Verifies the min-cut is indeed a cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Llb_ManFlowVerifyCut( Aig_Man_t * p, Vec_Ptr_t * vMinCut )
{
    Aig_Obj_t * pObj;
    int i;
    // mark the cut with the current traversal ID
    Aig_ManIncrementTravId(p);
    Vec_PtrForEachEntry( Aig_Obj_t *, vMinCut, pObj, i )
        Aig_ObjSetTravIdCurrent( p, pObj );
    // search from the latches for a path to the COs/CIs
    Saig_ManForEachLi( p, pObj, i )
    {
        if ( !Llb_ManFlowVerifyCut_rec( p, Aig_ObjFanin0(pObj) ) )
            return 0;
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    [Implementation of max-flow/min-cut computation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Llb_ManFlow( Aig_Man_t * p, Vec_Ptr_t * vSources, int * pnFlow )
{
    Vec_Ptr_t * vMinCut;
    Aig_Obj_t * pObj;
    int Flow, FlowCur, RetValue, i;
    // find the max-flow
    Flow = 0;
    Aig_ManCleanData( p );
    Aig_ManIncrementTravId(p);
    Vec_PtrForEachEntry( Aig_Obj_t *, vSources, pObj, i )
    {
        assert( !pObj->fMarkA && pObj->fMarkB );
        if ( !Aig_ObjFanin0(pObj)->fMarkB )
        {
            FlowCur  = Llb_ManFlowBwdPath2_rec( p, Aig_ObjFanin0(pObj) );
            Flow    += FlowCur;
            if ( FlowCur )
                Aig_ManIncrementTravId(p);
        }
        if ( Aig_ObjIsNode(pObj) && !Aig_ObjFanin1(pObj)->fMarkB )
        {
            FlowCur  = Llb_ManFlowBwdPath2_rec( p, Aig_ObjFanin1(pObj) );
            Flow    += FlowCur;
            if ( FlowCur )
                Aig_ManIncrementTravId(p);
        }
    }
    if ( pnFlow )
        *pnFlow = Flow;

    // mark the nodes reachable from the latches
    Aig_ManIncrementTravId(p);
    Vec_PtrForEachEntry( Aig_Obj_t *, vSources, pObj, i )
    {
        assert( !pObj->fMarkA && pObj->fMarkB );
        if ( !Aig_ObjFanin0(pObj)->fMarkB )
        {
            RetValue = Llb_ManFlowBwdPath2_rec( p, Aig_ObjFanin0(pObj) );
            assert( RetValue == 0 );
        }
        if ( Aig_ObjIsNode(pObj) && !Aig_ObjFanin1(pObj)->fMarkB )
        {
            RetValue = Llb_ManFlowBwdPath2_rec( p, Aig_ObjFanin1(pObj) );
            assert( RetValue == 0 );
        }
    }

    // find the min-cut with the smallest volume
    vMinCut = Llb_ManFlowMinCut( p );
    assert( Vec_PtrSize(vMinCut) == Flow );
    // verify the cut
    if ( !Llb_ManFlowVerifyCut(p, vMinCut) )
        printf( "Llb_ManFlow() error! The computed min-cut is not a cut!\n" );
//    Llb_ManFlowPrintCut( p, vMinCut );
    return vMinCut;
}

/**Function*************************************************************

  Synopsis    [Implementation of max-flow/min-cut computation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Llb_ManFlowCompute( Aig_Man_t * p )
{
    Vec_Ptr_t * vMinCut;
    Aig_Obj_t * pObj;
    int Flow, FlowCur, RetValue, i;
    // find the max-flow
    Flow = 0;
    Aig_ManCleanData( p );
    Aig_ManIncrementTravId(p);
    Aig_ManForEachObj( p, pObj, i )
    {
        if ( !pObj->fMarkB )
            continue;
        assert( !pObj->fMarkA );
        if ( !Aig_ObjFanin0(pObj)->fMarkB )
        {
//printf( "%d ", Aig_ObjFanin0(pObj)->Id );
            FlowCur  = Llb_ManFlowBwdPath2_rec( p, Aig_ObjFanin0(pObj) );
            Flow    += FlowCur;
            if ( FlowCur )
                Aig_ManIncrementTravId(p);
        }
        if ( Aig_ObjIsNode(pObj) && !Aig_ObjFanin1(pObj)->fMarkB )
        {
//printf( "%d ", Aig_ObjFanin1(pObj)->Id );
            FlowCur  = Llb_ManFlowBwdPath2_rec( p, Aig_ObjFanin1(pObj) );
            Flow    += FlowCur;
            if ( FlowCur )
                Aig_ManIncrementTravId(p);
        }
    }
//printf( "\n" );

    // mark the nodes reachable from the latches
    Aig_ManIncrementTravId(p);
    Aig_ManForEachObj( p, pObj, i )
    {
        if ( !pObj->fMarkB )
            continue;
        assert( !pObj->fMarkA );
        if ( !Aig_ObjFanin0(pObj)->fMarkB )
        {
            RetValue = Llb_ManFlowBwdPath2_rec( p, Aig_ObjFanin0(pObj) );
            assert( RetValue == 0 );
        }
        if ( Aig_ObjIsNode(pObj) && !Aig_ObjFanin1(pObj)->fMarkB )
        {
            RetValue = Llb_ManFlowBwdPath2_rec( p, Aig_ObjFanin1(pObj) );
            assert( RetValue == 0 );
        }
    }
    // find the min-cut with the smallest volume
    vMinCut = Llb_ManFlowMinCut( p );
    assert( Vec_PtrSize(vMinCut) == Flow );
//printf( "%d ", Vec_PtrSize(vMinCut) );
    Llb_ManFlowUpdateCut( p, vMinCut );
//printf( "%d   ", Vec_PtrSize(vMinCut) );
    // verify the cut
    if ( !Llb_ManFlowVerifyCut(p, vMinCut) )
        printf( "Llb_ManFlow() error! The computed min-cut is not a cut!\n" );
//    Llb_ManFlowPrintCut( p, vMinCut );
    return vMinCut;
}




/**Function*************************************************************

  Synopsis    [Cleans markB.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Llb_ManFlowCleanMarkB_rec( Aig_Obj_t * pObj )
{
    if ( pObj->fMarkB == 0 )
        return;
    pObj->fMarkB = 0;
    assert( Aig_ObjIsNode(pObj) );
    Llb_ManFlowCleanMarkB_rec( Aig_ObjFanin0(pObj) );
    Llb_ManFlowCleanMarkB_rec( Aig_ObjFanin1(pObj) );
}

/**Function*************************************************************

  Synopsis    [Cleans markB.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Llb_ManFlowSetMarkA_rec( Aig_Obj_t * pObj )
{
    if ( pObj->fMarkA )
        return;
    pObj->fMarkA = 1;
    if ( Aig_ObjIsCi(pObj) || Aig_ObjIsConst1(pObj) )
        return;
    assert( Aig_ObjIsNode(pObj) );
    Llb_ManFlowSetMarkA_rec( Aig_ObjFanin0(pObj) );
    Llb_ManFlowSetMarkA_rec( Aig_ObjFanin1(pObj) );
}

/**Function*************************************************************

  Synopsis    [Prepares flow computation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Llb_ManFlowPrepareCut( Aig_Man_t * p, Vec_Ptr_t * vLower, Vec_Ptr_t * vUpper )
{
    Aig_Obj_t * pObj;
    int i;
    // reset marks
    Aig_ManForEachObj( p, pObj, i )
    {
        pObj->fMarkA = 0;
        pObj->fMarkB = 1;
    }
    // clean PIs and const
    Aig_ManConst1(p)->fMarkB = 0;
    Aig_ManForEachCi( p, pObj, i )
        pObj->fMarkB = 0;
    // clean upper cut
//printf( "Upper: ");
    Vec_PtrForEachEntry( Aig_Obj_t *, vUpper, pObj, i )
    {
        Llb_ManFlowCleanMarkB_rec( pObj );
//printf( "%d ", pObj->Id );
    }
//printf( "\n" );
    // set lower cut
//printf( "Lower: ");
    Vec_PtrForEachEntry( Aig_Obj_t *, vLower, pObj, i )
    {
//printf( "%d ", pObj->Id );
        assert( pObj->fMarkB == 0 );
        Llb_ManFlowSetMarkA_rec( pObj );
    }
//printf( "\n" );
}

/**Function*************************************************************

  Synopsis    [Prepares flow computation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Llb_ManFlowUnmarkCone( Aig_Man_t * p, Vec_Ptr_t * vCone )
{
    Aig_Obj_t * pObj;
    int i;
    Vec_PtrForEachEntry( Aig_Obj_t *, vCone, pObj, i )
    {
        assert( Aig_ObjIsNode(pObj) );
        assert( pObj->fMarkB == 1 );
        pObj->fMarkB = 0;
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Llb_ManFlowCollectAndMarkCone_rec( Aig_Man_t * p, Aig_Obj_t * pObj, Vec_Ptr_t * vCone )
{
    Aig_Obj_t * pFanout;
    int i, iFanout = -1;
    if ( Saig_ObjIsLi(p, pObj) )
        return;
    if ( pObj->fMarkB )
        return;
    if ( pObj->fMarkA == 0 )
    {
        assert( Aig_ObjIsNode(pObj) );
        pObj->fMarkB = 1;
        if ( Aig_ObjIsNode(pObj) )
            Vec_PtrPush( vCone, pObj );
    }
    Aig_ObjForEachFanout( p, pObj, pFanout, iFanout, i )
        Llb_ManFlowCollectAndMarkCone_rec( p, pFanout, vCone );
}

/**Function*************************************************************

  Synopsis    [Collects the cone.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Llb_ManFlowCollectAndMarkCone( Aig_Man_t * p, Vec_Ptr_t * vStarts, Vec_Ptr_t * vCone )
{
    Aig_Obj_t * pObj;
    int i;
    Vec_PtrClear( vCone );
    Vec_PtrForEachEntry( Aig_Obj_t *, vStarts, pObj, i )
    {
        assert( pObj->fMarkA && !pObj->fMarkB );
        Llb_ManFlowCollectAndMarkCone_rec( p, pObj, vCone );
    }
}




/**Function*************************************************************

  Synopsis    [Finds balanced cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Llb_ManComputeCutLo( Aig_Man_t * p )
{
    Vec_Ptr_t * vMinCut;
    Aig_Obj_t * pObj;
    int i;
    vMinCut = Vec_PtrAlloc( 100 );
    Aig_ManForEachCi( p, pObj, i )
        Vec_PtrPush( vMinCut, pObj );
    return vMinCut;
}

/**Function*************************************************************

  Synopsis    [Finds balanced cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Llb_ManComputeCutLi( Aig_Man_t * p )
{
    Vec_Ptr_t * vMinCut;
    Aig_Obj_t * pObj;
    int i;
    assert( Saig_ManPoNum(p) == 0 );
    vMinCut = Vec_PtrAlloc( 100 );
    Aig_ManIncrementTravId(p);
    Saig_ManForEachLi( p, pObj, i )
    {
        pObj = Aig_ObjFanin0(pObj);
        if ( Aig_ObjIsConst1(pObj) )
            continue;
        if ( Aig_ObjIsTravIdCurrent(p, pObj) )
            continue;
        Aig_ObjSetTravIdCurrent(p, pObj);
        Vec_PtrPush( vMinCut, pObj );
    }
    return vMinCut;
}



/**Function*************************************************************

  Synopsis    [Finds balanced cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Llb_ManFlowGetObjSet( Aig_Man_t * p, Vec_Ptr_t * vLower, int iStart, int nSize, Vec_Ptr_t * vSet )
{
    Aig_Obj_t * pObj;
    int i;
    Vec_PtrClear( vSet );
    for ( i = 0; i < nSize; i++ )
    {
        pObj = (Aig_Obj_t *)Vec_PtrEntry( vLower, (iStart + i) % Vec_PtrSize(vLower) );
        Vec_PtrPush( vSet, pObj );
    }
}

/**Function*************************************************************

  Synopsis    [Finds balanced cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Llb_ManFlowFindBestCut( Aig_Man_t * p, Vec_Ptr_t * vLower, Vec_Ptr_t * vUpper, int Num )
{
    int nVolMin = Aig_ManNodeNum(p) / Num / 2;
    Vec_Ptr_t * vMinCut;
    Vec_Ptr_t * vCone, * vSet;
    Aig_Obj_t * pObj;
    int i, s, Vol, VolLower, VolUpper, VolCmp;
    int iBest = -1, iMinCut = ABC_INFINITY, iVolBest = 0;

    Vol = Llb_ManCutVolume( p, vLower, vUpper );
    assert( Vol > nVolMin );
    VolCmp = Abc_MinInt( nVolMin, Vol - nVolMin );
    vCone = Vec_PtrAlloc( 100 );
    vSet  = Vec_PtrAlloc( 100 );
    Llb_ManFlowPrepareCut( p, vLower, vUpper );
    for ( s = 1; s < Aig_ManRegNum(p); s += 5 )
    {
        Vec_PtrForEachEntry( Aig_Obj_t *, vLower, pObj, i )
        {
            Llb_ManFlowGetObjSet( p, vLower, i, s, vSet );
            Llb_ManFlowCollectAndMarkCone( p, vSet, vCone );
            if ( Vec_PtrSize(vCone) == 0 )
                continue;
            vMinCut  = Llb_ManFlowCompute( p );
            Llb_ManFlowUnmarkCone( p, vCone );

            VolLower = Llb_ManCutVolume( p, vLower, vMinCut );
            VolUpper = Llb_ManCutVolume( p, vMinCut, vUpper );
            Vol = Abc_MinInt( VolLower, VolUpper );
            if ( Vol >= VolCmp &&  (iMinCut == -1 || 
                                    iMinCut >  Vec_PtrSize(vMinCut) || 
                                   (iMinCut == Vec_PtrSize(vMinCut) && iVolBest < Vol)) )
            {
                iBest = i;
                iMinCut = Vec_PtrSize(vMinCut);
                iVolBest = Vol;
            }
            Vec_PtrFree( vMinCut );
        }
        if ( iBest >= 0 )
            break;
    }
    if ( iBest == -1 )
    {
        // cleanup
        Vec_PtrFree( vCone );
        Vec_PtrFree( vSet );
        return NULL;
    }
    // get the best cut
    assert( iBest >= 0 );
    Llb_ManFlowGetObjSet( p, vLower, iBest, s, vSet );
    Llb_ManFlowCollectAndMarkCone( p, vSet, vCone );
    vMinCut = Llb_ManFlowCompute( p );
    Llb_ManFlowUnmarkCone( p, vCone );
    // cleanup
    Vec_PtrFree( vCone );
    Vec_PtrFree( vSet );
    return vMinCut;
}

/**Function*************************************************************

  Synopsis    [Finds balanced cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Llb_ManComputeCuts( Aig_Man_t * p, int Num, int fVerbose, int fVeryVerbose )
{
    int nVolMax = Aig_ManNodeNum(p) / Num;
    Vec_Ptr_t * vResult, * vMinCut = NULL, * vLower, * vUpper;
    int i, k, nVol;
    abctime clk = Abc_Clock();
    vResult = Vec_PtrAlloc( 100 );
    Vec_PtrPush( vResult, Llb_ManComputeCutLo(p) );
    Vec_PtrPush( vResult, Llb_ManComputeCutLi(p) );
    while ( 1 ) 
    {
        // find a place to insert new cut
        vLower = (Vec_Ptr_t *)Vec_PtrEntry( vResult, 0 );
        Vec_PtrForEachEntryStart( Vec_Ptr_t *, vResult, vUpper, i, 1 )
        {
            nVol = Llb_ManCutVolume( p, vLower, vUpper );
            if ( nVol <= nVolMax )
            {
                vLower = vUpper;
                continue;
            }

            if ( fVeryVerbose )
            Llb_ManCutPrint( p, vLower,  vUpper );
            vMinCut = Llb_ManFlowFindBestCut( p, vLower, vUpper, Num );
            if ( vMinCut == NULL )
            {
                if ( fVeryVerbose )
                printf( "Could not break the cut.\n" );
                if ( fVeryVerbose )
                printf( "\n" );
                vLower = vUpper;
                continue;
            }

            if ( fVeryVerbose )
            Llb_ManCutPrint( p, vMinCut, vUpper );
            if ( fVeryVerbose )
            Llb_ManCutPrint( p, vLower,  vMinCut );
            if ( fVeryVerbose )
            printf( "\n" );

            break;
        }
        if ( i == Vec_PtrSize(vResult) )
            break;
        // insert vMinCut before vUpper
        Vec_PtrPush( vResult, NULL );
        for ( k = Vec_PtrSize(vResult) - 1; k > i; k-- )
            Vec_PtrWriteEntry( vResult, k, Vec_PtrEntry(vResult, k-1) );
        Vec_PtrWriteEntry( vResult, i, vMinCut );
    }
    if ( fVerbose )
    {
        printf( "Finished computing %d partitions.  ", Vec_PtrSize(vResult) - 1 );
        Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
        Llb_ManResultPrint( p, vResult );
    }
    return vResult;
}
 
/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Llb_BddSetDefaultParams( Gia_ParLlb_t * p )
{
    memset( p, 0, sizeof(Gia_ParLlb_t) );
    p->nBddMax       =  1000000;
    p->nIterMax      = 10000000;
    p->nClusterMax   =       20;
    p->nHintDepth    =        0;
    p->HintFirst     =        0;
    p->fUseFlow      =        0;  // use flow 
    p->nVolumeMax    =      100;  // max volume
    p->nVolumeMin    =       30;  // min volume
    p->fReorder      =        1;
    p->fIndConstr    =        0;
    p->fUsePivots    =        0;
    p->fCluster      =        0;
    p->fSchedule     =        0;
    p->fVerbose      =        0;
    p->fVeryVerbose  =        0;
    p->fSilent       =        0;
    p->TimeLimit     =        0;
//    p->TimeLimit     =        0;
    p->TimeLimitGlo  =        0;
    p->TimeTarget    =        0;
    p->iFrame        =       -1;
}

/**Function*************************************************************

  Synopsis    [Finds balanced cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Llb_ManMinCutTest( Aig_Man_t * pAig, int Num )
{
    extern void Llb_BddConstructTest( Aig_Man_t * p, Vec_Ptr_t * vResult );
    extern void Llb_BddExperiment( Aig_Man_t * pInit, Aig_Man_t * pAig, Gia_ParLlb_t * pPars, Vec_Ptr_t * vResult, Vec_Ptr_t * vMaps );
 

//    int fVerbose = 1;
    Gia_ParLlb_t Pars, * pPars = &Pars;
    Vec_Ptr_t * vResult;//, * vSupps, * vMaps;
    Aig_Man_t * p;

    Llb_BddSetDefaultParams( pPars );

    p = Aig_ManDupFlopsOnly( pAig );
//Aig_ManShow( p, 0, NULL );
    Aig_ManPrintStats( pAig );
    Aig_ManPrintStats( p );
    Aig_ManFanoutStart( p );

    vResult = Llb_ManComputeCuts( p, Num, 1, 0 );
//    vSupps  = Llb_ManCutSupps( p, vResult );
//    vMaps   = Llb_ManCutMap( p, vResult, vSupps );

//    Llb_BddExperiment( pAig, p, pPars, vResult, vMaps );
    Llb_CoreExperiment( pAig, p, pPars, vResult, 0 );

//    Vec_VecFree( (Vec_Vec_t *)vMaps );
//    Vec_VecFree( (Vec_Vec_t *)vSupps );
    Vec_VecFree( (Vec_Vec_t *)vResult );

    Aig_ManFanoutStop( p );
    Aig_ManCleanMarkAB( p );
    Aig_ManStop( p );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

