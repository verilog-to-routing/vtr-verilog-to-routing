/**CFile****************************************************************

  FileName    [sbdCut2.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [SAT-based optimization using internal don't-cares.]

  Synopsis    [Cut computation.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: sbdCut2.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "sbdInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#define SBD_MAX_CUTSIZE   10
#define SBD_MAX_CUTNUM    501

#define SBD_CUT_NO_LEAF   0xF

typedef struct Sbd_Cut_t_ Sbd_Cut_t; 
struct Sbd_Cut_t_
{
    word            Sign;                      // signature
    int             iFunc;                     // functionality
    int             Cost;                      // cut cost
    int             CostLev;                   // cut cost
    unsigned        nTreeLeaves  :  9;         // tree leaves
    unsigned        nSlowLeaves  :  9;         // slow leaves
    unsigned        nTopLeaves   : 10;         // top leaves
    unsigned        nLeaves      :  4;         // leaf count
    int             pLeaves[SBD_MAX_CUTSIZE];  // leaves
};

struct Sbd_Srv_t_
{
    int             nLutSize;
    int             nCutSize;
    int             nCutNum;
    int             fVerbose;
    Gia_Man_t *     pGia;                      // user's AIG manager (will be modified by adding nodes)
    Vec_Int_t *     vMirrors;                  // mirrors for each node
    Vec_Int_t *     vLutLevs;                  // delays for each node
    Vec_Int_t *     vLevs;                     // levels for each node
    Vec_Int_t *     vRefs;                     // refs for each node
    Sbd_Cut_t       pCuts[SBD_MAX_CUTNUM];     // temporary cuts
    Sbd_Cut_t *     ppCuts[SBD_MAX_CUTNUM];    // temporary cut pointers
    abctime         clkStart;                  // starting time
    Vec_Int_t *     vCut0;                     // current cut
    Vec_Int_t *     vCut;                      // current cut
    Vec_Int_t *     vCutTop;                   // current cut
    Vec_Int_t *     vCutBot;                   // current cut
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
Sbd_Srv_t * Sbd_ManCutServerStart( Gia_Man_t * pGia, Vec_Int_t * vMirrors, 
                                   Vec_Int_t * vLutLevs, Vec_Int_t * vLevs, Vec_Int_t * vRefs, 
                                   int nLutSize, int nCutSize, int nCutNum, int fVerbose )
{
    Sbd_Srv_t * p;
    assert( nLutSize <= nCutSize );
    assert( nCutSize < SBD_CUT_NO_LEAF );
    assert( nCutSize > 1 && nCutSize <= SBD_MAX_CUTSIZE );
    assert( nCutNum > 1 && nCutNum < SBD_MAX_CUTNUM );
    p = ABC_CALLOC( Sbd_Srv_t, 1 );
    p->clkStart = Abc_Clock();
    p->nLutSize = nLutSize;
    p->nCutSize = nCutSize;
    p->nCutNum  = nCutNum;
    p->fVerbose = fVerbose;
    p->pGia     = pGia;
    p->vMirrors = vMirrors;
    p->vLutLevs = vLutLevs;
    p->vLevs    = vLevs;
    p->vRefs    = vRefs;
    p->vCut0    = Vec_IntAlloc( 100 );
    p->vCut     = Vec_IntAlloc( 100 );
    p->vCutTop  = Vec_IntAlloc( 100 );
    p->vCutBot  = Vec_IntAlloc( 100 );
    return p;
}
void Sbd_ManCutServerStop( Sbd_Srv_t * p )
{
    Vec_IntFree( p->vCut0 );
    Vec_IntFree( p->vCut );
    Vec_IntFree( p->vCutTop );
    Vec_IntFree( p->vCutBot );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Sbd_ManCutIsTopo_rec( Gia_Man_t * p, Vec_Int_t * vMirrors, int iObj )
{
    Gia_Obj_t * pObj; 
    int Ret0, Ret1;
    if ( Vec_IntEntry(vMirrors, iObj) >= 0 )
        iObj = Abc_Lit2Var(Vec_IntEntry(vMirrors, iObj));
    if ( !iObj || Gia_ObjIsTravIdCurrentId(p, iObj) )
        return 1;
    Gia_ObjSetTravIdCurrentId(p, iObj);
    pObj = Gia_ManObj( p, iObj );
    if ( Gia_ObjIsCi(pObj) )
        return 0;
    assert( Gia_ObjIsAnd(pObj) );
    Ret0 = Sbd_ManCutIsTopo_rec( p, vMirrors, Gia_ObjFaninId0(pObj, iObj) );
    Ret1 = Sbd_ManCutIsTopo_rec( p, vMirrors, Gia_ObjFaninId1(pObj, iObj) );
    return Ret0 && Ret1;
}
int Sbd_ManCutIsTopo( Gia_Man_t * p, Vec_Int_t * vMirrors, Vec_Int_t * vCut, int iObj )
{
    int i, Entry, RetValue;
    Gia_ManIncrementTravId( p );
    Vec_IntForEachEntry( vCut, Entry, i )
        Gia_ObjSetTravIdCurrentId( p, Entry );        
    RetValue = Sbd_ManCutIsTopo_rec( p, vMirrors, iObj );
    if ( RetValue == 0 )
        printf( "Cut of node %d is not tological\n", iObj );
    assert( RetValue );
    return RetValue;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Sbd_ManCutExpandOne( Gia_Man_t * p, Vec_Int_t * vMirrors, Vec_Int_t * vLutLevs, Vec_Int_t * vCut, int iThis, int iObj )
{
    int Lit0m, Lit1m, Fan0, Fan1, iPlace0, iPlace1;
    int LutLev = Vec_IntEntry( vLutLevs, iObj );
    Gia_Obj_t * pObj = Gia_ManObj(p, iObj);
    if ( Gia_ObjIsCi(pObj) )
        return 0;
    assert( Gia_ObjIsAnd(pObj) );
    Lit0m = Vec_IntEntry( vMirrors, Gia_ObjFaninId0(pObj, iObj) );
    Lit1m = Vec_IntEntry( vMirrors, Gia_ObjFaninId1(pObj, iObj) );
    Fan0 = Lit0m >= 0 ? Abc_Lit2Var(Lit0m) : Gia_ObjFaninId0(pObj, iObj);
    Fan1 = Lit1m >= 0 ? Abc_Lit2Var(Lit1m) : Gia_ObjFaninId1(pObj, iObj);
    iPlace0 = Vec_IntFind( vCut, Fan0 );
    iPlace1 = Vec_IntFind( vCut, Fan1 );
    if ( iPlace0 == -1 && iPlace1 == -1 )
        return 0;
    if ( Vec_IntEntry(vLutLevs, Fan0) > LutLev || Vec_IntEntry(vLutLevs, Fan1) > LutLev )
        return 0;
    Vec_IntDrop( vCut, iThis );
    if ( iPlace0 == -1 && Fan0 )
        Vec_IntPushOrder( vCut, Fan0 );
    if ( iPlace1 == -1 && Fan1 )
        Vec_IntPushOrder( vCut, Fan1 );
    return 1;
}
void Vec_IntOrdered( Vec_Int_t * vCut )
{
    int i, Prev, Entry;
    Prev = Vec_IntEntry( vCut, 0 );
    Vec_IntForEachEntryStart( vCut, Entry, i, 1 )
    {
        assert( Prev < Entry );
        Prev = Entry;
    }
}
void Sbd_ManCutExpand( Gia_Man_t * p, Vec_Int_t * vMirrors, Vec_Int_t * vLutLevs, Vec_Int_t * vCut )
{
    int i, Entry;
    do
    {
        Vec_IntForEachEntry( vCut, Entry, i )
            if ( Sbd_ManCutExpandOne( p, vMirrors, vLutLevs, vCut, i, Entry ) )
                break;
    } 
    while ( i < Vec_IntSize(vCut) );
}
void Sbd_ManCutReload( Vec_Int_t * vMirrors, Vec_Int_t * vLutLevs, int LevStop, Vec_Int_t * vCut, Vec_Int_t * vCutTop, Vec_Int_t * vCutBot )
{
    int i, Entry;
    Vec_IntClear( vCutTop );
    Vec_IntClear( vCutBot );
    Vec_IntForEachEntry( vCut, Entry, i )
    {
        assert( Entry );
        assert( Vec_IntEntry(vMirrors, Entry) == -1 );
        assert( Vec_IntEntry(vLutLevs, Entry) <= LevStop );
        if ( Vec_IntEntry(vLutLevs, Entry) == LevStop )
            Vec_IntPush( vCutTop, Entry );
        else
            Vec_IntPush( vCutBot, Entry );
    }
    Vec_IntOrdered( vCut );
}
int Sbd_ManCutCollect_rec( Gia_Man_t * p, Vec_Int_t * vMirrors, int iObj, int LevStop, Vec_Int_t * vLutLevs, Vec_Int_t * vCut )
{
    Gia_Obj_t * pObj; 
    int Ret0, Ret1;
    if ( Vec_IntEntry(vMirrors, iObj) >= 0 )
        iObj = Abc_Lit2Var(Vec_IntEntry(vMirrors, iObj));
    if ( !iObj || Gia_ObjIsTravIdCurrentId(p, iObj) )
        return 1;
    Gia_ObjSetTravIdCurrentId(p, iObj);
    pObj = Gia_ManObj( p, iObj );
    if ( Gia_ObjIsCi(pObj) || Vec_IntEntry(vLutLevs, iObj) <= LevStop )
    {
        Vec_IntPush( vCut, iObj );
        return Vec_IntEntry(vLutLevs, iObj) <= LevStop;
    }
    assert( Gia_ObjIsAnd(pObj) );
    Ret0 = Sbd_ManCutCollect_rec( p, vMirrors, Gia_ObjFaninId0(pObj, iObj), LevStop, vLutLevs, vCut );
    Ret1 = Sbd_ManCutCollect_rec( p, vMirrors, Gia_ObjFaninId1(pObj, iObj), LevStop, vLutLevs, vCut );
    return Ret0 && Ret1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Sbd_ManCutReduceTop( Gia_Man_t * p, Vec_Int_t * vMirrors, int iObj, Vec_Int_t * vLutLevs, Vec_Int_t * vCut, Vec_Int_t * vCutTop, int nCutSize )
{
    int i, Entry, Lit0m, Lit1m, Fan0, Fan1;
    int LevStop = Vec_IntEntry(vLutLevs, iObj) - 2;
    Vec_IntOrdered( vCut );
    Vec_IntForEachEntryReverse( vCutTop, Entry, i )
    {
        Gia_Obj_t * pObj = Gia_ManObj( p, Entry );
        if ( Gia_ObjIsCi(pObj) )
            continue;
        assert( Gia_ObjIsAnd(pObj) );
        assert( Vec_IntEntry(vLutLevs, Entry) == LevStop );
        Lit0m = Vec_IntEntry( vMirrors, Gia_ObjFaninId0(pObj, Entry) );
        Lit1m = Vec_IntEntry( vMirrors, Gia_ObjFaninId1(pObj, Entry) );
        Fan0 = Lit0m >= 0 ? Abc_Lit2Var(Lit0m) : Gia_ObjFaninId0(pObj, Entry);
        Fan1 = Lit1m >= 0 ? Abc_Lit2Var(Lit1m) : Gia_ObjFaninId1(pObj, Entry);
        if ( Vec_IntEntry(vLutLevs, Fan0) > LevStop || Vec_IntEntry(vLutLevs, Fan1) > LevStop )
            continue;
        assert( Vec_IntEntry(vLutLevs, Fan0) <= LevStop );
        assert( Vec_IntEntry(vLutLevs, Fan1) <= LevStop );
        if ( Vec_IntEntry(vLutLevs, Fan0) == LevStop && Vec_IntEntry(vLutLevs, Fan1) == LevStop )
            continue;
        Vec_IntRemove( vCut, Entry );
        if ( Fan0 ) Vec_IntPushUniqueOrder( vCut, Fan0 );
        if ( Fan1 ) Vec_IntPushUniqueOrder( vCut, Fan1 );
        //Sbd_ManCutIsTopo( p, vMirrors, vCut, iObj );
        return 1;
    }
    return 0;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Sbd_ManCutServerFirst( Sbd_Srv_t * p, int iObj, int * pLeaves )
{
    int RetValue, LevStop = Vec_IntEntry(p->vLutLevs, iObj) - 2;

    Vec_IntClear( p->vCut );
    Gia_ManIncrementTravId( p->pGia );
    RetValue = Sbd_ManCutCollect_rec( p->pGia, p->vMirrors, iObj, LevStop, p->vLutLevs, p->vCut );
    if ( RetValue == 0 ) // cannot build delay-improving cut
        return -1;
    // check if the current cut is good
    Vec_IntSort( p->vCut, 0 );
/*
    Sbd_ManCutReload( p->vMirrors, p->vLutLevs, LevStop, p->vCut, p->vCutTop, p->vCutBot );
    if ( Vec_IntSize(p->vCut) <= p->nCutSize && Vec_IntSize(p->vCutTop) <= p->nLutSize-1 )
    {
        //printf( "%d ", Vec_IntSize(p->vCut) );
        memcpy( pLeaves, Vec_IntArray(p->vCut), sizeof(int) * Vec_IntSize(p->vCut) );
        return Vec_IntSize(p->vCut);
    }
*/
    // try to expand the cut
    Sbd_ManCutExpand( p->pGia, p->vMirrors, p->vLutLevs, p->vCut );
    Sbd_ManCutReload( p->vMirrors, p->vLutLevs, LevStop, p->vCut, p->vCutTop, p->vCutBot );
    if ( Vec_IntSize(p->vCut) <= p->nCutSize && Vec_IntSize(p->vCutTop) <= p->nLutSize-1 )
    {
        //printf( "1=(%d,%d) ", Vec_IntSize(p->vCutTop), Vec_IntSize(p->vCutBot) );
        //printf( "%d ", Vec_IntSize(p->vCut) );
        memcpy( pLeaves, Vec_IntArray(p->vCut), sizeof(int) * Vec_IntSize(p->vCut) );
        return Vec_IntSize(p->vCut);
    }

    // try to reduce the topmost
    Vec_IntClear( p->vCut0 );
    Vec_IntAppend( p->vCut0, p->vCut );
    if ( Vec_IntSize(p->vCut) < p->nCutSize && Sbd_ManCutReduceTop( p->pGia, p->vMirrors, iObj, p->vLutLevs, p->vCut, p->vCutTop, p->nCutSize ) )
    {
        Sbd_ManCutExpand( p->pGia, p->vMirrors, p->vLutLevs, p->vCut );
        Sbd_ManCutReload( p->vMirrors, p->vLutLevs, LevStop, p->vCut, p->vCutTop, p->vCutBot );
        assert( Vec_IntSize(p->vCut) <= p->nCutSize );
        if ( Vec_IntSize(p->vCutTop) <= p->nLutSize-1 )
        {
            //printf( "%d -> %d (%d + %d)\n", Vec_IntSize(p->vCut0), Vec_IntSize(p->vCut), Vec_IntSize(p->vCutTop), Vec_IntSize(p->vCutBot) );
            memcpy( pLeaves, Vec_IntArray(p->vCut), sizeof(int) * Vec_IntSize(p->vCut) );
            return Vec_IntSize(p->vCut);
        }
        // try again
        if ( Vec_IntSize(p->vCut) < p->nCutSize && Sbd_ManCutReduceTop( p->pGia, p->vMirrors, iObj, p->vLutLevs, p->vCut, p->vCutTop, p->nCutSize ) )
        {
            Sbd_ManCutExpand( p->pGia, p->vMirrors, p->vLutLevs, p->vCut );
            Sbd_ManCutReload( p->vMirrors, p->vLutLevs, LevStop, p->vCut, p->vCutTop, p->vCutBot );
            assert( Vec_IntSize(p->vCut) <= p->nCutSize );
            if ( Vec_IntSize(p->vCutTop) <= p->nLutSize-1 )
            {
                //printf( "* %d -> %d (%d + %d)\n", Vec_IntSize(p->vCut0), Vec_IntSize(p->vCut), Vec_IntSize(p->vCutTop), Vec_IntSize(p->vCutBot) );
                memcpy( pLeaves, Vec_IntArray(p->vCut), sizeof(int) * Vec_IntSize(p->vCut) );
                return Vec_IntSize(p->vCut);
            }
            // try again
            if ( Vec_IntSize(p->vCut) < p->nCutSize && Sbd_ManCutReduceTop( p->pGia, p->vMirrors, iObj, p->vLutLevs, p->vCut, p->vCutTop, p->nCutSize ) )
            {
                Sbd_ManCutExpand( p->pGia, p->vMirrors, p->vLutLevs, p->vCut );
                Sbd_ManCutReload( p->vMirrors, p->vLutLevs, LevStop, p->vCut, p->vCutTop, p->vCutBot );
                assert( Vec_IntSize(p->vCut) <= p->nCutSize );
                if ( Vec_IntSize(p->vCutTop) <= p->nLutSize-1 )
                {
                    //printf( "** %d -> %d (%d + %d)\n", Vec_IntSize(p->vCut0), Vec_IntSize(p->vCut), Vec_IntSize(p->vCutTop), Vec_IntSize(p->vCutBot) );
                    memcpy( pLeaves, Vec_IntArray(p->vCut), sizeof(int) * Vec_IntSize(p->vCut) );
                    return Vec_IntSize(p->vCut);
                }
                // try again
                if ( Vec_IntSize(p->vCut) < p->nCutSize && Sbd_ManCutReduceTop( p->pGia, p->vMirrors, iObj, p->vLutLevs, p->vCut, p->vCutTop, p->nCutSize ) )
                {
                    Sbd_ManCutExpand( p->pGia, p->vMirrors, p->vLutLevs, p->vCut );
                    Sbd_ManCutReload( p->vMirrors, p->vLutLevs, LevStop, p->vCut, p->vCutTop, p->vCutBot );
                    assert( Vec_IntSize(p->vCut) <= p->nCutSize );
                    if ( Vec_IntSize(p->vCutTop) <= p->nLutSize-1 )
                    {
                        //printf( "*** %d -> %d (%d + %d)\n", Vec_IntSize(p->vCut0), Vec_IntSize(p->vCut), Vec_IntSize(p->vCutTop), Vec_IntSize(p->vCutBot) );
                        memcpy( pLeaves, Vec_IntArray(p->vCut), sizeof(int) * Vec_IntSize(p->vCut) );
                        return Vec_IntSize(p->vCut);
                    }
                }
            }
        }
    }

    // recompute the cut
    Vec_IntClear( p->vCut );
    Gia_ManIncrementTravId( p->pGia );
    RetValue = Sbd_ManCutCollect_rec( p->pGia, p->vMirrors, iObj, LevStop-1, p->vLutLevs, p->vCut );
    if ( RetValue == 0 ) // cannot build delay-improving cut
        return -1;
    // check if the current cut is good
    Vec_IntSort( p->vCut, 0 );
/*
    Sbd_ManCutReload( p->vMirrors, p->vLutLevs, LevStop, p->vCut, p->vCutTop, p->vCutBot );
    if ( Vec_IntSize(p->vCut) <= p->nCutSize && Vec_IntSize(p->vCutTop) <= p->nLutSize-1 )
    {
        //printf( "%d ", Vec_IntSize(p->vCut) );
        memcpy( pLeaves, Vec_IntArray(p->vCut), sizeof(int) * Vec_IntSize(p->vCut) );
        return Vec_IntSize(p->vCut);
    }
*/
    // try to expand the cut
    Sbd_ManCutExpand( p->pGia, p->vMirrors, p->vLutLevs, p->vCut );
    Sbd_ManCutReload( p->vMirrors, p->vLutLevs, LevStop, p->vCut, p->vCutTop, p->vCutBot );
    if ( Vec_IntSize(p->vCut) <= p->nCutSize && Vec_IntSize(p->vCutTop) <= p->nLutSize-1 )
    {
        //printf( "2=(%d,%d) ", Vec_IntSize(p->vCutTop), Vec_IntSize(p->vCutBot) );
        //printf( "%d ", Vec_IntSize(p->vCut) );
        memcpy( pLeaves, Vec_IntArray(p->vCut), sizeof(int) * Vec_IntSize(p->vCut) );
        return Vec_IntSize(p->vCut);
    }

    return -1;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

