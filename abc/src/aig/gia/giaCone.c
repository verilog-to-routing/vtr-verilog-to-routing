/**CFile****************************************************************

  FileName    [giaCone.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Scalable AIG package.]

  Synopsis    []

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: giaCone.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "gia.h"
#include "misc/extra/extra.h"
#include "misc/vec/vecHsh.h"
#include "misc/vec/vecWec.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Opa_Man_t_ Opa_Man_t;
struct Opa_Man_t_
{
    Gia_Man_t *    pGia;
    Vec_Int_t *    vFront;
    Vec_Int_t *    pvParts;
    int *          pId2Part; 
    int            nParts;
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
static inline Opa_Man_t * Opa_ManStart( Gia_Man_t * pGia)
{
    Opa_Man_t * p;
    Gia_Obj_t * pObj;
    int i;
    p = ABC_CALLOC( Opa_Man_t, 1 );
    p->pGia = pGia;
    p->pvParts  = ABC_CALLOC( Vec_Int_t, Gia_ManPoNum(pGia) );
    p->pId2Part = ABC_FALLOC( int, Gia_ManObjNum(pGia) );
    p->vFront   = Vec_IntAlloc( 100 );
    Gia_ManForEachPo( pGia, pObj, i )
    {
        Vec_IntPush( p->pvParts + i, Gia_ObjId(pGia, pObj) );
        p->pId2Part[Gia_ObjId(pGia, pObj)] = i;
        Vec_IntPush( p->vFront, Gia_ObjId(pGia, pObj) );
    }
    p->nParts = Gia_ManPoNum(pGia);
    return p;
}
static inline void Opa_ManStop( Opa_Man_t * p )
{
    int i;
    Vec_IntFree( p->vFront );
    for ( i = 0; i < Gia_ManPoNum(p->pGia); i++ )
        ABC_FREE( p->pvParts[i].pArray );
    ABC_FREE( p->pvParts );
    ABC_FREE( p->pId2Part );
    ABC_FREE( p );
}
static inline void Opa_ManPrint( Opa_Man_t * p )
{
    int i, k;
    printf( "Groups:\n" );
    for ( i = 0; i < Gia_ManPoNum(p->pGia); i++ )
    {
        if ( p->pvParts[i].nSize == 0 )
            continue;
        printf( "%3d : ", i );
        for ( k = 0; k < p->pvParts[i].nSize; k++ )
            printf( "%d ", p->pvParts[i].pArray[k] );
        printf( "\n" );
    }
}
static inline void Opa_ManPrint2( Opa_Man_t * p )
{
    Gia_Obj_t * pObj;
    int i, k, Count;
    printf( "Groups %d: ", p->nParts );
    for ( i = 0; i < Gia_ManPoNum(p->pGia); i++ )
    {
        if ( p->pvParts[i].nSize == 0 )
            continue;
        // count POs in this group
        Count = 0;
        Gia_ManForEachObjVec( p->pvParts + i, p->pGia, pObj, k )
            Count += Gia_ObjIsPo(p->pGia, pObj);
        printf( "%d ", Count );
    }
    printf( "\n" );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Opa_ManMoveOne( Opa_Man_t * p, Gia_Obj_t * pObj, Gia_Obj_t * pFanin )
{
    int iObj   = Gia_ObjId(p->pGia, pObj);
    int iFanin = Gia_ObjId(p->pGia, pFanin);
    if ( iFanin == 0 )
        return;
    assert( p->pId2Part[ iObj ] >= 0 );
    if ( p->pId2Part[ iFanin ] == -1 )
    {
        p->pId2Part[ iFanin ] = p->pId2Part[ iObj ];
        Vec_IntPush( p->pvParts + p->pId2Part[ iObj ], iFanin );
        assert( Gia_ObjIsCi(pFanin) || Gia_ObjIsAnd(pFanin) );
        if ( Gia_ObjIsAnd(pFanin) )
            Vec_IntPush( p->vFront, iFanin );
        else if ( Gia_ObjIsRo(p->pGia, pFanin) )
        {
            pFanin = Gia_ObjRoToRi(p->pGia, pFanin);
            iFanin = Gia_ObjId(p->pGia, pFanin);
            assert( p->pId2Part[ iFanin ] == -1 );
            p->pId2Part[ iFanin ] = p->pId2Part[ iObj ];
            Vec_IntPush( p->pvParts + p->pId2Part[ iObj ], iFanin );
            Vec_IntPush( p->vFront, iFanin );
        }
    }
    else if ( p->pId2Part[ iObj ] != p->pId2Part[ iFanin ] )
    {
        Vec_Int_t * vPartObj = p->pvParts + p->pId2Part[ iObj ];
        Vec_Int_t * vPartFan = p->pvParts + p->pId2Part[ iFanin ];
        int iTemp, i;
//        printf( "Moving %d to %d (%d -> %d)\n", iObj, iFanin, Vec_IntSize(vPartObj), Vec_IntSize(vPartFan) );
        // add group of iObj to group of iFanin
        assert( Vec_IntSize(vPartObj) > 0 );
        Vec_IntForEachEntry( vPartObj, iTemp, i )
        {
            Vec_IntPush( vPartFan, iTemp );
            p->pId2Part[ iTemp ] = p->pId2Part[ iFanin ];
        }
        Vec_IntShrink( vPartObj, 0 );
        p->nParts--;
    }
}
void Opa_ManPerform( Gia_Man_t * pGia )
{
    Opa_Man_t * p;
    Gia_Obj_t * pObj;
    int i, Limit, Count = 0;
 
    p = Opa_ManStart( pGia );
    Limit = Vec_IntSize(p->vFront);
//Opa_ManPrint2( p );
    Gia_ManForEachObjVec( p->vFront, pGia, pObj, i )
    {
        if ( i == Limit )
        {
            printf( "%6d : %6d -> %6d\n", ++Count, i, p->nParts );
            Limit = Vec_IntSize(p->vFront);
            if ( Count > 1 )
                Opa_ManPrint2( p );
        }
//        printf( "*** Object %d  ", Gia_ObjId(pGia, pObj) );
        if ( Gia_ObjIsAnd(pObj) )
        {
            Opa_ManMoveOne( p, pObj, Gia_ObjFanin0(pObj) );
            Opa_ManMoveOne( p, pObj, Gia_ObjFanin1(pObj) );
        }
        else if ( Gia_ObjIsCo(pObj) )
            Opa_ManMoveOne( p, pObj, Gia_ObjFanin0(pObj) );
        else assert( 0 );
//        if ( i % 10 == 0 )
//            printf( "%d   ", p->nParts );
        if ( p->nParts == 1 )
            break;
        if ( Count == 5 )
            break;
    }
    printf( "\n" );
    Opa_ManStop( p );
}



/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManConeMark_rec( Gia_Man_t * p, Gia_Obj_t * pObj, Vec_Int_t * vRoots, int nLimit )
{
    if ( Gia_ObjIsTravIdCurrent(p, pObj) )
        return 0;
    Gia_ObjSetTravIdCurrent(p, pObj);
    if ( Gia_ObjIsAnd(pObj) )
    {
        if ( Gia_ManConeMark_rec( p, Gia_ObjFanin0(pObj), vRoots, nLimit ) )
            return 1;
        if ( Gia_ManConeMark_rec( p, Gia_ObjFanin1(pObj), vRoots, nLimit ) )
            return 1;
    }
    else if ( Gia_ObjIsCo(pObj) )
    {
        if ( Gia_ManConeMark_rec( p, Gia_ObjFanin0(pObj), vRoots, nLimit ) )
            return 1;
    }
    else if ( Gia_ObjIsRo(p, pObj) )
        Vec_IntPush( vRoots, Gia_ObjId(p, Gia_ObjRoToRi(p, pObj)) );
    else if ( Gia_ObjIsPi(p, pObj) )
    {}
    else assert( 0 );
    return (int)(Vec_IntSize(vRoots) > nLimit);
}
int Gia_ManConeMark( Gia_Man_t * p, int iOut, int Limit )
{
    Vec_Int_t * vRoots;
    Gia_Obj_t * pObj;
    int i, RetValue;
    // start the outputs
    pObj = Gia_ManPo( p, iOut );
    vRoots = Vec_IntAlloc( 100 );
    Vec_IntPush( vRoots, Gia_ObjId(p, pObj) );
    // mark internal nodes
    Gia_ManIncrementTravId( p );
    Gia_ObjSetTravIdCurrent( p, Gia_ManConst0(p) );
    Gia_ManForEachObjVec( vRoots, p, pObj, i )
        if ( Gia_ManConeMark_rec( p, pObj, vRoots, Limit ) )
            break;
    RetValue = Vec_IntSize( vRoots ) - 1;
    Vec_IntFree( vRoots );
    return RetValue;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManCountFlops( Gia_Man_t * p, Vec_Int_t * vOuts )
{
    int Limit = ABC_INFINITY;
    Vec_Int_t * vRoots;
    Gia_Obj_t * pObj;
    int i, RetValue, iOut;
    // start the outputs
    vRoots = Vec_IntAlloc( 100 );
    Vec_IntForEachEntry( vOuts, iOut, i )
    {
        pObj = Gia_ManPo( p, iOut );
        Vec_IntPush( vRoots, Gia_ObjId(p, pObj) );
    }
    // mark internal nodes
    Gia_ManIncrementTravId( p );
    Gia_ObjSetTravIdCurrent( p, Gia_ManConst0(p) );
    Gia_ManForEachObjVec( vRoots, p, pObj, i )
        if ( Gia_ManConeMark_rec( p, pObj, vRoots, Limit ) )
            break;
    RetValue = Vec_IntSize( vRoots ) - Vec_IntSize(vOuts);
    Vec_IntFree( vRoots );
    return RetValue;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManFindPoPartition3( Gia_Man_t * p, int iOut, int nDelta, int nOutsMin, int nOutsMax, int fVerbose, Vec_Ptr_t ** pvPosEquivs )
{
/*
    int i, Count = 0;
    // mark nodes belonging to output 'iOut'
    for ( i = 0; i < Gia_ManPoNum(p); i++ )
        Count += (Gia_ManConeMark(p, i, 10000) < 10000);
     //   printf( "%d ", Gia_ManConeMark(p, i, 1000) );
    printf( "%d out of %d\n", Count, Gia_ManPoNum(p) );

    // add other outputs as long as they are nDelta away
*/
//    Opa_ManPerform( p );

    return NULL;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Gia_ManFindPivots( Gia_Man_t * p, int SelectShift, int fOnlyCis, int fVerbose )
{
    Vec_Int_t * vPivots, * vWeights;
    Vec_Int_t * vCount, * vResult;
    int i, j, Count, * pPerm, Limit;
/*
    Gia_Obj_t * pObj;
    // count MUX controls
    vCount = Vec_IntStart( Gia_ManObjNum(p) );
    Gia_ManForEachAnd( p, pObj, i )
    {
        Gia_Obj_t * pNodeC, * pNodeT, * pNodeE;
        if ( !Gia_ObjIsMuxType(pObj) ) 
            continue;
        pNodeC = Gia_ObjRecognizeMux( pObj, &pNodeT, &pNodeE );
        Vec_IntAddToEntry( vCount, Gia_ObjId(p, Gia_Regular(pNodeC)), 1 );
    }
*/
    // count references
    Gia_ManCreateRefs( p );
    vCount = Vec_IntAllocArray( p->pRefs, Gia_ManObjNum(p) ); p->pRefs = NULL;

    // collect nodes 
    vPivots  = Vec_IntAlloc( 100 );
    vWeights = Vec_IntAlloc( 100 );
    Vec_IntForEachEntry( vCount, Count, i )
    {
        if ( Count < 2 ) continue;
        if ( fOnlyCis && !Gia_ObjIsCi(Gia_ManObj(p, i)) )
            continue;
        Vec_IntPush( vPivots, i );
        Vec_IntPush( vWeights, Count );
    }
    Vec_IntFree( vCount );

    if ( fVerbose )
        printf( "Selected %d pivots with more than one fanout (out of %d CIs and ANDs).\n", Vec_IntSize(vWeights), Gia_ManCiNum(p) + Gia_ManAndNum(p) );

    // permute
    Gia_ManRandom(1);
    Gia_ManRandom(0);
    for ( i = 0; i < Vec_IntSize(vWeights); i++ )
    {
        j = (Gia_ManRandom(0) >> 1) % Vec_IntSize(vWeights);
        ABC_SWAP( int, vPivots->pArray[i], vPivots->pArray[j] );
        ABC_SWAP( int, vWeights->pArray[i], vWeights->pArray[j] );
    }
    // sort
    if ( SelectShift == 0 )
        pPerm = Abc_QuickSortCost( Vec_IntArray(vWeights), Vec_IntSize(vWeights), 1 );
    else
    {
        Vec_Int_t * vTemp = Vec_IntStartNatural( Vec_IntSize(vWeights) );
        pPerm = Vec_IntReleaseArray( vTemp );
        Vec_IntFree( vTemp );
    }

    // select    
    Limit = Abc_MinInt( 64, Vec_IntSize(vWeights) );
    vResult = Vec_IntAlloc( Limit );
    for ( i = 0; i < Limit; i++ )
    {
        j = (i + SelectShift) % Vec_IntSize(vWeights);
        if ( fVerbose )
            printf( "%2d : Pivot =%7d  Fanout =%7d\n", j, Vec_IntEntry(vPivots, pPerm[j]), Vec_IntEntry(vWeights, pPerm[j]) );
        Vec_IntPush( vResult, Vec_IntEntry(vPivots, pPerm[j]) );
    }

    Vec_IntFree( vPivots );
    Vec_IntFree( vWeights );
    ABC_FREE( pPerm );

    return vResult;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Wrd_t * Gia_ManDeriveSigns( Gia_Man_t * p, Vec_Int_t * vPivots, int fVerbose )
{
    Vec_Wrd_t * vSigns;
    Gia_Obj_t * pObj, * pObjRi;
    int i, fChange = 1, Counter;

    Gia_ManFillValue( p );
    Gia_ManForEachObjVec( vPivots, p, pObj, i )
        pObj->Value = i;

    if ( fVerbose )
        printf( "Signature propagation: " );
    vSigns = Vec_WrdStart( Gia_ManObjNum(p) );
    while ( fChange )
    {
        fChange = 0;
        Gia_ManForEachObj( p, pObj, i )
        {
            if ( ~pObj->Value )
            {
                assert( pObj->Value >= 0 && pObj->Value < 64 );
                *Vec_WrdEntryP( vSigns, i ) |= ( (word)1 << pObj->Value );
            }
            if ( Gia_ObjIsAnd(pObj) )
                *Vec_WrdEntryP( vSigns, i ) |= Vec_WrdEntry(vSigns, Gia_ObjFaninId0(pObj, i)) | Vec_WrdEntry(vSigns, Gia_ObjFaninId1(pObj, i));
            else if ( Gia_ObjIsCo(pObj) )
                *Vec_WrdEntryP( vSigns, i ) |= Vec_WrdEntry(vSigns, Gia_ObjFaninId0(pObj, i));
        }
        Counter = 0;
        Gia_ManForEachRiRo( p, pObjRi, pObj, i )
        {
            word Value = Vec_WrdEntry(vSigns, Gia_ObjId(p, pObj));
            *Vec_WrdEntryP( vSigns, Gia_ObjId(p, pObj) ) |= Vec_WrdEntry(vSigns, Gia_ObjId(p, pObjRi));
            if ( Value != Vec_WrdEntry(vSigns, Gia_ObjId(p, pObj)) )
                fChange = 1, Counter++;
        }
        if ( fVerbose )
            printf( "%d ", Counter );
    }
    if ( fVerbose )
       printf( "\n" );
    return vSigns;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Gia_ManHashOutputs( Gia_Man_t * p, Vec_Wrd_t * vSigns, int fVerbose )
{
    Vec_Ptr_t * vBins;
    Vec_Wec_t * vClasses;
    Vec_Wrd_t * vSignsPo;
    Vec_Int_t * vPriority, * vBin;
    Gia_Obj_t * pObj;
    int i;
    // collect PO signatures
    vSignsPo = Vec_WrdAlloc( Gia_ManPoNum(p) );
    Gia_ManForEachPo( p, pObj, i )
        Vec_WrdPush( vSignsPo, Vec_WrdEntry(vSigns, Gia_ObjId(p, pObj)) );
    // find equivalence classes
    vPriority = Hsh_WrdManHashArray( vSignsPo, 1 );
    Vec_WrdFree( vSignsPo );
    vClasses = Vec_WecCreateClasses( vPriority );
    Vec_IntFree( vPriority );
    vBins = (Vec_Ptr_t *)Vec_WecConvertToVecPtr( vClasses );
    Vec_WecFree( vClasses );
    Vec_VecSort( (Vec_Vec_t *)vBins, 1 );

    if ( fVerbose )
        printf( "Computed %d partitions:\n", Vec_PtrSize(vBins) );
    if ( !fVerbose )
        printf( "Listing partitions with more than 100 outputs:\n" );
    Vec_PtrForEachEntry( Vec_Int_t *, vBins, vBin, i )
    {
        assert( Vec_IntSize(vBin) > 0 );
        if ( fVerbose || Vec_IntSize(vBin) > 100 )
        {
            int PoNum = Vec_IntEntry( vBin, 0 );
            Gia_Obj_t * pObj = Gia_ManPo( p, PoNum );
            word Sign = Vec_WrdEntry( vSigns, Gia_ObjId(p, pObj) );
            // print
            printf( "%3d ", i );
            Extra_PrintBinary( stdout, (unsigned *)&Sign, 64 );
            printf( "  " );
            printf( "PO =%7d  ", Vec_IntSize(vBin) );
            printf( "FF =%7d", Gia_ManCountFlops(p, vBin) );
            printf( "\n" );
        }
    }
    return vBins;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManFindPoPartition2( Gia_Man_t * p, int iStartNum, int nDelta, int nOutsMin, int nOutsMax, int fSetLargest, int fVerbose, Vec_Ptr_t ** pvPosEquivs )
{
    return NULL;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManFindPoPartition( Gia_Man_t * p, int SelectShift, int fOnlyCis, int fSetLargest, int fVerbose, Vec_Ptr_t ** pvPosEquivs )
{
    Gia_Man_t * pGia = NULL;
    Vec_Int_t * vPivots;
    Vec_Wrd_t * vSigns;
    Vec_Ptr_t * vParts;
    Vec_Int_t * vPart;
    abctime clk = Abc_Clock();
    vPivots = Gia_ManFindPivots( p, SelectShift, fOnlyCis, fVerbose );
    vSigns = Gia_ManDeriveSigns( p, vPivots, fVerbose );
    Vec_IntFree( vPivots );
    vParts = Gia_ManHashOutputs( p, vSigns, fVerbose );
    Vec_WrdFree( vSigns );
    if ( fSetLargest )
    {
        vPart = Vec_VecEntryInt( (Vec_Vec_t *)vParts, 0 );
        pGia = Gia_ManDupCones( p, Vec_IntArray(vPart), Vec_IntSize(vPart), 1 );
    }
    if ( pvPosEquivs )
    {
        *pvPosEquivs = vParts;
        printf( "The algorithm divided %d POs into %d partitions.   ", Gia_ManPoNum(p), Vec_PtrSize(vParts) );
        Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
    }
    else
        Vec_VecFree( (Vec_Vec_t *)vParts );
    return pGia;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

