/**CFile****************************************************************

  FileName    [giaPack.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Scalable AIG package.]

  Synopsis    [LUT packing.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: giaPack.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

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

  Synopsis    [Collects LUT nodes in the increasing order of distance from COs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Gia_ManLutCollect2( Gia_Man_t * p )
{
    Gia_Obj_t * pObj;
    Vec_Int_t * vOrder;
    int i, k, Id, iFan;
    vOrder = Vec_IntAlloc( Gia_ManLutNum(p) );
    Gia_ManIncrementTravId( p );
    Gia_ManForEachCoDriver( p, pObj, i )
    {
        if ( !Gia_ObjIsAnd(pObj) )
            continue;
        Id = Gia_ObjId( p, pObj );
        assert( Gia_ObjIsLut(p, Id) );
        if ( Gia_ObjIsTravIdCurrentId(p, Id) )
            continue;
        Gia_ObjSetTravIdCurrentId(p, Id);
        Vec_IntPush( vOrder, Id );
    }
    Vec_IntForEachEntry( vOrder, Id, i )
    {
        Gia_LutForEachFanin( p, Id, iFan, k )
        {
            if ( !Gia_ObjIsAnd(Gia_ManObj(p, iFan)) )
                continue;
            assert( Gia_ObjIsLut(p, iFan) );
            if ( Gia_ObjIsTravIdCurrentId(p, iFan) )
                continue;
            Gia_ObjSetTravIdCurrentId(p, iFan);
            Vec_IntPush( vOrder, iFan );
        }
    }
    assert( Vec_IntCap(vOrder) == 16 || Vec_IntSize(vOrder) == Vec_IntCap(vOrder) );
    Vec_IntReverseOrder( vOrder );
    return vOrder;
}
Vec_Int_t * Gia_ManLutCollect( Gia_Man_t * p )
{
    Vec_Int_t * vLuts, * vDist, * vOrder;
    int i, k, Id, iFan, * pPerm;
    // collect LUTs
    vLuts = Vec_IntAlloc( Gia_ManAndNum(p) );
    Gia_ManForEachLut( p, Id )
        Vec_IntPush( vLuts, Id );
    // propagate distance
    vDist = Vec_IntStart( Gia_ManObjNum(p) );
    Gia_ManForEachCoDriverId( p, Id, i )
        Vec_IntWriteEntry( vDist, Id, 1 );
    Vec_IntForEachEntryReverse( vLuts, Id, i )
    {
        int Dist = 1 + Vec_IntEntry(vDist, Id);
        Gia_LutForEachFanin( p, Id, iFan, k )
            Vec_IntUpdateEntry( vDist, iFan, Dist );
    }
    // sort LUTs by distance
    k = 0;
    Vec_IntForEachEntry( vLuts, Id, i )
        Vec_IntWriteEntry( vDist, k++, -Vec_IntEntry(vDist, Id) );
    Vec_IntShrink( vDist, k );
    pPerm = Abc_MergeSortCost( Vec_IntArray(vDist), Vec_IntSize(vDist) );
    // collect
    vOrder = Vec_IntAlloc( Vec_IntSize(vLuts) );
    for ( i = 0; i < Vec_IntSize(vLuts); i++ )
        Vec_IntPush( vOrder, Vec_IntEntry(vLuts, pPerm[i]) );
    Vec_IntFree( vDist );
    Vec_IntFree( vLuts );
    ABC_FREE( pPerm );
    return vOrder;
}

/**Function*************************************************************

  Synopsis    [LUT packing algorithm.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManLutPacking( Gia_Man_t * p, int nBlockSize, int DelayRoute, int DelayDir, int fVerbose )
{
    int Delays[32], Perm[32];
    Vec_Int_t * vPacking, * vStarts;
    Vec_Int_t * vOrder = Gia_ManLutCollect( p );
    Vec_Int_t * vDelay = Vec_IntStart( Gia_ManObjNum(p) );
    Vec_Int_t * vBlock = Vec_IntStart( Gia_ManObjNum(p) );
    Vec_Int_t * vBSize = Vec_IntAlloc( 2 * Vec_IntSize(vOrder) / nBlockSize );
    int i, k, Id, iFan, nSize, iBlock, Delay, DelayMax = 0;
    // create blocks
    Vec_IntForEachEntry( vOrder, Id, i )
    {
        nSize = Gia_ObjLutSize( p, Id );
        assert( nSize <= 32 );
        Gia_LutForEachFanin( p, Id, iFan, k )
        {
            Delays[k] = Vec_IntEntry(vDelay, iFan);
            Perm[k] = iFan;
        }
        Vec_IntSelectSortCost2Reverse( Perm, nSize, Delays );
        assert( nSize < 2 || Delays[0] >= Delays[nSize-1] );
        assert( Delays[0] >= 0 && Delays[nSize-1] >= 0 );
        // check if we can reduce delay by adding it to the same bin as the latest one
        iBlock = Vec_IntEntry( vBlock, Perm[0] );
        if ( Delays[0] > 0 && Delays[0] > Delays[1] && Vec_IntEntry(vBSize, iBlock) < nBlockSize )
        {
            Delay = Delays[0] + DelayDir;
            Vec_IntWriteEntry( vBlock, Id, iBlock );
            Vec_IntAddToEntry( vBSize, iBlock, 1 );
        }
        else // clean new block
        {
            Delay = Delays[0] + DelayRoute;
            Vec_IntWriteEntry( vBlock, Id, Vec_IntSize(vBSize) );
            Vec_IntPush( vBSize, 1 );
        }
        // calculate delay
        for ( k = 1; k < nSize; k++ )
            Delay = Abc_MaxInt( Delay, Delays[k] + DelayRoute );
        Vec_IntWriteEntry( vDelay, Id, Delay );
        DelayMax = Abc_MaxInt( DelayMax, Delay );
    }
    assert( Vec_IntSum(vBSize) == Vec_IntSize(vOrder) );
    // create packing info
    vPacking = Vec_IntAlloc( Vec_IntSize(vBSize) + Vec_IntSize(vOrder) + 1 );
    Vec_IntPush( vPacking, Vec_IntSize(vBSize) );
    // create starting places for each block
    vStarts = Vec_IntAlloc( Vec_IntSize(vBSize) );
    Vec_IntForEachEntry( vBSize, nSize, i )
    {
        Vec_IntPush( vPacking, nSize );
        Vec_IntPush( vStarts, Vec_IntSize(vPacking) );
        Vec_IntFillExtra( vPacking, Vec_IntSize(vPacking) + nSize, -1 );
    }
    assert( Vec_IntCap(vPacking) == 16 || Vec_IntSize(vPacking) == Vec_IntCap(vPacking) );
    // collect LUTs from the block
    Vec_IntForEachEntryReverse( vOrder, Id, i )
    {
        int Block = Vec_IntEntry( vBlock, Id );
        int Start = Vec_IntEntry( vStarts, Block );
        assert( Vec_IntEntry(vPacking, Start) == -1 );
        Vec_IntWriteEntry( vPacking, Start, Id );
        Vec_IntAddToEntry( vStarts, Block, 1 );
    }
    assert( Vec_IntCountEntry(vPacking, -1) == 0 );
    // cleanup
    Vec_IntFree( vOrder );
    Vec_IntFree( vDelay );
    Vec_IntFree( vBlock );
    Vec_IntFree( vBSize );
    Vec_IntFree( vStarts );
    Vec_IntFreeP( &p->vPacking );
    p->vPacking = vPacking;
    printf( "Global delay = %d.\n", DelayMax );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

