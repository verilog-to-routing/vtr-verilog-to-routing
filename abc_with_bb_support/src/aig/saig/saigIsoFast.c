/**CFile****************************************************************

  FileName    [aigIsoFast.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [AIG package.]

  Synopsis    [Graph isomorphism package.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - April 28, 2007.]

  Revision    [$Id: aigIsoFast.c,v 1.00 2007/04/28 00:00:00 alanmi Exp $]

***********************************************************************/

#include "saig.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#define AIG_ISO_NUM 16

typedef struct Iso_Dat_t_ Iso_Dat_t;
struct Iso_Dat_t_
{
    unsigned      nFiNeg    :  3;
    unsigned      nFoNeg    :  2;
    unsigned      nFoPos    :  2;
    unsigned      Fi0Lev    :  3;
    unsigned      Fi1Lev    :  3;
    unsigned      Level     :  3;
    unsigned      fVisit    : 16;
};

typedef struct Iso_Dat2_t_ Iso_Dat2_t;
struct Iso_Dat2_t_
{
    unsigned      Data      : 16;
};

typedef struct Iso_Sto_t_ Iso_Sto_t;
struct Iso_Sto_t_
{
    Aig_Man_t *   pAig;       // user's AIG manager
    int           nObjs;      // number of objects
    Iso_Dat_t *   pData;      // data for each object
    Vec_Int_t *   vVisited;   // visited nodes
    Vec_Ptr_t *   vRoots;     // root nodes
    Vec_Int_t *   vPlaces;    // places in the counter lists
    int *         pCounters;  // counters    
};

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Iso_Sto_t * Iso_StoStart( Aig_Man_t * pAig )
{
    Iso_Sto_t * p;
    p = ABC_CALLOC( Iso_Sto_t, 1 );
    p->pAig      = pAig;
    p->nObjs     = Aig_ManObjNumMax( pAig );
    p->pData     = ABC_CALLOC( Iso_Dat_t, p->nObjs );
    p->vVisited  = Vec_IntStart( 1000 );
    p->vPlaces   = Vec_IntStart( 1000 );
    p->vRoots    = Vec_PtrStart( 1000 );
    p->pCounters = ABC_CALLOC( int, (1 << AIG_ISO_NUM) );
    return p;
}
void Iso_StoStop( Iso_Sto_t * p )
{
    Vec_IntFree( p->vPlaces );
    Vec_IntFree( p->vVisited );
    Vec_PtrFree( p->vRoots );
    ABC_FREE( p->pCounters );
    ABC_FREE( p->pData );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    [Collect statistics about one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Iso_StoCollectInfo_rec( Aig_Man_t * p, Aig_Obj_t * pObj, int fCompl, Vec_Int_t * vVisited, Iso_Dat_t * pData, Vec_Ptr_t * vRoots )
{
    Iso_Dat_t * pThis = pData + Aig_ObjId(pObj);
    assert( Aig_ObjIsCi(pObj) || Aig_ObjIsNode(pObj) );
    if ( pThis->fVisit )
    {
        if ( fCompl )
            pThis->nFoNeg++;
        else
            pThis->nFoPos++;
        return;
    }
    assert( *((int *)pThis) == 0 );
    pThis->fVisit = 1;
    if ( fCompl )
        pThis->nFoNeg++;
    else
        pThis->nFoPos++;
    pThis->Level = pObj->Level;
    pThis->nFiNeg = Aig_ObjFaninC0(pObj) + Aig_ObjFaninC1(pObj);
    if ( Aig_ObjIsNode(pObj) )
    {
        if ( Aig_ObjFaninC0(pObj) < Aig_ObjFaninC1(pObj) || (Aig_ObjFaninC0(pObj) == Aig_ObjFaninC1(pObj) && Aig_ObjFanin0(pObj)->Level < Aig_ObjFanin1(pObj)->Level) )
        {
            pThis->Fi0Lev = pObj->Level - Aig_ObjFanin0(pObj)->Level;
            pThis->Fi1Lev = pObj->Level - Aig_ObjFanin1(pObj)->Level;
        }
        else
        {
            pThis->Fi0Lev = pObj->Level - Aig_ObjFanin1(pObj)->Level;
            pThis->Fi1Lev = pObj->Level - Aig_ObjFanin0(pObj)->Level;
        }
        Iso_StoCollectInfo_rec( p, Aig_ObjFanin0(pObj), Aig_ObjFaninC0(pObj), vVisited, pData, vRoots );
        Iso_StoCollectInfo_rec( p, Aig_ObjFanin1(pObj), Aig_ObjFaninC1(pObj), vVisited, pData, vRoots );
    }
    else if ( Saig_ObjIsLo(p, pObj) )
    {
        pThis->Fi0Lev = 1;
        pThis->Fi1Lev = 0;
        Vec_PtrPush( vRoots, Saig_ObjLoToLi(p, pObj) );
    }
    else if ( Saig_ObjIsPi(p, pObj) )
    {
        pThis->Fi0Lev = 0;
        pThis->Fi1Lev = 0;
    }
    else
        assert( 0 );
    assert( pThis->nFoNeg + pThis->nFoPos );
    Vec_IntPush( vVisited, Aig_ObjId(pObj) );
}

//static abctime time_Trav = 0;

/**Function*************************************************************

  Synopsis    [Collect statistics about one output.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Iso_StoCollectInfo( Iso_Sto_t * p, Aig_Obj_t * pPo )
{
    int fVerboseShow = 0;
    Vec_Int_t * vInfo;
    Iso_Dat2_t * pData2 = (Iso_Dat2_t *)p->pData;
    Aig_Man_t * pAig = p->pAig;
    Aig_Obj_t * pObj;
    int i, Value, Entry, * pPerm;
//    abctime clk = Abc_Clock();

    assert( Aig_ObjIsCo(pPo) );

    // collect initial POs
    Vec_IntClear( p->vVisited );
    Vec_PtrClear( p->vRoots );
    Vec_PtrPush( p->vRoots, pPo );

    // mark internal nodes
    Vec_PtrForEachEntry( Aig_Obj_t *, p->vRoots, pObj, i )
        if ( !Aig_ObjIsConst1(Aig_ObjFanin0(pObj)) )
            Iso_StoCollectInfo_rec( pAig, Aig_ObjFanin0(pObj), Aig_ObjFaninC0(pObj), p->vVisited, p->pData, p->vRoots );
//    time_Trav += Abc_Clock() - clk;

    // count how many times each data entry appears
    Vec_IntClear( p->vPlaces );
    Vec_IntForEachEntry( p->vVisited, Entry, i )
    {
        Value = pData2[Entry].Data;
//        assert( Value > 0 );
        if ( p->pCounters[Value]++ == 0 )
            Vec_IntPush( p->vPlaces, Value );
//        pData2[Entry].Data = 0;
        *((int *)(p->pData + Entry)) = 0;
    }

    // collect non-trivial counters
    Vec_IntClear( p->vVisited );
    Vec_IntForEachEntry( p->vPlaces, Entry, i )
    {
        assert( p->pCounters[Entry] );
        Vec_IntPush( p->vVisited, p->pCounters[Entry] );
        p->pCounters[Entry] = 0;
    }
//    printf( "%d ", Vec_IntSize(p->vVisited) );

    // sort the costs in the increasing order
    pPerm = Abc_MergeSortCost( Vec_IntArray(p->vVisited), Vec_IntSize(p->vVisited) );
    assert( Vec_IntEntry(p->vVisited, pPerm[0]) <= Vec_IntEntry(p->vVisited, pPerm[Vec_IntSize(p->vVisited)-1]) );

    // create information
    vInfo = Vec_IntAlloc( Vec_IntSize(p->vVisited) );
    for ( i = Vec_IntSize(p->vVisited)-1; i >= 0; i-- )
    {
        Entry = Vec_IntEntry( p->vVisited, pPerm[i] );
        Entry = (Entry << AIG_ISO_NUM) | Vec_IntEntry( p->vPlaces, pPerm[i] );
        Vec_IntPush( vInfo, Entry );
    }
    ABC_FREE( pPerm );

    // show the final result
    if ( fVerboseShow )
    Vec_IntForEachEntry( vInfo, Entry, i )
    {
        Iso_Dat2_t Data = { Entry & 0xFFFF };
        Iso_Dat_t * pData = (Iso_Dat_t *)&Data;

        printf( "%6d : ", i );
        printf( "Freq =%6d ", Entry >> 16 );

        printf( "FiNeg =%3d ", pData->nFiNeg );
        printf( "FoNeg =%3d ", pData->nFoNeg );
        printf( "FoPos =%3d ", pData->nFoPos );
        printf( "Fi0L  =%3d ", pData->Fi0Lev );
        printf( "Fi1L  =%3d ", pData->Fi1Lev );
        printf( "Lev   =%3d ", pData->Level  );
        printf( "\n" );
    }
    return vInfo;
}

/**Function*************************************************************

  Synopsis    [Takes multi-output sequential AIG.]

  Description [Returns candidate equivalence classes of POs.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Iso_StoCompareVecInt( Vec_Int_t ** p1, Vec_Int_t ** p2 )
{
    return Vec_IntCompareVec( *p1, *p2 );
}

/**Function*************************************************************

  Synopsis    [Takes multi-output sequential AIG.]

  Description [Returns candidate equivalence classes of POs.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Vec_t * Saig_IsoDetectFast( Aig_Man_t * pAig )
{
    Iso_Sto_t * pMan;
    Aig_Obj_t * pObj;
    Vec_Ptr_t * vClasses, * vInfos;
    Vec_Int_t * vInfo, * vPrev, * vLevel;
    int i, Number, nUnique = 0;
    abctime clk = Abc_Clock();

    // collect infos and remember their number
    pMan = Iso_StoStart( pAig );
    vInfos = Vec_PtrAlloc( Saig_ManPoNum(pAig) );
    Saig_ManForEachPo( pAig, pObj, i )
    {
        vInfo = Iso_StoCollectInfo(pMan, pObj);
        Vec_IntPush( vInfo, i );
        Vec_PtrPush( vInfos, vInfo );
    }
    Iso_StoStop( pMan );
    Abc_PrintTime( 1, "Info computation time", Abc_Clock() - clk );

    // sort the infos
    clk = Abc_Clock();
    Vec_PtrSort( vInfos, (int (*)(void))Iso_StoCompareVecInt );

    // create classes
    clk = Abc_Clock();
    vClasses = Vec_PtrAlloc( Saig_ManPoNum(pAig) );
    // start the first class
    Vec_PtrPush( vClasses, (vLevel = Vec_IntAlloc(4)) );
    vPrev = (Vec_Int_t *)Vec_PtrEntry( vInfos, 0 );
    Vec_IntPush( vLevel, Vec_IntPop(vPrev) );
    // consider other classes
    Vec_PtrForEachEntryStart( Vec_Int_t *, vInfos, vInfo, i, 1 )
    {
        Number = Vec_IntPop( vInfo );
        if ( Vec_IntCompareVec(vPrev, vInfo) )
            Vec_PtrPush( vClasses, Vec_IntAlloc(4) );
        vLevel = (Vec_Int_t *)Vec_PtrEntryLast( vClasses );
        Vec_IntPush( vLevel, Number );
        vPrev = vInfo;
    }
    Vec_VecFree( (Vec_Vec_t *)vInfos );
    Abc_PrintTime( 1, "Sorting time", Abc_Clock() - clk );
//    Abc_PrintTime( 1, "Traversal time", time_Trav );

    // report the results
//    Vec_VecPrintInt( (Vec_Vec_t *)vClasses );
    printf( "Devided %d outputs into %d cand equiv classes.\n", Saig_ManPoNum(pAig), Vec_PtrSize(vClasses) );

    Vec_PtrForEachEntry( Vec_Int_t *, vClasses, vLevel, i )
        if ( Vec_IntSize(vLevel) > 1 )
            printf( "%d ", Vec_IntSize(vLevel) );
        else
            nUnique++;
    printf( " Unique = %d\n", nUnique );

//    return (Vec_Vec_t *)vClasses;
    Vec_VecFree( (Vec_Vec_t *)vClasses );
    return NULL;
}



////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

