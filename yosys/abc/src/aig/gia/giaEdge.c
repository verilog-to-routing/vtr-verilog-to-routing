/**CFile****************************************************************

  FileName    [giaEdge.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Scalable AIG package.]

  Synopsis    [Edge-related procedures.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: giaEdge.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "gia.h"
#include "misc/tim/tim.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static inline int Gia_ObjEdgeCount( int iObj, Vec_Int_t * vEdge1, Vec_Int_t * vEdge2 )
{
    return (Vec_IntEntry(vEdge1, iObj) > 0) + (Vec_IntEntry(vEdge2, iObj) > 0);
}
static inline int Gia_ObjEdgeAdd( int iObj, int iNext, Vec_Int_t * vEdge1, Vec_Int_t * vEdge2 )
{
    int RetValue = 0;
    if ( Vec_IntEntry(vEdge1, iObj) == 0 )
        Vec_IntWriteEntry(vEdge1, iObj, iNext);
    else if ( Vec_IntEntry(vEdge2, iObj) == 0 )
        Vec_IntWriteEntry(vEdge2, iObj, iNext);
    else RetValue = 1;
    return RetValue;
}
static inline void Gia_ObjEdgeRemove( int iObj, int iNext, Vec_Int_t * vEdge1, Vec_Int_t * vEdge2 )
{
    assert( Vec_IntEntry(vEdge1, iObj) == iNext || Vec_IntEntry(vEdge2, iObj) == iNext );
    if ( Vec_IntEntry(vEdge1, iObj) == iNext )
        Vec_IntWriteEntry( vEdge1, iObj, Vec_IntEntry(vEdge2, iObj) );
    Vec_IntWriteEntry( vEdge2, iObj, 0 );
}
static inline void Gia_ObjEdgeClean( int iObj, Vec_Int_t * vEdge1, Vec_Int_t * vEdge2 )
{
    Vec_IntWriteEntry( vEdge1, iObj, 0 );
    Vec_IntWriteEntry( vEdge2, iObj, 0 );
}

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Transforms edge assignment.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManEdgeFromArray( Gia_Man_t * p, Vec_Int_t * vArray )
{
    int i, iObj1, iObj2, Count = 0;
    Vec_IntFreeP( &p->vEdge1 );
    Vec_IntFreeP( &p->vEdge2 );
    p->vEdge1 = Vec_IntStart( Gia_ManObjNum(p) );
    p->vEdge2 = Vec_IntStart( Gia_ManObjNum(p) );
    Vec_IntForEachEntryDouble( vArray, iObj1, iObj2, i )
    {
        assert( iObj1 < iObj2 );
        Count += Gia_ObjEdgeAdd( iObj1, iObj2, p->vEdge1, p->vEdge2 );
        Count += Gia_ObjEdgeAdd( iObj2, iObj1, p->vEdge1, p->vEdge2 );
    }
    if ( Count ) 
        printf( "Found %d violations during edge conversion.\n", Count );
}
Vec_Int_t * Gia_ManEdgeToArray( Gia_Man_t * p )
{
    int iObj, iFanin;
    Vec_Int_t * vArray = Vec_IntAlloc( 1000 );
    assert( p->vEdge1 && p->vEdge2 );
    assert( Vec_IntSize(p->vEdge1) == Gia_ManObjNum(p) );
    assert( Vec_IntSize(p->vEdge2) == Gia_ManObjNum(p) );
    for ( iObj = 0; iObj < Gia_ManObjNum(p); iObj++ )
    {
        iFanin = Vec_IntEntry( p->vEdge1, iObj );
        if ( iFanin && iFanin < iObj )
            Vec_IntPushTwo( vArray, iFanin, iObj );
        iFanin = Vec_IntEntry( p->vEdge2, iObj );
        if ( iFanin && iFanin < iObj )
            Vec_IntPushTwo( vArray, iFanin, iObj );
    }
    return vArray;
}

/**Function*************************************************************

  Synopsis    [Prints mapping statistics.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManConvertPackingToEdges( Gia_Man_t * p )
{
    int i, k, Entry, nEntries, nEntries2, nNodes[4], Count = 0;
    if ( p->vPacking == NULL )
        return;
    Vec_IntFreeP( &p->vEdge1 );
    Vec_IntFreeP( &p->vEdge2 );
    p->vEdge1 = Vec_IntStart( Gia_ManObjNum(p) );
    p->vEdge2 = Vec_IntStart( Gia_ManObjNum(p) );
    // iterate through structures
    nEntries = Vec_IntEntry( p->vPacking, 0 );
    nEntries2 = 0;
    Vec_IntForEachEntryStart( p->vPacking, Entry, i, 1 )
    {
        assert( Entry > 0 && Entry < 4 );
        i++;
        for ( k = 0; k < Entry; k++, i++ )
            nNodes[k] = Vec_IntEntry(p->vPacking, i);
        i--;
        nEntries2++;
        // create edges
        if ( Entry == 2 )
        {
            Count += Gia_ObjEdgeAdd( nNodes[0], nNodes[1], p->vEdge1, p->vEdge2 );
            Count += Gia_ObjEdgeAdd( nNodes[1], nNodes[0], p->vEdge1, p->vEdge2 );
        }
        else if ( Entry == 3 )
        {
            Count += Gia_ObjEdgeAdd( nNodes[0], nNodes[2], p->vEdge1, p->vEdge2 );
            Count += Gia_ObjEdgeAdd( nNodes[2], nNodes[0], p->vEdge1, p->vEdge2 );
            Count += Gia_ObjEdgeAdd( nNodes[1], nNodes[2], p->vEdge1, p->vEdge2 );
            Count += Gia_ObjEdgeAdd( nNodes[2], nNodes[1], p->vEdge1, p->vEdge2 );
        }
    }
    assert( nEntries == nEntries2 );
    if ( Count )
        printf( "Skipped %d illegal edges.\n", Count );
}

/**Function*************************************************************

  Synopsis    [Evaluates given edge assignment.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Gia_ObjHaveEdge( Gia_Man_t * p, int iObj, int iNext )
{
    return Vec_IntEntry(p->vEdge1, iObj) == iNext || Vec_IntEntry(p->vEdge2, iObj) == iNext;
}
int Gia_ObjCheckEdge( Gia_Man_t * p, int iObj, int iNext )
{
    return Gia_ObjHaveEdge( p, iObj, iNext );
}
static inline int Gia_ObjEvalEdgeDelay( Gia_Man_t * p, int iObj, Vec_Int_t * vDelay )
{
    int nEdgeDelay = 2;
    int i, iFan, Delay, DelayMax = 0;
    if ( Gia_ManHasMapping(p) && Gia_ObjIsLut(p, iObj) )
    {
        assert( Gia_ObjLutSize(p, iObj) <= 4 );
        Gia_LutForEachFanin( p, iObj, iFan, i )
        {
            Delay = Vec_IntEntry(vDelay, iFan) + (Gia_ObjHaveEdge(p, iObj, iFan) ? nEdgeDelay : 10);
            DelayMax = Abc_MaxInt( DelayMax, Delay );
        }
    }
    else if ( Gia_ObjIsLut2(p, iObj) )
    {
        assert( Gia_ObjLutSize2(p, iObj) <= 4 );
        Gia_LutForEachFanin2( p, iObj, iFan, i )
        {
            Delay = Vec_IntEntry(vDelay, iFan) + (Gia_ObjHaveEdge(p, iObj, iFan) ? nEdgeDelay : 10);
            DelayMax = Abc_MaxInt( DelayMax, Delay );
        }
    }
    else assert( 0 );
    return DelayMax;
}
int Gia_ManEvalEdgeDelay( Gia_Man_t * p )
{
    int k, iLut, DelayMax = 0;
    assert( p->vEdge1 && p->vEdge2 );
    Vec_IntFreeP( &p->vEdgeDelay );
    p->vEdgeDelay = Vec_IntStart( Gia_ManObjNum(p) );
    if ( Gia_ManHasMapping(p) )
    {
        if ( p->pManTime != NULL && Tim_ManBoxNum((Tim_Man_t*)p->pManTime) )
        {
            Gia_Obj_t * pObj; 
            Vec_Int_t * vNodes = Gia_ManOrderWithBoxes( p );
            Tim_ManIncrementTravId( (Tim_Man_t*)p->pManTime );
            Gia_ManForEachObjVec( vNodes, p, pObj, k )
            {
                iLut = Gia_ObjId( p, pObj );
                if ( Gia_ObjIsAnd(pObj) )
                {
                    if ( Gia_ObjIsLut(p, iLut) )
                        Vec_IntWriteEntry( p->vEdgeDelay, iLut, Gia_ObjEvalEdgeDelay(p, iLut, p->vEdgeDelay) );
                }
                else if ( Gia_ObjIsCi(pObj) )
                {
                    int arrTime = Tim_ManGetCiArrival( (Tim_Man_t*)p->pManTime, Gia_ObjCioId(pObj) );
                    Vec_IntWriteEntry( p->vEdgeDelay,  iLut, arrTime );
                }
                else if ( Gia_ObjIsCo(pObj) )
                {
                    int arrTime = Vec_IntEntry( p->vEdgeDelay, Gia_ObjFaninId0(pObj, iLut) );
                    Tim_ManSetCoArrival( (Tim_Man_t*)p->pManTime, Gia_ObjCioId(pObj), arrTime );
                }
                else if ( !Gia_ObjIsConst0(pObj) ) 
                    assert( 0 );
            }
            Vec_IntFree( vNodes );
        }
        else
        {
            Gia_ManForEachLut( p, iLut )
                Vec_IntWriteEntry( p->vEdgeDelay, iLut, Gia_ObjEvalEdgeDelay(p, iLut, p->vEdgeDelay) );
        }
    }
    else if ( Gia_ManHasMapping2(p) )
    {
        if ( p->pManTime != NULL && Tim_ManBoxNum((Tim_Man_t*)p->pManTime) )
        {
            Gia_Obj_t * pObj; 
            Vec_Int_t * vNodes = Gia_ManOrderWithBoxes( p );
            Tim_ManIncrementTravId( (Tim_Man_t*)p->pManTime );
            Gia_ManForEachObjVec( vNodes, p, pObj, k )
            {
                iLut = Gia_ObjId( p, pObj );
                if ( Gia_ObjIsAnd(pObj) )
                {
                    if ( Gia_ObjIsLut2(p, iLut) )
                        Vec_IntWriteEntry( p->vEdgeDelay, iLut, Gia_ObjEvalEdgeDelay(p, iLut, p->vEdgeDelay) );
                }
                else if ( Gia_ObjIsCi(pObj) )
                {
                    int arrTime = Tim_ManGetCiArrival( (Tim_Man_t*)p->pManTime, Gia_ObjCioId(pObj) );
                    Vec_IntWriteEntry( p->vEdgeDelay,  iLut, arrTime );
                }
                else if ( Gia_ObjIsCo(pObj) )
                {
                    int arrTime = Vec_IntEntry( p->vEdgeDelay, Gia_ObjFaninId0(pObj, iLut) );
                    Tim_ManSetCoArrival( (Tim_Man_t*)p->pManTime, Gia_ObjCioId(pObj), arrTime );
                }
                else if ( !Gia_ObjIsConst0(pObj) ) 
                    assert( 0 );
            }
            Vec_IntFree( vNodes );
        }
        else
        {
            Gia_ManForEachLut2( p, iLut )
                Vec_IntWriteEntry( p->vEdgeDelay, iLut, Gia_ObjEvalEdgeDelay(p, iLut, p->vEdgeDelay) );
        }
    }
    else assert( 0 );
    Gia_ManForEachCoDriverId( p, iLut, k )
        DelayMax = Abc_MaxInt( DelayMax, Vec_IntEntry(p->vEdgeDelay, iLut) );
    return DelayMax;
}
int Gia_ManEvalEdgeCount( Gia_Man_t * p )
{
    return (Vec_IntCountPositive(p->vEdge1) + Vec_IntCountPositive(p->vEdge2))/2;
}


/**Function*************************************************************

  Synopsis    [Finds edge assignment.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ObjComputeEdgeDelay( Gia_Man_t * p, int iObj, Vec_Int_t * vDelay, Vec_Int_t * vEdge1, Vec_Int_t * vEdge2, int fUseTwo )
{
    int i, iFan, Delay, Status1, Status2;
    int DelayMax = 0, DelayMax2 = 0, nCountMax = 0;
    int iFanMax1 = -1, iFanMax2 = -1;
    Vec_IntWriteEntry(vEdge1, iObj, 0);
    Vec_IntWriteEntry(vEdge2, iObj, 0);
    if ( Gia_ManHasMapping(p) && Gia_ObjIsLut(p, iObj) )
    {
        assert( Gia_ObjLutSize(p, iObj) <= 4 );
        Gia_LutForEachFanin( p, iObj, iFan, i )
        {
            Delay = Vec_IntEntry( vDelay, iFan ) + 10;
            if ( DelayMax < Delay )
            {
                DelayMax2 = DelayMax;
                DelayMax  = Delay;
                iFanMax1  = iFan;
                nCountMax = 1;
            }
            else if ( DelayMax == Delay )
            {
                iFanMax2  = iFan;
                nCountMax++;
                if ( !fUseTwo )
                    DelayMax2 = DelayMax;
            }
            else
                DelayMax2 = Abc_MaxInt( DelayMax2, Delay );
        }
    }
    else if ( Gia_ObjIsLut2(p, iObj) )
    {
        assert( Gia_ObjLutSize2(p, iObj) <= 4 );
        Gia_LutForEachFanin2( p, iObj, iFan, i )
        {
            Delay = Vec_IntEntry( vDelay, iFan ) + 10;
            if ( DelayMax < Delay )
            {
                DelayMax2 = DelayMax;
                DelayMax  = Delay;
                iFanMax1  = iFan;
                nCountMax = 1;
            }
            else if ( DelayMax == Delay )
            {
                iFanMax2  = iFan;
                nCountMax++;
                if ( !fUseTwo )
                    DelayMax2 = DelayMax;
            }
            else
                DelayMax2 = Abc_MaxInt( DelayMax2, Delay );
        }
    }
    else assert( 0 );
    assert( nCountMax > 0 );
    if ( DelayMax <= 10 )
    {} // skip first level
    else if ( nCountMax == 1 )
    {
        Status1 = Gia_ObjEdgeCount( iFanMax1, vEdge1, vEdge2 );
        if ( Status1 <= 1 )
        {
            Gia_ObjEdgeAdd( iFanMax1, iObj, vEdge1, vEdge2 );
            Gia_ObjEdgeAdd( iObj, iFanMax1, vEdge1, vEdge2 );
            DelayMax = Abc_MaxInt( DelayMax2, DelayMax - 8 );
            Vec_IntWriteEntry( vDelay, iObj, DelayMax );
            return DelayMax;
        }
    }
    else if ( fUseTwo && nCountMax == 2 )
    {
        Status1 = Gia_ObjEdgeCount( iFanMax1, vEdge1, vEdge2 );
        Status2 = Gia_ObjEdgeCount( iFanMax2, vEdge1, vEdge2 );
        if ( Status1 <= 1 && Status2 <= 1 )
        {
            Gia_ObjEdgeAdd( iFanMax1, iObj, vEdge1, vEdge2 );
            Gia_ObjEdgeAdd( iFanMax2, iObj, vEdge1, vEdge2 );
            Gia_ObjEdgeAdd( iObj, iFanMax1, vEdge1, vEdge2 );
            Gia_ObjEdgeAdd( iObj, iFanMax2, vEdge1, vEdge2 );
            DelayMax = Abc_MaxInt( DelayMax2, DelayMax - 8 );
            Vec_IntWriteEntry( vDelay, iObj, DelayMax );
            return DelayMax;
        }
    }
    Vec_IntWriteEntry( vDelay, iObj, DelayMax );
    return DelayMax;
}
int Gia_ManComputeEdgeDelay( Gia_Man_t * p, int fUseTwo )
{
    int k, iLut, DelayMax = 0;
    Vec_IntFreeP( &p->vEdgeDelay );
    Vec_IntFreeP( &p->vEdge1 );
    Vec_IntFreeP( &p->vEdge2 );
    p->vEdge1 = Vec_IntStart( Gia_ManObjNum(p) );
    p->vEdge2 = Vec_IntStart( Gia_ManObjNum(p) );
    p->vEdgeDelay = Vec_IntStart( Gia_ManObjNum(p) );
    if ( Gia_ManHasMapping(p) )
    {
        if ( p->pManTime != NULL && Tim_ManBoxNum((Tim_Man_t*)p->pManTime) )
        {
            Gia_Obj_t * pObj; 
            Vec_Int_t * vNodes = Gia_ManOrderWithBoxes( p );
            Tim_ManIncrementTravId( (Tim_Man_t*)p->pManTime );
            Gia_ManForEachObjVec( vNodes, p, pObj, k )
            {
                iLut = Gia_ObjId( p, pObj );
                if ( Gia_ObjIsAnd(pObj) )
                {
                    if ( Gia_ObjIsLut(p, iLut) )
                        Gia_ObjComputeEdgeDelay( p, iLut, p->vEdgeDelay, p->vEdge1, p->vEdge2, fUseTwo );
                }
                else if ( Gia_ObjIsCi(pObj) )
                {
                    int arrTime = Tim_ManGetCiArrival( (Tim_Man_t*)p->pManTime, Gia_ObjCioId(pObj) );
                    Vec_IntWriteEntry( p->vEdgeDelay,  iLut, arrTime );
                }
                else if ( Gia_ObjIsCo(pObj) )
                {
                    int arrTime = Vec_IntEntry( p->vEdgeDelay, Gia_ObjFaninId0(pObj, iLut) );
                    Tim_ManSetCoArrival( (Tim_Man_t*)p->pManTime, Gia_ObjCioId(pObj), arrTime );
                }
                else if ( !Gia_ObjIsConst0(pObj) ) 
                    assert( 0 );
            }
            Vec_IntFree( vNodes );
        }
        else
        {
            Gia_ManForEachLut( p, iLut )
                Gia_ObjComputeEdgeDelay( p, iLut, p->vEdgeDelay, p->vEdge1, p->vEdge2, fUseTwo );
        }
    }
    else if ( Gia_ManHasMapping2(p) )
    {
        if ( p->pManTime != NULL && Tim_ManBoxNum((Tim_Man_t*)p->pManTime) )
        {
            Gia_Obj_t * pObj; 
            Vec_Int_t * vNodes = Gia_ManOrderWithBoxes( p );
            Tim_ManIncrementTravId( (Tim_Man_t*)p->pManTime );
            Gia_ManForEachObjVec( vNodes, p, pObj, k )
            {
                iLut = Gia_ObjId( p, pObj );
                if ( Gia_ObjIsAnd(pObj) )
                {
                    if ( Gia_ObjIsLut2(p, iLut) )
                        Gia_ObjComputeEdgeDelay( p, iLut, p->vEdgeDelay, p->vEdge1, p->vEdge2, fUseTwo );
                }
                else if ( Gia_ObjIsCi(pObj) )
                {
                    int arrTime = Tim_ManGetCiArrival( (Tim_Man_t*)p->pManTime, Gia_ObjCioId(pObj) );
                    Vec_IntWriteEntry( p->vEdgeDelay,  iLut, arrTime );
                }
                else if ( Gia_ObjIsCo(pObj) )
                {
                    int arrTime = Vec_IntEntry( p->vEdgeDelay, Gia_ObjFaninId0(pObj, iLut) );
                    Tim_ManSetCoArrival( (Tim_Man_t*)p->pManTime, Gia_ObjCioId(pObj), arrTime );
                }
                else if ( !Gia_ObjIsConst0(pObj) ) 
                    assert( 0 );
            }
            Vec_IntFree( vNodes );
        }
        else
        {
            Gia_ManForEachLut2( p, iLut )
                Gia_ObjComputeEdgeDelay( p, iLut, p->vEdgeDelay, p->vEdge1, p->vEdge2, fUseTwo );
        }
    }
    else assert( 0 );
    Gia_ManForEachCoDriverId( p, iLut, k )
        DelayMax = Abc_MaxInt( DelayMax, Vec_IntEntry(p->vEdgeDelay, iLut) );
    //printf( "The number of edges = %d.  Delay = %d.\n", Gia_ManEvalEdgeCount(p), DelayMax );
    return DelayMax;
}

/**Function*************************************************************

  Synopsis    [Finds edge assignment.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ObjComputeEdgeDelay2( Gia_Man_t * p, int iObj, Vec_Int_t * vDelay, Vec_Int_t * vEdge1, Vec_Int_t * vEdge2, Vec_Int_t * vFanMax1, Vec_Int_t * vFanMax2, Vec_Int_t * vCountMax )
{
    int i, iFan, DelayFanin, Status1, Status2;
    int DelayMax = 0, nCountMax = 0;
    int iFanMax1 = -1, iFanMax2 = -1;
    Vec_IntWriteEntry(vEdge1, iObj, 0);
    Vec_IntWriteEntry(vEdge2, iObj, 0);
    // analyze this node
    DelayMax = Vec_IntEntry( vDelay, iObj );
    nCountMax = Vec_IntEntry( vCountMax, iObj );
    if ( DelayMax == 0 )
    {}
    else if ( nCountMax == 1 )
    {
        iFanMax1 = Vec_IntEntry( vFanMax1, iObj );
        Status1 = Gia_ObjEdgeCount( iFanMax1, vEdge1, vEdge2 );
        if ( Status1 <= 1 )
        {
            Gia_ObjEdgeAdd( iFanMax1, iObj, vEdge1, vEdge2 );
            Gia_ObjEdgeAdd( iObj, iFanMax1, vEdge1, vEdge2 );
            DelayMax--;
        }
    }
    else if ( nCountMax == 2 )
    {
        iFanMax1 = Vec_IntEntry( vFanMax1, iObj );
        iFanMax2 = Vec_IntEntry( vFanMax2, iObj );
        Status1 = Gia_ObjEdgeCount( iFanMax1, vEdge1, vEdge2 );
        Status2 = Gia_ObjEdgeCount( iFanMax2, vEdge1, vEdge2 );
        if ( Status1 <= 1 && Status2 <= 1 )
        {
            Gia_ObjEdgeAdd( iFanMax1, iObj, vEdge1, vEdge2 );
            Gia_ObjEdgeAdd( iFanMax2, iObj, vEdge1, vEdge2 );
            Gia_ObjEdgeAdd( iObj, iFanMax1, vEdge1, vEdge2 );
            Gia_ObjEdgeAdd( iObj, iFanMax2, vEdge1, vEdge2 );
            DelayMax--;
        }
    }
    Vec_IntWriteEntry( vDelay, iObj, DelayMax );
    // computed DelayMax at this point
    if ( Gia_ManHasMapping(p) && Gia_ObjIsLut(p, iObj) )
    {
        Gia_LutForEachFanin( p, iObj, iFan, i )
        {
            DelayFanin = Vec_IntEntry( vDelay, iFan );
            if ( DelayFanin < DelayMax + 1 )
            {
                Vec_IntWriteEntry( vDelay, iFan, DelayMax + 1 );
                Vec_IntWriteEntry( vFanMax1, iFan, iObj );
                Vec_IntWriteEntry( vCountMax, iFan, 1 );
            }
            else if ( DelayFanin == DelayMax + 1 )
            {
                Vec_IntWriteEntry( vFanMax2, iFan, iObj );
                Vec_IntAddToEntry( vCountMax, iFan, 1 );
            }
        }
    }
    else if ( Gia_ObjIsLut2(p, iObj) )
    {
        Gia_LutForEachFanin2( p, iObj, iFan, i )
        {
            DelayFanin = Vec_IntEntry( vDelay, iFan );
            if ( DelayFanin < DelayMax + 1 )
            {
                Vec_IntWriteEntry( vDelay, iFan, DelayMax + 1 );
                Vec_IntWriteEntry( vFanMax1, iFan, iObj );
                Vec_IntWriteEntry( vCountMax, iFan, 1 );
            }
            else if ( DelayFanin == DelayMax + 1 )
            {
                Vec_IntWriteEntry( vFanMax2, iFan, iObj );
                Vec_IntAddToEntry( vCountMax, iFan, 1 );
            }
        }
    }
    else assert( 0 );
    return DelayMax;
}
int Gia_ManComputeEdgeDelay2( Gia_Man_t * p )
{
    int k, iLut, DelayMax = 0;
    Vec_Int_t * vFanMax1 = Vec_IntStart( Gia_ManObjNum(p) );
    Vec_Int_t * vFanMax2 = Vec_IntStart( Gia_ManObjNum(p) );
    Vec_Int_t * vCountMax = Vec_IntStart( Gia_ManObjNum(p) );
    assert( p->pManTime == NULL );
    Vec_IntFreeP( &p->vEdgeDelay );
    Vec_IntFreeP( &p->vEdge1 );
    Vec_IntFreeP( &p->vEdge2 );
    p->vEdgeDelay = Vec_IntStart( Gia_ManObjNum(p) );
    p->vEdge1 = Vec_IntStart( Gia_ManObjNum(p) );
    p->vEdge2 = Vec_IntStart( Gia_ManObjNum(p) );
//    Gia_ManForEachCoDriverId( p, iLut, k )
//        Vec_IntWriteEntry( p->vEdgeDelay, iLut, 1 );
    if ( Gia_ManHasMapping(p) )
        Gia_ManForEachLutReverse( p, iLut )
            Gia_ObjComputeEdgeDelay2( p, iLut, p->vEdgeDelay, p->vEdge1, p->vEdge2, vFanMax1, vFanMax2, vCountMax );
    else if ( Gia_ManHasMapping2(p) )
        Gia_ManForEachLut2Reverse( p, iLut )
            Gia_ObjComputeEdgeDelay2( p, iLut, p->vEdgeDelay, p->vEdge1, p->vEdge2, vFanMax1, vFanMax2, vCountMax );
    else assert( 0 );
    Gia_ManForEachCiId( p, iLut, k )
        DelayMax = Abc_MaxInt( DelayMax, Vec_IntEntry(p->vEdgeDelay, iLut) );
    Vec_IntFree( vFanMax1 );
    Vec_IntFree( vFanMax2 );
    Vec_IntFree( vCountMax );
    //printf( "The number of edges = %d.  Delay = %d.\n", Gia_ManEvalEdgeCount(p), DelayMax );
    return DelayMax;
}

/**Function*************************************************************

  Synopsis    [Finds edge assignment.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManUpdateMapping( Gia_Man_t * p, Vec_Int_t * vNodes, Vec_Wec_t * vWin )
{
    int i, iNode;
    Vec_IntForEachEntry( vNodes, iNode, i )
        ABC_SWAP( Vec_Int_t, *Vec_WecEntry(p->vMapping2, iNode), *Vec_WecEntry(vWin, i) );
}
int Gia_ManEvalWindowInc( Gia_Man_t * p, Vec_Int_t * vLeaves, Vec_Int_t * vNodes, Vec_Wec_t * vWin, Vec_Int_t * vTemp, int fUseTwo )
{
    int i, iLut, Delay, DelayMax = 0;
    assert( Vec_IntSize(vNodes) == Vec_WecSize(vWin) );
    Gia_ManUpdateMapping( p, vNodes, vWin );
    Gia_ManCollectTfo( p, vLeaves, vTemp );
    Vec_IntReverseOrder( vTemp );
    Vec_IntForEachEntry( vTemp, iLut, i )
    {
        if ( !Gia_ObjIsLut(p, iLut) )
            continue;
        Delay = Gia_ObjComputeEdgeDelay( p, iLut, p->vEdgeDelay, p->vEdge1, p->vEdge2, fUseTwo );
        DelayMax = Abc_MaxInt( DelayMax, Delay );
    }
    Gia_ManUpdateMapping( p, vNodes, vWin );
    return DelayMax;
}
int Gia_ManEvalWindow( Gia_Man_t * p, Vec_Int_t * vLeaves, Vec_Int_t * vNodes, Vec_Wec_t * vWin, Vec_Int_t * vTemp, int fUseTwo )
{
    int DelayMax;
    assert( Vec_IntSize(vNodes) == Vec_WecSize(vWin) );
    Gia_ManUpdateMapping( p, vNodes, vWin );
    DelayMax = Gia_ManComputeEdgeDelay( p, fUseTwo );
    Gia_ManUpdateMapping( p, vNodes, vWin );
    return DelayMax;
}




/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Edg_ManToMapping( Gia_Man_t * p )
{
    int iObj, iFanin, k;
    assert( Gia_ManHasMapping(p) );
    Vec_WecFreeP( &p->vMapping2 );
    Vec_WecFreeP( &p->vFanouts2 );
    p->vMapping2 = Vec_WecStart( Gia_ManObjNum(p) );
    p->vFanouts2 = Vec_WecStart( Gia_ManObjNum(p) );
    Gia_ManForEachLut( p, iObj )
    {
        assert( Gia_ObjLutSize(p, iObj) <= 4 );
        Gia_LutForEachFanin( p, iObj, iFanin, k )
        {
            Vec_WecPush( p->vMapping2, iObj, iFanin );
            Vec_WecPush( p->vFanouts2, iFanin, iObj );
        }
    }
}

/**Function*************************************************************

  Synopsis    [Computes delay for the given edge assignment.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Edg_ObjEvalEdgeDelay( Gia_Man_t * p, int iObj, Vec_Int_t * vDelay )
{
    int DelayEdge = 0; // 2;
    int DelayNoEdge = 1;
    int i, iFan, Delay, DelayMax = 0;
    assert( Gia_ObjIsLut2(p, iObj) );
    Gia_LutForEachFanin2( p, iObj, iFan, i )
    {
        Delay = Vec_IntEntry(vDelay, iFan) + (Gia_ObjHaveEdge(p, iObj, iFan) ? DelayEdge : DelayNoEdge);
        DelayMax = Abc_MaxInt( DelayMax, Delay );
    }
    //printf( "Obj %d - Level %d\n", iObj, DelayMax );
    return DelayMax;
}
int Edg_ManEvalEdgeDelay( Gia_Man_t * p )
{
    int iLut, Delay, DelayMax = 0;
    assert( p->vEdge1 && p->vEdge2 );
    if ( p->vEdgeDelay == NULL )
        p->vEdgeDelay = Vec_IntStart( Gia_ManObjNum(p) );
    else
        Vec_IntFill( p->vEdgeDelay, Gia_ManObjNum(p), 0 );
    Gia_ManForEachLut2( p, iLut )
    {
        Delay = Edg_ObjEvalEdgeDelay(p, iLut, p->vEdgeDelay);
        Vec_IntWriteEntry( p->vEdgeDelay, iLut, Delay );
        DelayMax = Abc_MaxInt( DelayMax, Delay );
    }
    return DelayMax;
}

static inline int Edg_ObjEvalEdgeDelayR( Gia_Man_t * p, int iObj, Vec_Int_t * vDelay )
{
    int DelayEdge = 0; // 2;
    int DelayNoEdge = 1;
    int i, iFan, Delay, DelayMax = 0;
    assert( Gia_ObjIsLut2(p, iObj) );
    Gia_LutForEachFanout2( p, iObj, iFan, i )
    {
        Delay = Vec_IntEntry(vDelay, iFan) + (Gia_ObjHaveEdge(p, iObj, iFan) ? DelayEdge : DelayNoEdge);
        DelayMax = Abc_MaxInt( DelayMax, Delay );
    }
    //printf( "Obj %d - LevelR %d\n", iObj, DelayMax );
    return DelayMax;
}
int Edg_ManEvalEdgeDelayR( Gia_Man_t * p )
{
//    int k, DelayNoEdge = 1;
    int iLut, Delay, DelayMax = 0;
    assert( p->vEdge1 && p->vEdge2 );
    if ( p->vEdgeDelayR == NULL )
        p->vEdgeDelayR = Vec_IntStart( Gia_ManObjNum(p) );
    else
        Vec_IntFill( p->vEdgeDelayR, Gia_ManObjNum(p), 0 );
//    Gia_ManForEachCoDriverId( p, iLut, k )
//        Vec_IntWriteEntry( p->vEdgeDelayR, iLut, DelayNoEdge );
    Gia_ManForEachLut2Reverse( p, iLut )
    {
        Delay = Edg_ObjEvalEdgeDelayR(p, iLut, p->vEdgeDelayR);
        Vec_IntWriteEntry( p->vEdgeDelayR, iLut, Delay );
        DelayMax = Abc_MaxInt( DelayMax, Delay );
    }
    return DelayMax;
}

void Edg_ManCollectCritEdges( Gia_Man_t * p, Vec_Wec_t * vEdges, int DelayMax )
{
    Vec_Int_t * vLevel;
    int k, iLut, Delay1, Delay2;
    assert( p->vEdge1 && p->vEdge2 );
    Vec_WecClear( vEdges );
    Vec_WecInit( vEdges, DelayMax + 1 );
    Gia_ManForEachLut2( p, iLut )
    {
        Delay1 = Vec_IntEntry( p->vEdgeDelay, iLut );
        Delay2 = Vec_IntEntry( p->vEdgeDelayR, iLut );
        assert( Delay1 + Delay2 <= DelayMax );
        if ( Delay1 + Delay2 == DelayMax )
            Vec_WecPush( vEdges, Delay1, iLut );
    }
    // every level should have critical nodes, except the first one
    //Vec_WecPrint( vEdges, 0 );
    Vec_WecForEachLevelStart( vEdges, vLevel, k, 1 )
        assert( Vec_IntSize(vLevel) > 0 );
}


/**Function*************************************************************

  Synopsis    [Update one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Edg_ObjImprove( Gia_Man_t * p, int iObj, int nEdgeLimit, int DelayMax, int fVerbose )
{
    int nFaninsC = 0,   nFanoutsC = 0;    // critical
    int nFaninsEC = 0,  nFanoutsEC = 0;   // edge-critical
    int nFaninsENC = 0, nFanoutsENC = 0;  // edge-non-critial
    int pFanins[4], pFanouts[4];
    int nEdgeDiff, nEdges = 0, Count = 0;
    int i, iNext, Delay1, Delay2;
    // count how many fanins have critical edge
    Delay1 = Vec_IntEntry( p->vEdgeDelayR, iObj );
    //if ( Delay1 > 1 )
    Gia_LutForEachFanin2( p, iObj, iNext, i )
    {
        if ( !Gia_ObjIsAnd(Gia_ManObj(p, iNext)) )
            continue;
        Delay2 = Vec_IntEntry( p->vEdgeDelay, iNext );
        if ( Gia_ObjHaveEdge(p, iObj, iNext) )
        {
            nEdges++;
            assert( Delay1 + Delay2 <= DelayMax );
            if ( Delay1 + Delay2 == DelayMax )
                nFaninsEC++;
            else
                nFaninsENC++;
        }
        else
        {
            assert( Delay1 + Delay2 + 1 <= DelayMax );
            if ( Delay1 + Delay2 + 1 == DelayMax )
                pFanins[nFaninsC++] = iNext;
        }
    }
    // count how many fanouts have critical edge
    Delay1 = Vec_IntEntry( p->vEdgeDelay, iObj );
    //if ( Delay2 < DelayMax - 1 )
    Gia_LutForEachFanout2( p, iObj, iNext, i )
    {
        //if ( !Gia_ObjIsAnd(Gia_ManObj(p, iNext)) )
        //    continue;
        assert( Gia_ObjIsAnd(Gia_ManObj(p, iNext)) );
        Delay2 = Vec_IntEntry( p->vEdgeDelayR, iNext );
        if ( Gia_ObjHaveEdge(p, iObj, iNext) )
        {
            nEdges++;
            assert( Delay1 + Delay2 <= DelayMax );
            if ( Delay1 + Delay2 == DelayMax )
                nFanoutsEC++;
            else
                nFanoutsENC++;
        }
        else
        {
            assert( Delay1 + Delay2 + 1 <= DelayMax );
            if ( Delay1 + Delay2 + 1 == DelayMax )
            {
                if ( nFanoutsC < nEdgeLimit )
                    pFanouts[nFanoutsC] = iNext;
                nFanoutsC++;
            }
        }
    }
    if ( fVerbose )
    {
        printf( "%8d : ", iObj );
        printf( "Edges = %d  ", nEdges );
        printf( "Fanins (all %d  EC %d  ENC %d  C %d)  ", 
            Gia_ObjLutSize2(p, iObj), nFaninsEC, nFaninsENC, nFaninsC );
        printf( "Fanouts (all %d  EC %d  ENC %d  C %d)  ", 
            Gia_ObjLutFanoutNum2(p, iObj), nFanoutsEC, nFanoutsENC, nFanoutsC );
    }
    // consider simple cases
    assert( nEdges <= nEdgeLimit );
    if ( nEdges == nEdgeLimit )
    {
        if ( fVerbose )
            printf( "Full\n" );
        return 0;
    }
    nEdgeDiff = nEdgeLimit - nEdges;
    // check if fanins or fanouts could be improved
    if ( nFaninsEC == 0 && nFaninsC && nFaninsC <= nEdgeDiff )
    {
        for ( i = 0; i < nFaninsC; i++ )
            if ( Gia_ObjEdgeCount(pFanins[i], p->vEdge1, p->vEdge2) == nEdgeLimit )
                break;
        if ( i == nFaninsC )
        {
            for ( i = 0; i < nFaninsC; i++ )
            {
                Count += Gia_ObjEdgeAdd( iObj, pFanins[i], p->vEdge1, p->vEdge2 );
                Count += Gia_ObjEdgeAdd( pFanins[i], iObj, p->vEdge1, p->vEdge2 );
            }
            if ( Count )
                printf( "Wrong number of edges.\n" );
            if ( fVerbose )
                printf( "Fixed %d critical fanins\n", nFaninsC );
            return 1;
        }
    }
    if ( nFanoutsEC == 0 && nFanoutsC && nFanoutsC <= nEdgeDiff )
    {
        for ( i = 0; i < nFanoutsC; i++ )
            if ( Gia_ObjEdgeCount(pFanouts[i], p->vEdge1, p->vEdge2) == nEdgeLimit )
                break;
        if ( i == nFanoutsC )
        {
            for ( i = 0; i < nFanoutsC; i++ )
            {
                Count += Gia_ObjEdgeAdd( iObj, pFanouts[i], p->vEdge1, p->vEdge2 );
                Count += Gia_ObjEdgeAdd( pFanouts[i], iObj, p->vEdge1, p->vEdge2 );
            }
            if ( Count )
                printf( "Wrong number of edges.\n" );
            if ( fVerbose )
                printf( "Fixed %d critical fanouts\n", nFanoutsC );
            return 1;
        }
    }
    if ( fVerbose )
        printf( "Cannot fix\n" );
    return 0;
}

/**Function*************************************************************

  Synopsis    [Finds edge assignment.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Edg_ManAssignEdgeNew( Gia_Man_t * p, int nEdges, int fVerbose )
{
    int DelayNoEdge = 1;
    int fLevelVerbose = 0;
    Vec_Int_t * vLevel;
    Vec_Wec_t * vEdges = Vec_WecStart(0);
    Vec_Int_t * vEdge1 = NULL, * vEdge2 = NULL;
    int DelayD = 0, DelayR = 0, DelayPrev = ABC_INFINITY;
    int k, j, i, iLast = -1, iObj;
    if ( fVerbose )
        printf( "Running edge assignment with E = %d.\n", nEdges );
    // create fanouts
    Edg_ManToMapping( p );
    // create empty assignment
    Vec_IntFreeP( &p->vEdge1 );
    Vec_IntFreeP( &p->vEdge2 );
    p->vEdge1 = Vec_IntStart( Gia_ManObjNum(p) );
    p->vEdge2 = Vec_IntStart( Gia_ManObjNum(p) );
    // perform optimization
    for ( i = 0; i < 10000; i++ )
    {
        // if there is no improvement after 10 iterations, quit
        if ( i > iLast + 50 )
            break;
        // create delay information
        DelayD = Edg_ManEvalEdgeDelay( p );
        DelayR = Edg_ManEvalEdgeDelayR( p );
        assert( DelayD == DelayR + DelayNoEdge );
        if ( DelayPrev > DelayD )
        {
            //printf( "Saving backup point at %d levels.\n", DelayD );
            Vec_IntFreeP( &vEdge1 );  vEdge1 = Vec_IntDup( p->vEdge1 );
            Vec_IntFreeP( &vEdge2 );  vEdge2 = Vec_IntDup( p->vEdge2 );
            DelayPrev = DelayD;
            iLast = i;
        }
        if ( fVerbose )
            printf( "\nIter %4d : Delay = %4d\n", i, DelayD );
        // collect critical nodes (nodes with critical edges)
        Edg_ManCollectCritEdges( p, vEdges, DelayD );
        // sort levels according to the number of critical edges
        if ( fLevelVerbose )
        {
            Vec_WecForEachLevel( vEdges, vLevel, k )
                Vec_IntPush( vLevel, k );
        }
        Vec_WecSort( vEdges, 0 );
        if ( fLevelVerbose )
        {
            Vec_WecForEachLevel( vEdges, vLevel, k )
            {
                int Level = Vec_IntPop( vLevel );
                printf( "%d: Level %2d : ", k, Level );
                Vec_IntPrint( vLevel );
            }
        }
        Vec_WecForEachLevel( vEdges, vLevel, k )
        {
            Vec_IntForEachEntry( vLevel, iObj, j )
                if ( Edg_ObjImprove(p, iObj, nEdges, DelayD, fVerbose) ) // improved
                    break;
            if ( j < Vec_IntSize(vLevel) )
                break;
        }
        if ( k == Vec_WecSize(vEdges) ) // if we could not improve anything, quit
            break;
    }
    Vec_WecFree( vEdges );
    // update to the saved version
    Vec_IntFreeP( &p->vEdge1 );  p->vEdge1 = vEdge1;
    Vec_IntFreeP( &p->vEdge2 );  p->vEdge2 = vEdge2;
    return DelayD;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

