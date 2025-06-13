/**CFile****************************************************************

  FileName    [giaFanout.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Scalable AIG package.]

  Synopsis    []

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: giaFanout.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "gia.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

// 0: first iFan
// 1: prev iFan0
// 2: prev iFan1
// 3: next iFan0
// 4: next iFan1

static inline int   Gia_FanoutCreate( int FanId, int Num )    { assert( Num < 2 ); return (FanId << 1) | Num;   } 
static inline int * Gia_FanoutObj( int * pData, int ObjId )   { return pData + 5*ObjId;                         }
static inline int * Gia_FanoutPrev( int * pData, int iFan )   { return pData + 5*(iFan >> 1) + 1 + (iFan & 1);  }
static inline int * Gia_FanoutNext( int * pData, int iFan )   { return pData + 5*(iFan >> 1) + 3 + (iFan & 1);  }

// these two procedures are only here for the use inside the iterator
static inline int     Gia_ObjFanout0Int( Gia_Man_t * p, int ObjId )  { assert(ObjId < p->nFansAlloc);  return p->pFanData[5*ObjId];                         }
static inline int     Gia_ObjFanoutNext( Gia_Man_t * p, int iFan )   { assert(iFan/2 < p->nFansAlloc); return p->pFanData[5*(iFan >> 1) + 3 + (iFan & 1)];  }

// iterator over the fanouts
#define Gia_ObjForEachFanout( p, pObj, pFanout, iFan, i )                       \
    for ( assert(p->pFanData), i = 0; (i < (int)(pObj)->nRefs) &&               \
          (((iFan) = i? Gia_ObjFanoutNext(p, iFan) : Gia_ObjFanout0Int(p, Gia_ObjId(p, pObj))), 1) && \
          (((pFanout) = Gia_ManObj(p, iFan>>1)), 1); i++ )

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Create fanout for all objects in the manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManFanoutStart( Gia_Man_t * p )
{
    Gia_Obj_t * pObj;
    int i;
    // allocate fanout datastructure
    assert( p->pFanData == NULL );
    p->nFansAlloc = 2 * Gia_ManObjNum(p);
    if ( p->nFansAlloc < (1<<12) )
        p->nFansAlloc = (1<<12);
    p->pFanData = ABC_ALLOC( int, 5 * p->nFansAlloc );
    memset( p->pFanData, 0, sizeof(int) * 5 * p->nFansAlloc );
    // add fanouts for all objects
    Gia_ManForEachObj( p, pObj, i )
    {
        if ( Gia_ObjChild0(pObj) )
            Gia_ObjAddFanout( p, Gia_ObjFanin0(pObj), pObj );
        if ( Gia_ObjChild1(pObj) )
            Gia_ObjAddFanout( p, Gia_ObjFanin1(pObj), pObj );
    }
}

/**Function*************************************************************

  Synopsis    [Deletes fanout for all objects in the manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManFanoutStop( Gia_Man_t * p )
{
    assert( p->pFanData != NULL );
    ABC_FREE( p->pFanData );
    p->nFansAlloc = 0;
}

/**Function*************************************************************

  Synopsis    [Adds fanout (pFanout) of node (pObj).]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ObjAddFanout( Gia_Man_t * p, Gia_Obj_t * pObj, Gia_Obj_t * pFanout )
{
    int iFan, * pFirst, * pPrevC, * pNextC, * pPrev, * pNext;
    assert( p->pFanData );
    assert( !Gia_IsComplement(pObj) && !Gia_IsComplement(pFanout) );
    assert( Gia_ObjId(p, pFanout) > 0 );
    if ( Gia_ObjId(p, pObj) >= p->nFansAlloc || Gia_ObjId(p, pFanout) >= p->nFansAlloc )
    {
        int nFansAlloc = 2 * Abc_MaxInt( Gia_ObjId(p, pObj), Gia_ObjId(p, pFanout) ); 
        p->pFanData = ABC_REALLOC( int, p->pFanData, 5 * nFansAlloc );
        memset( p->pFanData + 5 * p->nFansAlloc, 0, sizeof(int) * 5 * (nFansAlloc - p->nFansAlloc) );
        p->nFansAlloc = nFansAlloc;
    }
    assert( Gia_ObjId(p, pObj) < p->nFansAlloc && Gia_ObjId(p, pFanout) < p->nFansAlloc );
    iFan   = Gia_FanoutCreate( Gia_ObjId(p, pFanout), Gia_ObjWhatFanin(p, pFanout, pObj) );
    pPrevC = Gia_FanoutPrev( p->pFanData, iFan );
    pNextC = Gia_FanoutNext( p->pFanData, iFan );
    pFirst = Gia_FanoutObj( p->pFanData, Gia_ObjId(p, pObj) );
    if ( *pFirst == 0 )
    {
        *pFirst = iFan;
        *pPrevC = iFan;
        *pNextC = iFan;
    }
    else
    {
        pPrev = Gia_FanoutPrev( p->pFanData, *pFirst );
        pNext = Gia_FanoutNext( p->pFanData, *pPrev );
        assert( *pNext == *pFirst );
        *pPrevC = *pPrev;
        *pNextC = *pFirst;
        *pPrev  = iFan;
        *pNext  = iFan;
    }
}

/**Function*************************************************************

  Synopsis    [Removes fanout (pFanout) of node (pObj).]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ObjRemoveFanout( Gia_Man_t * p, Gia_Obj_t * pObj, Gia_Obj_t * pFanout )
{
    int iFan, * pFirst, * pPrevC, * pNextC, * pPrev, * pNext;
    assert( p->pFanData && Gia_ObjId(p, pObj) < p->nFansAlloc && Gia_ObjId(p, pFanout) < p->nFansAlloc );
    assert( !Gia_IsComplement(pObj) && !Gia_IsComplement(pFanout) );
    assert( Gia_ObjId(p, pFanout) > 0 );
    iFan   = Gia_FanoutCreate( Gia_ObjId(p, pFanout), Gia_ObjWhatFanin(p, pFanout, pObj) );
    pPrevC = Gia_FanoutPrev( p->pFanData, iFan );
    pNextC = Gia_FanoutNext( p->pFanData, iFan );
    pPrev  = Gia_FanoutPrev( p->pFanData, *pNextC );
    pNext  = Gia_FanoutNext( p->pFanData, *pPrevC );
    assert( *pPrev == iFan );
    assert( *pNext == iFan );
    pFirst = Gia_FanoutObj( p->pFanData, Gia_ObjId(p, pObj) );
    assert( *pFirst > 0 );
    if ( *pFirst == iFan )
    {
        if ( *pNextC == iFan )
        {
            *pFirst = 0;
            *pPrev  = 0;
            *pNext  = 0;
            *pPrevC = 0;
            *pNextC = 0;
            return;
        }
        *pFirst = *pNextC;
    }
    *pPrev  = *pPrevC;
    *pNext  = *pNextC;
    *pPrevC = 0;
    *pNextC = 0;
}




/**Function*************************************************************

  Synopsis    [Compute the map of all edges.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Gia_ManStartFanoutMap( Gia_Man_t * p, Vec_Int_t * vFanoutNums )
{
    Vec_Int_t * vEdgeMap;
    Gia_Obj_t * pObj;
    int i, iOffset;
    iOffset  = Gia_ManObjNum(p);
    vEdgeMap = Vec_IntStart( iOffset + Gia_ManMuxNum(p) + 2 * Gia_ManAndNum(p) + Gia_ManCoNum(p) - Gia_ManBufNum(p) );
    Gia_ManForEachObj( p, pObj, i )
    {
        Vec_IntWriteEntry( vEdgeMap, i, iOffset );
        iOffset += Vec_IntEntry( vFanoutNums, Gia_ObjId(p, pObj) );
    }
    assert( iOffset <= Vec_IntSize(vEdgeMap) );
    return vEdgeMap;
}

/**Function*************************************************************

  Synopsis    [Allocates static fanout.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManStaticFanoutStart( Gia_Man_t * p )
{
    Vec_Int_t * vCounts;
    int * pRefsOld;
    Gia_Obj_t * pObj, * pFanin;
    int i, iFanout;
    assert( p->vFanoutNums == NULL );
    assert( p->vFanout == NULL );
    // recompute reference counters
    pRefsOld = p->pRefs; p->pRefs = NULL;
    Gia_ManCreateRefs(p);
    p->vFanoutNums = Vec_IntAllocArray( p->pRefs, Gia_ManObjNum(p) );
    p->pRefs = pRefsOld;
    // start the fanout maps
    p->vFanout = Gia_ManStartFanoutMap( p, p->vFanoutNums );
    // incrementally add fanouts
    vCounts = Vec_IntStart( Gia_ManObjNum(p) );
    Gia_ManForEachObj( p, pObj, i )
    {
        if ( Gia_ObjIsAnd(pObj) || Gia_ObjIsCo(pObj) )
        {
            pFanin = Gia_ObjFanin0(pObj);
            iFanout = Vec_IntEntry( vCounts, Gia_ObjId(p, pFanin) );
            Gia_ObjSetFanout( p, pFanin, iFanout, pObj );
            Vec_IntAddToEntry( vCounts, Gia_ObjId(p, pFanin), 1 );
        }
        if ( Gia_ObjIsAnd(pObj) && !Gia_ObjIsBuf(pObj) )
        {
            pFanin = Gia_ObjFanin1(pObj);
            iFanout = Vec_IntEntry( vCounts, Gia_ObjId(p, pFanin) );
            Gia_ObjSetFanout( p, pFanin, iFanout, pObj );
            Vec_IntAddToEntry( vCounts, Gia_ObjId(p, pFanin), 1 );
        }
        if ( Gia_ObjIsMux(p, pObj) )
        {

            pFanin = Gia_ObjFanin2(p, pObj);
            iFanout = Vec_IntEntry( vCounts, Gia_ObjId(p, pFanin) );
            Gia_ObjSetFanout( p, pFanin, iFanout, pObj );
            Vec_IntAddToEntry( vCounts, Gia_ObjId(p, pFanin), 1 );
        }
    }
    // double-check the current number of fanouts added
    Gia_ManForEachObj( p, pObj, i )
        assert( Vec_IntEntry(vCounts, i) == Gia_ObjFanoutNum(p, pObj) );
    Vec_IntFree( vCounts );
}


/**Function*************************************************************

  Synopsis    [Compute the map of all edges.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Gia_ManStartMappingFanoutMap( Gia_Man_t * p, Vec_Int_t * vFanoutNums )
{
    Gia_Obj_t * pObj;
    int i, iOffset = Gia_ManObjNum(p);
    Vec_Int_t * vEdgeMap = Vec_IntAlloc( 2 * iOffset );
    Vec_IntFill( vEdgeMap, iOffset, 0 );
    Gia_ManForEachObj( p, pObj, i )
    {
        if ( Vec_IntEntry(vFanoutNums, i) == 0 )
            continue;
        Vec_IntWriteEntry( vEdgeMap, i, iOffset );
        iOffset += Vec_IntEntry( vFanoutNums, i );
        Vec_IntFillExtra( vEdgeMap, iOffset, 0 );
    }
    //printf( "Fanout map is %.2fx larger than AIG manager.\n", 1.0*Vec_IntSize(vEdgeMap)/Gia_ManObjNum(p) );
    return vEdgeMap;
}

/**Function*************************************************************

  Synopsis    [Allocates static fanout.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ObjCheckDupMappingFanins( Gia_Man_t * p, int iObj )
{
    int * pFanins = Gia_ObjLutFanins( p, iObj );
    int i, k, nFanins = Gia_ObjLutSize( p, iObj );
    for ( i = 0; i < nFanins; i++ )
    for ( k = i + 1; k < nFanins; k++ )
        assert( pFanins[i] != pFanins[k] );
}
void Gia_ManStaticMappingFanoutStart( Gia_Man_t * p, Vec_Int_t ** pvIndex )
{
    Vec_Int_t * vCounts;
    int * pRefsOld;
    Gia_Obj_t * pObj, * pFanin;
    int i, k, iFan, iFanout, Index;
    assert( p->vFanoutNums == NULL );
    assert( p->vFanout == NULL );
    // recompute reference counters
    pRefsOld = p->pLutRefs; p->pLutRefs = NULL;
    Gia_ManSetLutRefs(p);
    p->vFanoutNums = Vec_IntAllocArray( p->pLutRefs, Gia_ManObjNum(p) );
    p->pLutRefs = pRefsOld;
    // start the fanout maps
    p->vFanout = Gia_ManStartMappingFanoutMap( p, p->vFanoutNums );
    if ( pvIndex )
        *pvIndex = Vec_IntStart( Vec_IntSize(p->vFanout) );
    // incrementally add fanouts
    vCounts = Vec_IntStart( Gia_ManObjNum(p) );
    Gia_ManForEachLut( p, i )
    {
        Gia_ObjCheckDupMappingFanins( p, i );
        pObj = Gia_ManObj( p, i );
        Gia_LutForEachFaninIndex( p, i, iFan, k, Index )
        {
            pFanin = Gia_ManObj( p, iFan );
            iFanout = Vec_IntEntry( vCounts, iFan );
            Gia_ObjSetFanout( p, pFanin, iFanout, pObj );
            Vec_IntAddToEntry( vCounts, iFan, 1 );
            if ( pvIndex )
                Vec_IntWriteEntry( *pvIndex, Vec_IntEntry(p->vFanout, iFan) + iFanout, Index );
        }
    }
    Gia_ManForEachCo( p, pObj, i )
    {
        iFan = Gia_ObjFaninId0p(p, pObj);
        pFanin = Gia_ManObj( p, iFan );
        iFanout = Vec_IntEntry( vCounts, iFan );
        Gia_ObjSetFanout( p, pFanin, iFanout, pObj );
        Vec_IntAddToEntry( vCounts, iFan, 1 );
    }
    // double-check the current number of fanouts added
    Gia_ManForEachObj( p, pObj, i )
        assert( Vec_IntEntry(vCounts, i) == Gia_ObjFanoutNum(p, pObj) );
    Vec_IntFree( vCounts );
}

/**Function*************************************************************

  Synopsis    [Deallocates static fanout.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManStaticFanoutStop( Gia_Man_t * p )
{
    Vec_IntFreeP( &p->vFanoutNums );
    Vec_IntFreeP( &p->vFanout );
}


/**Function*************************************************************

  Synopsis    [Tests static fanout.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManStaticFanoutTest( Gia_Man_t * p )
{
    Gia_Obj_t * pObj, * pFanout;
    int i, k;
    Gia_ManStaticFanoutStart( p );
    Gia_ManForEachObj( p, pObj, i )
    {
        Gia_ObjPrint( p, pObj );
        printf( "   Fanouts : " );
        Gia_ObjForEachFanoutStatic( p, pObj, pFanout, k )
            printf( "%5d ", Gia_ObjId(p, pFanout) );
        printf( "\n" );
    }
    Gia_ManStaticFanoutStop( p );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

