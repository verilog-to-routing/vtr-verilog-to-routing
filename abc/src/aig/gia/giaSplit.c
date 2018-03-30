/**CFile****************************************************************

  FileName    [giaSplit.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Scalable AIG package.]

  Synopsis    [Structural AIG splitting.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: giaSplit.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "gia.h"
#include "misc/extra/extra.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Spl_Man_t_ Spl_Man_t;
struct Spl_Man_t_
{
    // input data
    Gia_Man_t *      pGia;       // user AIG with nodes marked
    int              iObj;       // object ID
    int              Limit;      // limit on AIG nodes
    int              fReverse;   // enable reverse search
    // intermediate
    Vec_Bit_t *      vMarksCIO;  // CI/CO marks
    Vec_Bit_t *      vMarksIn;   // input marks
    Vec_Bit_t *      vMarksNo;   // node marks
    Vec_Bit_t *      vMarksAnd;  // AND node marks
    Vec_Int_t *      vRoots;     // nodes pointing to Nodes
    Vec_Int_t *      vNodes;     // nodes used in the window
    Vec_Int_t *      vLeaves;    // nodes pointed by Nodes
    Vec_Int_t *      vAnds;      // AND nodes used in the window
    // temporary 
    Vec_Int_t *      vFanouts;   // fanouts of the node
    Vec_Int_t *      vCands;     // candidate nodes
    Vec_Int_t *      vInputs;    // non-trivial inputs
};

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Transforming to the internal LUT representation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Wec_t * Spl_ManToWecMapping( Gia_Man_t * p )
{
    Vec_Wec_t * vMapping = Vec_WecStart( Gia_ManObjNum(p) );
    int Obj, Fanin, k;
    assert( Gia_ManHasMapping(p) );
    Gia_ManForEachLut( p, Obj )
        Gia_LutForEachFanin( p, Obj, Fanin, k )
            Vec_WecPush( vMapping, Obj, Fanin );
    return vMapping;
}
Vec_Int_t * Spl_ManFromWecMapping( Gia_Man_t * p, Vec_Wec_t * vMap )
{
    Vec_Int_t * vMapping, * vVec; int i, k, Obj;
    assert( Gia_ManHasMapping2(p) );
    vMapping = Vec_IntAlloc( Gia_ManObjNum(p) + Vec_WecSizeSize(vMap) + 2*Vec_WecSizeUsed(vMap) );
    Vec_IntFill( vMapping, Gia_ManObjNum(p), 0 );
    Vec_WecForEachLevel( vMap, vVec, i )
        if ( Vec_IntSize(vVec) > 0 )
        {
            Vec_IntWriteEntry( vMapping, i, Vec_IntSize(vMapping) );
            Vec_IntPush( vMapping, Vec_IntSize(vVec) );
            Vec_IntForEachEntry( vVec, Obj, k )
                Vec_IntPush( vMapping, Obj );
            Vec_IntPush( vMapping, i );
        }
    assert( Vec_IntSize(vMapping) < 16 || Vec_IntSize(vMapping) == Vec_IntCap(vMapping) );
    return vMapping;
}


/**Function*************************************************************

  Synopsis    [Creating manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Spl_Man_t * Spl_ManAlloc( Gia_Man_t * pGia, int Limit, int fReverse )
{
    int i, iObj;
    Spl_Man_t * p = ABC_CALLOC( Spl_Man_t, 1 );
    p->pGia       = pGia;
    p->Limit      = Limit;
    p->fReverse   = fReverse;
    // intermediate
    p->vMarksCIO  = Vec_BitStart( Gia_ManObjNum(pGia) );
    p->vMarksIn   = Vec_BitStart( Gia_ManObjNum(pGia) );
    p->vMarksNo   = Vec_BitStart( Gia_ManObjNum(pGia) );
    p->vMarksAnd  = Vec_BitStart( Gia_ManObjNum(pGia) );
    p->vRoots     = Vec_IntAlloc( 100 );
    p->vNodes     = Vec_IntAlloc( 100 );
    p->vLeaves    = Vec_IntAlloc( 100 );
    p->vAnds      = Vec_IntAlloc( 100 );
    // temporary 
    p->vFanouts   = Vec_IntAlloc( 100 );
    p->vCands     = Vec_IntAlloc( 100 );
    p->vInputs    = Vec_IntAlloc( 100 );
    // mark CIs/COs
    Vec_BitWriteEntry( p->vMarksCIO, 0, 1 );
    Gia_ManForEachCiId( pGia, iObj, i )
        Vec_BitWriteEntry( p->vMarksCIO, iObj, 1 );
    Gia_ManForEachCoId( pGia, iObj, i )
        Vec_BitWriteEntry( p->vMarksCIO, iObj, 1 );
    // mapping
    ABC_FREE( pGia->pRefs );
    Gia_ManCreateRefs( pGia );
    Gia_ManSetLutRefs( pGia );
    assert( Gia_ManHasMapping(pGia) );
    assert( !Gia_ManHasMapping2(pGia) );
    pGia->vMapping2 = Spl_ManToWecMapping( pGia );
    Vec_IntFreeP( &pGia->vMapping );
    // fanout
    Gia_ManStaticFanoutStart( pGia );
    return p;
}
void Spl_ManStop( Spl_Man_t * p )
{
    // fanout
    Gia_ManStaticFanoutStop( p->pGia );
    // mapping
    assert( !Gia_ManHasMapping(p->pGia) );
    assert( Gia_ManHasMapping2(p->pGia) );
    p->pGia->vMapping = Spl_ManFromWecMapping( p->pGia, p->pGia->vMapping2 );
    Vec_WecFreeP( &p->pGia->vMapping2 );
    // intermediate
    Vec_BitFree( p->vMarksCIO );
    Vec_BitFree( p->vMarksIn );
    Vec_BitFree( p->vMarksNo );
    Vec_BitFree( p->vMarksAnd );
    Vec_IntFree( p->vRoots );
    Vec_IntFree( p->vNodes );
    Vec_IntFree( p->vLeaves );
    Vec_IntFree( p->vAnds );
    // temporary 
    Vec_IntFree( p->vFanouts );
    Vec_IntFree( p->vCands );
    Vec_IntFree( p->vInputs );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    [Takes Nodes and Marks. Returns Leaves and Roots.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Spl_ManWinFindLeavesRoots( Spl_Man_t * p )
{
    Vec_Int_t * vVec;
    int iObj, iFan, i, k;
    // collect leaves
/*
    Vec_IntClear( p->vLeaves );
    Gia_ManForEachLut2Vec( p->vNodes, p->pGia, vVec, iObj, i )
    {
        assert( Vec_BitEntry(p->vMarksNo, iObj) );
        Vec_IntForEachEntry( vVec, iFan, k )
            if ( !Vec_BitEntry(p->vMarksNo, iFan) )
            {
                Vec_BitWriteEntry(p->vMarksNo, iFan, 1);
                Vec_IntPush( p->vLeaves, iFan );
            }
    }
    Vec_IntForEachEntry( p->vLeaves, iFan, i )
        Vec_BitWriteEntry(p->vMarksNo, iFan, 0);
*/
    Vec_IntClear( p->vLeaves );
    Vec_IntForEachEntry( p->vAnds, iObj, i )
    {
        Gia_Obj_t * pObj = Gia_ManObj( p->pGia, iObj );
        assert( Vec_BitEntry(p->vMarksAnd, iObj) );
        iFan = Gia_ObjFaninId0( pObj, iObj );
        if ( !Vec_BitEntry(p->vMarksAnd, iFan) )
        {
            assert( Gia_ObjIsLut2(p->pGia, iFan) || Vec_BitEntry(p->vMarksCIO, iFan) );
            Vec_BitWriteEntry(p->vMarksAnd, iFan, 1);
            Vec_IntPush( p->vLeaves, iFan );
        }
        iFan = Gia_ObjFaninId1( pObj, iObj );
        if ( !Vec_BitEntry(p->vMarksAnd, iFan) )
        {
            assert( Gia_ObjIsLut2(p->pGia, iFan) || Vec_BitEntry(p->vMarksCIO, iFan) );
            Vec_BitWriteEntry(p->vMarksAnd, iFan, 1);
            Vec_IntPush( p->vLeaves, iFan );
        }
    }
    Vec_IntForEachEntry( p->vLeaves, iFan, i )
        Vec_BitWriteEntry(p->vMarksAnd, iFan, 0);

    // collect roots
    Vec_IntClear( p->vRoots );
    Gia_ManForEachLut2Vec( p->vNodes, p->pGia, vVec, iObj, i )
        Vec_IntForEachEntry( vVec, iFan, k )
            Gia_ObjLutRefDecId( p->pGia, iFan );
    Vec_IntForEachEntry( p->vAnds, iObj, i )
        if ( Gia_ObjLutRefNumId(p->pGia, iObj) )
            Vec_IntPush( p->vRoots, iObj );
    Gia_ManForEachLut2Vec( p->vNodes, p->pGia, vVec, iObj, i )
        Vec_IntForEachEntry( vVec, iFan, k )
            Gia_ObjLutRefIncId( p->pGia, iFan );
}




/**Function*************************************************************

  Synopsis    [Computes LUTs that are fanouts of the node outside of the cone.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Spl_ManLutFanouts_rec( Gia_Man_t * p, int iObj, Vec_Int_t * vFanouts, Vec_Bit_t * vMarksNo, Vec_Bit_t * vMarksCIO )
{
    int iFanout, i;
    if ( Vec_BitEntry(vMarksNo, iObj) || Vec_BitEntry(vMarksCIO, iObj) )
        return;
    if ( Gia_ObjIsLut2(p, iObj) )
    {
        Vec_BitWriteEntry( vMarksCIO, iObj, 1 );
        Vec_IntPush( vFanouts, iObj );
        return;
    }
    Gia_ObjForEachFanoutStaticId( p, iObj, iFanout, i )
        Spl_ManLutFanouts_rec( p, iFanout, vFanouts, vMarksNo, vMarksCIO );
}
int Spl_ManLutFanouts( Gia_Man_t * p, int iObj, Vec_Int_t * vFanouts, Vec_Bit_t * vMarksNo, Vec_Bit_t * vMarksCIO )
{
    int i, iFanout;
    assert( Gia_ObjIsLut2(p, iObj) );
    Vec_IntClear( vFanouts );
    Gia_ObjForEachFanoutStaticId( p, iObj, iFanout, i )
        Spl_ManLutFanouts_rec( p, iFanout, vFanouts, vMarksNo, vMarksCIO );
    // clean up
    Vec_IntForEachEntry( vFanouts, iFanout, i )
        Vec_BitWriteEntry( vMarksCIO, iFanout, 0 );
    return Vec_IntSize(vFanouts);
}


/**Function*************************************************************

  Synopsis    [Returns the number of fanins beloning to the set.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Spl_ManCountMarkedFanins( Gia_Man_t * p, int iObj, Vec_Bit_t * vMarks )
{
    int i, iFan, Count = 0;
    Vec_Int_t * vFanins = Gia_ObjLutFanins2(p, iObj);
    Vec_IntForEachEntry( vFanins, iFan, i )
        if ( Vec_BitEntry(vMarks, iFan) )
            Count++;
    return Count;
}
int Spl_ManFindGoodCand( Spl_Man_t * p )
{
    int i, iObj;
    int Res = 0, InCount, InCountMax = -1; 
    // mark leaves
    Vec_IntForEachEntry( p->vInputs, iObj, i )
        Vec_BitWriteEntry( p->vMarksIn, iObj, 1 );
    // find candidate with maximum input overlap
    Vec_IntForEachEntry( p->vCands, iObj, i )
    {
        InCount = Spl_ManCountMarkedFanins( p->pGia, iObj, p->vMarksIn );
        if ( InCountMax < InCount )
        {
            InCountMax = InCount;
            Res = iObj;
        }
    }
    // unmark leaves
    Vec_IntForEachEntry( p->vInputs, iObj, i )
        Vec_BitWriteEntry( p->vMarksIn, iObj, 0 );
    return Res;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Spl_ManFindOne( Spl_Man_t * p )
{
    Vec_Int_t * vVec;
    int nFanouts, iObj, iFan, i, k;
    int Res = 0; 

    // deref
    Gia_ManForEachLut2Vec( p->vNodes, p->pGia, vVec, iObj, i )
        Vec_IntForEachEntry( vVec, iFan, k )
            Gia_ObjLutRefDecId( p->pGia, iFan );

    // collect external nodes
    if ( p->fReverse && (Vec_IntSize(p->vNodes) & 1) )
    {
        Vec_IntForEachEntry( p->vNodes, iObj, i )
        {
            if ( Gia_ObjLutRefNumId(p->pGia, iObj) == 0 )
                continue;
            assert( Gia_ObjLutRefNumId(p->pGia, iObj) > 0 );
            if ( Gia_ObjLutRefNumId(p->pGia, iObj) >= 5 )  // skip nodes with high fanout!
                continue;
            nFanouts = Spl_ManLutFanouts( p->pGia, iObj, p->vFanouts, p->vMarksNo, p->vMarksCIO );
            if ( Gia_ObjLutRefNumId(p->pGia, iObj) == 1 && nFanouts == 1 )
            {
                Res = Vec_IntEntry(p->vFanouts, 0);
                goto finish;
            }
            //Vec_IntAppend( p->vCands, p->vFanouts );
        }
    }

    // consider LUT inputs - get one that has no refs
    Vec_IntClear( p->vCands );
    Vec_IntClear( p->vInputs );
    Gia_ManForEachLut2Vec( p->vNodes, p->pGia, vVec, iObj, i )
        Vec_IntForEachEntry( vVec, iFan, k )
            if ( !Vec_BitEntry(p->vMarksNo, iFan) && !Vec_BitEntry(p->vMarksCIO, iFan) && !Gia_ObjLutRefNumId(p->pGia, iFan) )
            {
                Vec_IntPush( p->vCands, iFan );
                Vec_IntPush( p->vInputs, iFan );
            }
    Res = Spl_ManFindGoodCand( p );
    if ( Res )
        goto finish;

    // collect candidates
    Vec_IntClear( p->vCands );
    Vec_IntClear( p->vInputs );
    Gia_ManForEachLut2Vec( p->vNodes, p->pGia, vVec, iObj, i )
        Vec_IntForEachEntry( vVec, iFan, k )
            if ( !Vec_BitEntry(p->vMarksNo, iFan) && !Vec_BitEntry(p->vMarksCIO, iFan) )
            {
                Vec_IntPush( p->vCands, iFan );
                Vec_IntPush( p->vInputs, iFan );
            }

    // all inputs have refs - collect external nodes
    Vec_IntForEachEntry( p->vNodes, iObj, i )
    {
        if ( Gia_ObjLutRefNumId(p->pGia, iObj) == 0 )
            continue;
        assert( Gia_ObjLutRefNumId(p->pGia, iObj) > 0 );
        if ( Gia_ObjLutRefNumId(p->pGia, iObj) >= 5 )  // skip nodes with high fanout!
            continue;
        nFanouts = Spl_ManLutFanouts( p->pGia, iObj, p->vFanouts, p->vMarksNo, p->vMarksCIO );
        if ( Gia_ObjLutRefNumId(p->pGia, iObj) == 1 && nFanouts == 1 )
        {
            Res = Vec_IntEntry(p->vFanouts, 0);
            goto finish;
        }
        Vec_IntAppend( p->vCands, p->vFanouts );
    }

    // choose among not-so-good ones
    Res = Spl_ManFindGoodCand( p );
    if ( Res )
        goto finish;

    // get the first candidate
    if ( Res == 0 && Vec_IntSize(p->vCands) > 0 )
        Res = Vec_IntEntry( p->vCands, 0 );

finish:
    // ref
    Gia_ManForEachLut2Vec( p->vNodes, p->pGia, vVec, iObj, i )
        Vec_IntForEachEntry( vVec, iFan, k )
            Gia_ObjLutRefIncId( p->pGia, iFan );
    return Res;
}



/**Function*************************************************************

  Synopsis    [Computing window for one pivot node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Spl_ManLutMffcSize( Gia_Man_t * p, int iObj, Vec_Int_t * vTemp, Vec_Bit_t * vMarksAnd )
{
    int iTemp, i, k = 0;
    assert( Gia_ObjIsLut2(p, iObj) );
//Vec_IntPrint( Gia_ObjLutFanins2(p, iObj) );
    Gia_ManIncrementTravId( p );
    Gia_ManCollectAnds( p, &iObj, 1, vTemp, Gia_ObjLutFanins2(p, iObj) );
    Vec_IntForEachEntry( vTemp, iTemp, i )
        if ( !Vec_BitEntry(vMarksAnd, iTemp) )
            Vec_IntWriteEntry( vTemp, k++, iTemp );
    Vec_IntShrink( vTemp, k );
    return k;
}
void Spl_ManAddNode( Spl_Man_t * p, int iPivot, Vec_Int_t * vCands )
{
    int i, iObj;
    Vec_IntPush( p->vNodes, iPivot );
    Vec_BitWriteEntry( p->vMarksNo, iPivot, 1 );
    Vec_IntAppend( p->vAnds, vCands );
    Vec_IntForEachEntry( vCands, iObj, i )
        Vec_BitWriteEntry( p->vMarksAnd, iObj, 1 );
}
int Spl_ManComputeOne( Spl_Man_t * p, int iPivot )
{
    int CountAdd, iObj, i;
    assert( Gia_ObjIsLut2(p->pGia, iPivot) );
//Gia_ManPrintCone2( p->pGia, Gia_ManObj(p->pGia, iPivot) );
    // assume it was previously filled in
    Vec_IntForEachEntry( p->vNodes, iObj, i )
        Vec_BitWriteEntry( p->vMarksNo, iObj, 0 );
    Vec_IntForEachEntry( p->vAnds, iObj, i )
        Vec_BitWriteEntry( p->vMarksAnd, iObj, 0 );
    // double check that it is empty
    //Gia_ManForEachLut2( p->pGia, iObj )
    //    assert( !Vec_BitEntry(p->vMarksNo, iObj) );
    //Gia_ManForEachLut2( p->pGia, iObj )
    //    assert( !Vec_BitEntry(p->vMarksAnd, iObj) );
    // clean arrays
    Vec_IntClear( p->vNodes );
    Vec_IntClear( p->vAnds );
    // add root node
    Spl_ManLutMffcSize( p->pGia, iPivot, p->vCands, p->vMarksAnd );
    Spl_ManAddNode( p, iPivot, p->vCands );
    if ( Vec_IntSize(p->vAnds) > p->Limit )
        return 0;
    //printf( "%d ", iPivot );
    // add one node at a time
    while ( (iObj = Spl_ManFindOne(p)) )
    {
        assert( Gia_ObjIsLut2(p->pGia, iObj) );
        assert( !Vec_BitEntry(p->vMarksNo, iObj) );
        CountAdd = Spl_ManLutMffcSize( p->pGia, iObj, p->vCands, p->vMarksAnd );
        if ( Vec_IntSize(p->vAnds) + CountAdd > p->Limit )
            break;
        Spl_ManAddNode( p, iObj, p->vCands );
        //printf( "+%d ", iObj );
    }
    //printf( "\n" );
    Vec_IntSort( p->vNodes, 0 );
    Vec_IntSort( p->vAnds, 0 );
    Spl_ManWinFindLeavesRoots( p );
    Vec_IntSort( p->vLeaves, 0 );
    Vec_IntSort( p->vRoots, 0 );
    return 1;
}

/**Function*************************************************************

  Synopsis    [External procedures.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManComputeOneWin( Gia_Man_t * pGia, int iPivot, Vec_Int_t ** pvRoots, Vec_Int_t ** pvNodes, Vec_Int_t ** pvLeaves, Vec_Int_t ** pvAnds )
{
    Spl_Man_t * p = (Spl_Man_t *)pGia->pSatlutWinman;
    assert( p != NULL );
    if ( iPivot == -1 )
    {
        Spl_ManStop( p );
        pGia->pSatlutWinman = NULL;
        return 0;
    }
    if ( !Spl_ManComputeOne( p, iPivot ) )
    {
        *pvRoots  = NULL;
        *pvNodes  = NULL;
        *pvLeaves = NULL;
        *pvAnds   = NULL;
        return 0;
    }
    *pvRoots  = p->vRoots;
    *pvNodes  = p->vNodes;
    *pvLeaves = p->vLeaves;
    *pvAnds   = p->vAnds;
    // Vec_IntPrint( p->vNodes );
    return Vec_IntSize(p->vAnds);
}
void Gia_ManComputeOneWinStart( Gia_Man_t * pGia, int nAnds, int fReverse )
{
    assert( pGia->pSatlutWinman == NULL );
    pGia->pSatlutWinman = Spl_ManAlloc( pGia, nAnds, fReverse );
}

/**Function*************************************************************

  Synopsis    [Testing procedure.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Spl_ManComputeOneTest( Gia_Man_t * pGia )
{
    int iLut, Count;
    Gia_ManComputeOneWinStart( pGia, 64, 0 );
    Gia_ManForEachLut2( pGia, iLut )
    {
        Vec_Int_t * vRoots, * vNodes, * vLeaves, * vAnds;
        Count = Gia_ManComputeOneWin( pGia, iLut, &vRoots, &vNodes, &vLeaves, &vAnds );
        printf( "Obj = %6d : Leaf = %2d.  Node = %2d.  Root = %2d.    AND = %3d.\n", 
            iLut, Vec_IntSize(vLeaves), Vec_IntSize(vNodes), Vec_IntSize(vRoots), Count ); 
    }
    Gia_ManComputeOneWin( pGia, -1, NULL, NULL, NULL, NULL );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

