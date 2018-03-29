/**CFile****************************************************************

  FileName    [absRpm.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Abstraction package.]

  Synopsis    [Structural reparameterization.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: absRpm.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/
 
#include "abs.h"
#include "misc/vec/vecWec.h"

ABC_NAMESPACE_IMPL_START 

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static inline int   Gia_ObjDom( Gia_Man_t * p, Gia_Obj_t * pObj )            { return Vec_IntEntry(p->vDoms, Gia_ObjId(p, pObj));   }
static inline void  Gia_ObjSetDom( Gia_Man_t * p, Gia_Obj_t * pObj, int d )  { Vec_IntWriteEntry(p->vDoms, Gia_ObjId(p, pObj), d);  }

static int Abs_ManSupport2( Gia_Man_t * p, Gia_Obj_t * pObj, Vec_Int_t * vSupp );
static int Abs_GiaObjDeref_rec( Gia_Man_t * p, Gia_Obj_t * pNode );
static int Abs_GiaObjRef_rec( Gia_Man_t * p, Gia_Obj_t * pNode );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Computes one-node dominators.]

  Description [For each node, computes the closest one-node dominator,
  which can be the node itself if the node has no other dominators.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManAddDom( Gia_Man_t * p, Gia_Obj_t * pObj, int iDom0 )
{
    int iDom1, iDomNext;
    if ( Gia_ObjDom(p, pObj) == -1 )
    {
        Gia_ObjSetDom( p, pObj, iDom0 );
        return;
    }
    iDom1 = Gia_ObjDom( p, pObj );
    while ( 1 )
    {
        if ( iDom0 > iDom1 )
        {
            iDomNext = Gia_ObjDom( p, Gia_ManObj(p, iDom1) );
            if ( iDomNext == iDom1 )
                break;
            iDom1 = iDomNext;
            continue;
        }
        if ( iDom1 > iDom0 )
        {
            iDomNext = Gia_ObjDom( p, Gia_ManObj(p, iDom0) );
            if ( iDomNext == iDom0 )
                break;
            iDom0 = iDomNext;
            continue;
        }
        assert( iDom0 == iDom1 );
        Gia_ObjSetDom( p, pObj, iDom0 );
        return;
    }
    Gia_ObjSetDom( p, pObj, Gia_ObjId(p, pObj) );
}
void Gia_ManComputeDoms( Gia_Man_t * p )
{
    Gia_Obj_t * pObj;
    int i;
    if ( p->vDoms == NULL )
        p->vDoms = Vec_IntAlloc( 0 );
    Vec_IntFill( p->vDoms, Gia_ManObjNum(p), -1 );
    Gia_ManForEachObjReverse( p, pObj, i )
    {
        if ( i == 0 || Gia_ObjIsCi(pObj) )
            continue;
        if ( pObj->fMark1 || (p->pRefs && Gia_ObjIsAnd(pObj) && Gia_ObjRefNum(p, pObj) == 0) )
            continue;
        if ( Gia_ObjIsCo(pObj) )
        {
            Gia_ObjSetDom( p, pObj, i );
            Gia_ManAddDom( p, Gia_ObjFanin0(pObj), i );
            continue;
        }
        assert( Gia_ObjIsAnd(pObj) );
        Gia_ManAddDom( p, Gia_ObjFanin0(pObj), i );
        Gia_ManAddDom( p, Gia_ObjFanin1(pObj), i );
    }
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Wec_t * Gia_ManCreateSupps( Gia_Man_t * p, int fVerbose )
{
    abctime clk = Abc_Clock();
    Gia_Obj_t * pObj; int i, Id;
    Vec_Wec_t * vSupps = Vec_WecStart( Gia_ManObjNum(p) );
    Gia_ManForEachCiId( p, Id, i )
        Vec_IntPush( Vec_WecEntry(vSupps, Id), i );
    Gia_ManForEachAnd( p, pObj, Id )
        Vec_IntTwoMerge2( Vec_WecEntry(vSupps, Gia_ObjFaninId0(pObj, Id)), 
                          Vec_WecEntry(vSupps, Gia_ObjFaninId1(pObj, Id)), 
                          Vec_WecEntry(vSupps, Id) ); 
//    Gia_ManForEachCo( p, pObj, i )
//        Vec_IntAppend( Vec_WecEntry(vSupps, Gia_ObjId(p, pObj)), Vec_WecEntry(vSupps, Gia_ObjFaninId0p(p, pObj)) );
    if ( fVerbose )
        Abc_PrintTime( 1, "Support computation", Abc_Clock() - clk );
    return vSupps;
}
void Gia_ManDomTest( Gia_Man_t * p )
{
    Vec_Int_t * vDoms = Vec_IntAlloc( 100 );
    Vec_Int_t * vSupp = Vec_IntAlloc( 100 );
    Vec_Wec_t * vSupps = Gia_ManCreateSupps( p, 1 );
    Vec_Wec_t * vDomeds = Vec_WecStart( Gia_ManObjNum(p) );
    Gia_Obj_t * pObj, * pDom; int i, Id, nMffcSize;
    Gia_ManCreateRefs( p );
    Gia_ManComputeDoms( p );
    Gia_ManForEachCi( p, pObj, i )
    {
        if ( Gia_ObjDom(p, pObj) == -1 )
            continue;
        for ( pDom = Gia_ManObj(p, Gia_ObjDom(p, pObj)); Gia_ObjIsAnd(pDom); pDom = Gia_ManObj(p, Gia_ObjDom(p, pDom)) )
            Vec_IntPush( Vec_WecEntry(vDomeds, Gia_ObjId(p, pDom)), i );
    }
    Gia_ManForEachAnd( p, pObj, i )
        if ( Vec_IntEqual(Vec_WecEntry(vSupps, i), Vec_WecEntry(vDomeds, i)) )
            Vec_IntPush( vDoms, i );
    Vec_WecFree( vSupps );
    Vec_WecFree( vDomeds );

    // check MFFC sizes
    Vec_IntForEachEntry( vDoms, Id, i )
        Gia_ObjRefInc( p, Gia_ManObj(p, Id) );
    Vec_IntForEachEntry( vDoms, Id, i )
    {
        nMffcSize = Gia_NodeMffcSizeSupp( p, Gia_ManObj(p, Id), vSupp );
        printf( "%d(%d:%d) ", Id, Vec_IntSize(vSupp), nMffcSize );
    }
    printf( "\n" );
    Vec_IntForEachEntry( vDoms, Id, i )
        Gia_ObjRefDec( p, Gia_ManObj(p, Id) );

//    Vec_IntPrint( vDoms );
    Vec_IntFree( vDoms );
    Vec_IntFree( vSupp );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManTestDoms2( Gia_Man_t * p )
{
    Vec_Int_t * vNodes;
    Gia_Obj_t * pObj, * pDom;
    abctime clk = Abc_Clock();
    int i;
    assert( p->vDoms == NULL );
    Gia_ManComputeDoms( p );
/*
    Gia_ManForEachPi( p, pObj, i )
        if ( Gia_ObjId(p, pObj) != Gia_ObjDom(p, pObj) )
            printf( "PI =%6d  Id =%8d. Dom =%8d.\n", i, Gia_ObjId(p, pObj), Gia_ObjDom(p, pObj) );
*/
    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
    // for each dominated PI, when if the PIs is in a leaf of the MFFC of the dominator
    Gia_ManCleanMark1( p );
    Gia_ManForEachPi( p, pObj, i )
        pObj->fMark1 = 1;
    vNodes = Vec_IntAlloc( 100 );
    Gia_ManCreateRefs( p );
    Gia_ManForEachPi( p, pObj, i )
    {
        if ( Gia_ObjId(p, pObj) == Gia_ObjDom(p, pObj) )
            continue;

        pDom = Gia_ManObj(p, Gia_ObjDom(p, pObj));
        if ( Gia_ObjIsCo(pDom) )
        {
            assert( Gia_ObjFanin0(pDom) == pObj );
            continue;
        }
        assert( Gia_ObjIsAnd(pDom) );
        Abs_GiaObjDeref_rec( p, pDom );
        Abs_ManSupport2( p, pDom, vNodes );
        Abs_GiaObjRef_rec( p, pDom );

        if ( Vec_IntFind(vNodes, Gia_ObjId(p, pObj)) == -1 )
            printf( "FAILURE.\n" );
//        else
//            printf( "Success.\n" );
    }
    Vec_IntFree( vNodes );
    Gia_ManCleanMark1( p );
}

/**Function*************************************************************

  Synopsis    [Collect PI doms.]

  Description [Assumes that some PIs and ANDs are marked with fMark1.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Gia_ManCollectDoms( Gia_Man_t * p )
{
    Vec_Int_t * vNodes;
    Gia_Obj_t * pObj;
    int Lev, LevMax = ABC_INFINITY;
    int i, iDom, iDomNext;
    vNodes = Vec_IntAlloc( 100 );
    Gia_ManForEachObj( p, pObj, i )
    {
        if ( !pObj->fMark1 )
            continue;
        if ( p->pRefs && Gia_ObjRefNum(p, pObj) == 0 )
            continue;
        iDom = Gia_ObjDom(p, pObj);
        if ( iDom == -1 )
            continue;
        if ( iDom == i )
            continue;
        for ( Lev = 0; Lev < LevMax && Gia_ObjIsAnd( Gia_ManObj(p, iDom) ); Lev++ )
        {
            Vec_IntPush( vNodes, iDom );
            iDomNext = Gia_ObjDom( p, Gia_ManObj(p, iDom) );
            if ( iDomNext == iDom )
                break;
            iDom = iDomNext;
        }
    }
    Vec_IntUniqify( vNodes );
    return vNodes;
}
Vec_Int_t * Gia_ManComputePiDoms( Gia_Man_t * p )
{
    Vec_Int_t * vNodes;
    Gia_ManComputeDoms( p );
    vNodes = Gia_ManCollectDoms( p );
//    Vec_IntPrint( vNodes );
    return vNodes;
}
void Gia_ManTestDoms( Gia_Man_t * p )
{
    Vec_Int_t * vNodes;
    Gia_Obj_t * pObj;
    int i;
    // mark PIs
//    Gia_ManCreateRefs( p );
    Gia_ManCleanMark1( p );
    Gia_ManForEachPi( p, pObj, i )
        pObj->fMark1 = 1;
    // compute dominators
    assert( p->vDoms == NULL );
    vNodes = Gia_ManComputePiDoms( p );
//    printf( "Nodes = %d. Doms = %d.\n", Gia_ManAndNum(p), Vec_IntSize(vNodes) );
    Vec_IntFree( vNodes );
    // unmark PIs
    Gia_ManCleanMark1( p );
}


/**Function*************************************************************

  Synopsis    [Counts flops without fanout.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManCountFanoutlessFlops( Gia_Man_t * p )
{
    Gia_Obj_t * pObj;
    int i;
    int Counter = 0;
    Gia_ManCreateRefs( p );
    Gia_ManForEachRo( p, pObj, i )
        if ( Gia_ObjRefNum(p, pObj) == 0 )
            Counter++;
    printf( "Fanoutless flops = %d.\n", Counter );
    ABC_FREE( p->pRefs );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManCountPisNodes_rec( Gia_Man_t * p, Gia_Obj_t * pObj, Vec_Int_t * vPis, Vec_Int_t * vAnds )
{
    if ( Gia_ObjIsTravIdCurrent(p, pObj) )
        return;
    Gia_ObjSetTravIdCurrent(p, pObj);
    if ( pObj->fMark1 )
    {
        Vec_IntPush( vPis, Gia_ObjId(p, pObj) );
        return;
    }
    assert( Gia_ObjIsAnd(pObj) );
    Gia_ManCountPisNodes_rec( p, Gia_ObjFanin0(pObj), vPis, vAnds );
    Gia_ManCountPisNodes_rec( p, Gia_ObjFanin1(pObj), vPis, vAnds );
    Vec_IntPush( vAnds, Gia_ObjId(p, pObj) );
}
void Gia_ManCountPisNodes( Gia_Man_t * p, Vec_Int_t * vPis, Vec_Int_t * vAnds )
{
    Gia_Obj_t * pObj;
    int i;
    // mark const0 and flop output
    Gia_ManIncrementTravId( p );
    Gia_ObjSetTravIdCurrent( p, Gia_ManConst0(p) );
    Gia_ManForEachRo( p, pObj, i )
        Gia_ObjSetTravIdCurrent( p, pObj );
    // count PIs and internal nodes reachable from COs
    Vec_IntClear( vPis );
    Vec_IntClear( vAnds );
    Gia_ManForEachCo( p, pObj, i )
        Gia_ManCountPisNodes_rec( p, Gia_ObjFanin0(pObj), vPis, vAnds );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abs_GiaObjDeref_rec( Gia_Man_t * p, Gia_Obj_t * pNode )
{
    Gia_Obj_t * pFanin;
    int Counter = 0;
    if ( pNode->fMark1 || Gia_ObjIsRo(p, pNode) )
        return 0;
    assert( Gia_ObjIsAnd(pNode) );
    pFanin = Gia_ObjFanin0(pNode);
    assert( Gia_ObjRefNum(p, pFanin) > 0 );
    if ( Gia_ObjRefDec(p, pFanin) == 0 )
        Counter += Abs_GiaObjDeref_rec( p, pFanin );
    pFanin = Gia_ObjFanin1(pNode);
    assert( Gia_ObjRefNum(p, pFanin) > 0 );
    if ( Gia_ObjRefDec(p, pFanin) == 0 )
        Counter += Abs_GiaObjDeref_rec( p, pFanin );
    return Counter + 1;
}
int Abs_GiaObjRef_rec( Gia_Man_t * p, Gia_Obj_t * pNode )
{
    Gia_Obj_t * pFanin;
    int Counter = 0;
    if ( pNode->fMark1 || Gia_ObjIsRo(p, pNode) )
        return 0;
    assert( Gia_ObjIsAnd(pNode) );
    pFanin = Gia_ObjFanin0(pNode);
    if ( Gia_ObjRefInc(p, pFanin) == 0 )
        Counter += Abs_GiaObjRef_rec( p, pFanin );
    pFanin = Gia_ObjFanin1(pNode);
    if ( Gia_ObjRefInc(p, pFanin) == 0 )
        Counter += Abs_GiaObjRef_rec( p, pFanin );
    return Counter + 1;
}

/**Function*************************************************************

  Synopsis    [Returns the number of nodes with zero refs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abs_GiaSortNodes( Gia_Man_t * p, Vec_Int_t * vSupp )
{
    Gia_Obj_t * pObj;
    int nSize = Vec_IntSize(vSupp);
    int i, RetValue;
    Gia_ManForEachObjVec( vSupp, p, pObj, i )
        if ( i < nSize && Gia_ObjRefNum(p, pObj) == 0 && !Gia_ObjIsRo(p, pObj) ) // add removable leaves
        {
            assert( pObj->fMark1 );
            Vec_IntPush( vSupp, Gia_ObjId(p, pObj) );
        }
    RetValue = Vec_IntSize(vSupp) - nSize;
    Gia_ManForEachObjVec( vSupp, p, pObj, i )
        if ( i < nSize && !(Gia_ObjRefNum(p, pObj) == 0 && !Gia_ObjIsRo(p, pObj)) ) // add non-removable leaves
            Vec_IntPush( vSupp, Gia_ObjId(p, pObj) );
    assert( Vec_IntSize(vSupp) == 2 * nSize );
    memmove( Vec_IntArray(vSupp), Vec_IntArray(vSupp) + nSize, sizeof(int) * nSize );
    Vec_IntShrink( vSupp, nSize );
    return RetValue;
}


/**Function*************************************************************

  Synopsis    [Computes support in terms of PIs and flops.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abs_ManSupport1_rec( Gia_Man_t * p, Gia_Obj_t * pObj, Vec_Int_t * vSupp )
{
    if ( Gia_ObjIsTravIdCurrent(p, pObj) )
        return;
    Gia_ObjSetTravIdCurrent(p, pObj);
    if ( pObj->fMark1 || Gia_ObjIsRo(p, pObj) )
    {
        Vec_IntPush( vSupp, Gia_ObjId(p, pObj) );
        return;
    }
    assert( Gia_ObjIsAnd(pObj) );
    Abs_ManSupport1_rec( p, Gia_ObjFanin0(pObj), vSupp );
    Abs_ManSupport1_rec( p, Gia_ObjFanin1(pObj), vSupp );
}
int Abs_ManSupport1( Gia_Man_t * p, Gia_Obj_t * pObj, Vec_Int_t * vSupp )
{
    assert( Gia_ObjIsAnd(pObj) );
    Vec_IntClear( vSupp );
    Gia_ManIncrementTravId( p );
    Abs_ManSupport1_rec( p, pObj, vSupp );
    return Vec_IntSize(vSupp);
}

/**Function*************************************************************

  Synopsis    [Computes support of the MFFC.]

  Description [Should be called when pObj's cone is dereferenced.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abs_ManSupport2_rec( Gia_Man_t * p, Gia_Obj_t * pObj, Vec_Int_t * vSupp )
{
    if ( Gia_ObjIsTravIdCurrent(p, pObj) )
        return;
    Gia_ObjSetTravIdCurrent(p, pObj);
    if ( pObj->fMark1 || Gia_ObjIsRo(p, pObj) || Gia_ObjRefNum(p, pObj) > 0 )
    {
        Vec_IntPush( vSupp, Gia_ObjId(p, pObj) );
        return;
    }
    assert( Gia_ObjIsAnd(pObj) );
    Abs_ManSupport2_rec( p, Gia_ObjFanin0(pObj), vSupp );
    Abs_ManSupport2_rec( p, Gia_ObjFanin1(pObj), vSupp );
}
int Abs_ManSupport2( Gia_Man_t * p, Gia_Obj_t * pObj, Vec_Int_t * vSupp )
{
    assert( Gia_ObjIsAnd(pObj) );
    Vec_IntClear( vSupp );
    Gia_ManIncrementTravId( p );
    Abs_ManSupport2_rec( p, Gia_ObjFanin0(pObj), vSupp );
    Abs_ManSupport2_rec( p, Gia_ObjFanin1(pObj), vSupp );
    Gia_ObjSetTravIdCurrent(p, pObj);
    return Vec_IntSize(vSupp);
}

/**Function*************************************************************

  Synopsis    [Computes support of the extended MFFC.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abs_ManSupport3( Gia_Man_t * p, Gia_Obj_t * pObj, Vec_Int_t * vSupp )
{
    Gia_Obj_t * pTemp, * pFan0, * pFan1;
    int i, nSize0;
    // collect MFFC
    Abs_ManSupport2( p, pObj, vSupp );
    // move dominated to the front
    nSize0 = Abs_GiaSortNodes( p, vSupp );
    assert( nSize0 > 0 );
    // consider remaining nodes
    while ( 1 )
    {
        int fChanges = 0;
        Gia_ManForEachObjVec( vSupp, p, pTemp, i )
        {
            if ( i < nSize0 )
                continue;
            if ( !Gia_ObjIsAnd(pTemp) )
                continue;
            assert( !pTemp->fMark1 );
            assert( Gia_ObjRefNum(p, pTemp) > 0 );
            pFan0 = Gia_ObjFanin0(pTemp);
            pFan1 = Gia_ObjFanin1(pTemp);
            if ( Gia_ObjIsTravIdCurrent(p, pFan0) && Gia_ObjIsTravIdCurrent(p, pFan1) )
            {
                Vec_IntRemove( vSupp, Gia_ObjId(p, pTemp) );
                fChanges = 1;
                break;
            }
            if ( Gia_ObjIsTravIdCurrent(p, pFan0) )
            {
                Vec_IntRemove( vSupp, Gia_ObjId(p, pTemp) );
                Vec_IntPush( vSupp, Gia_ObjId(p, pFan1) );
                assert( !Gia_ObjIsTravIdCurrent(p, pFan1) );
                Gia_ObjSetTravIdCurrent(p, pFan1);
                fChanges = 1;
                break;
            }
            if ( Gia_ObjIsTravIdCurrent(p, pFan1) )
            {
                Vec_IntRemove( vSupp, Gia_ObjId(p, pTemp) );
                Vec_IntPush( vSupp, Gia_ObjId(p, pFan0) );
                assert( !Gia_ObjIsTravIdCurrent(p, pFan0) );
                Gia_ObjSetTravIdCurrent(p, pFan0);
                fChanges = 1;
                break;
            }
        }
        if ( !fChanges )
            break;
    }
    return Vec_IntSize(vSupp);
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abs_GiaCofPrint( word * pTruth, int nSize, int nSize0, int Res )
{
    int i, Bit;
    int nBits = (1 << nSize);
    int nStep = (1 << nSize0);
    int Mark[2] = {1,1};
    for ( i = 0; i < nBits; i++ )
    {
        if ( i % nStep == 0 )
        {
            printf( " " );
            assert( Res || (Mark[0] && Mark[1]) );
            Mark[0] = Mark[1] = 0;
        }
        Bit = Abc_InfoHasBit((unsigned *)pTruth, i);
        Mark[Bit] = 1;
        printf( "%d", Bit );
    }
    printf( "\n" );
    assert( Res || (Mark[0] && Mark[1]) );
    return 1;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if truth table has no const cofactors.]

  Description [The cofactoring variables are the (nSize-nSize0)
  most significant vars.  Each cofactor depends on nSize0 vars.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abs_GiaCheckTruth( word * pTruth, int nSize, int nSize0 )
{
    unsigned char * pStr = (unsigned char *)pTruth;
    int nStr = (nSize >= 3 ? (1 << (nSize - 3)) : 1);
    int i, k, nSteps;
    assert( nSize0 > 0 && nSize0 <= nSize );
    if ( nSize0 == 1 )
    {
        for ( i = 0; i < nStr; i++ )
            if ( (((unsigned)pStr[i] ^ ((unsigned)pStr[i] >> 1)) & 0x55) != 0x55 )
                return 0;
        return 1;
    }
    if ( nSize0 == 2 )
    {
        for ( i = 0; i < nStr; i++ )
            if ( ((unsigned)pStr[i] & 0xF) == 0x0 || (((unsigned)pStr[i] >> 4) & 0xF) == 0x0 || 
                 ((unsigned)pStr[i] & 0xF) == 0xF || (((unsigned)pStr[i] >> 4) & 0xF) == 0xF  )
                return 0;
        return 1;
    }
    assert( nSize0 >= 3 );
    nSteps = (1 << (nSize0 - 3));
    for ( i = 0; i < nStr; i += nSteps )
    {
        for ( k = 0; k < nSteps; k++ )
            if ( ((unsigned)pStr[i+k] & 0xFF) != 0x00 )
                break;
        if ( k == nSteps )
            break;
        for ( k = 0; k < nSteps; k++ )
            if ( ((unsigned)pStr[i+k] & 0xFF) != 0xFF )
                break;
        if ( k == nSteps )
            break;
    }
    assert( i <= nStr );
    return (int)( i == nStr );
}

/**Function*************************************************************

  Synopsis    [Returns 1 if truth table has const cofactors.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abs_RpmPerformMark( Gia_Man_t * p, int nCutMax, int fVerbose, int fVeryVerbose )
{
    Vec_Int_t * vPis, * vAnds, * vDoms;
    Vec_Int_t * vSupp, * vSupp1, * vSupp2;
    Gia_Obj_t * pObj;
    word * pTruth;
    int Iter, i, nSize0, nNodes;
    int fHasConst, fChanges = 1;
    Gia_ManCreateRefs( p );
    Gia_ManCleanMark1( p );
    Gia_ManForEachPi( p, pObj, i )
        pObj->fMark1 = 1;
    vPis = Vec_IntAlloc( 100 );
    vAnds = Vec_IntAlloc( 100 );
    vSupp1 = Vec_IntAlloc( 100 );
    vSupp2 = Vec_IntAlloc( 100 );
    for ( Iter = 0; fChanges; Iter++ )
    {
        fChanges = 0;
        vDoms = Gia_ManComputePiDoms( p );
        // count the number of PIs and internal nodes
        if ( fVerbose || fVeryVerbose )
        {
            Gia_ManCountPisNodes( p, vPis, vAnds );
            printf( "Iter %3d :  ", Iter );
            printf( "PI = %5d  (%6.2f %%)  ",  Vec_IntSize(vPis),  100.0 * Vec_IntSize(vPis)  / Gia_ManPiNum(p)  );
            printf( "And = %6d  (%6.2f %%) ",  Vec_IntSize(vAnds), 100.0 * Vec_IntSize(vAnds) / Gia_ManAndNum(p) );
            printf( "Dom = %5d  (%6.2f %%)  ", Vec_IntSize(vDoms), 100.0 * Vec_IntSize(vDoms) / Gia_ManAndNum(p)  );
            printf( "\n" );
        }
//        pObj = Gia_ObjFanin0( Gia_ManPo(p, 1) );
        Gia_ManForEachObjVec( vDoms, p, pObj, i )
        {
            assert( !pObj->fMark1 );
            assert( Gia_ObjRefNum( p, pObj ) > 0 );
            // dereference root node
            nNodes = Abs_GiaObjDeref_rec( p, pObj );
/*
            // compute support of full cone
            if ( Abs_ManSupport1(p, pObj, vSupp1) > nCutMax )
//            if ( 1 )
            {
                // check support of MFFC
                if ( Abs_ManSupport2(p, pObj, vSupp2) > nCutMax )
//                if ( 1 )
                {
                    Abs_GiaObjRef_rec( p, pObj );
                    continue;
                }
                vSupp = vSupp2;
//                printf( "-" );
            }
            else
            {
                vSupp = vSupp1;
//                printf( "+" );
            }
*/
            if ( Abs_ManSupport2(p, pObj, vSupp2) > nCutMax )
            {
                Abs_GiaObjRef_rec( p, pObj );
                continue;
            }
            vSupp = vSupp2;

            // order nodes by their ref counts
            nSize0 = Abs_GiaSortNodes( p, vSupp );
            assert( nSize0 > 0 && nSize0 <= nCutMax );
            // check if truth table has const cofs
            pTruth = Gia_ObjComputeTruthTableCut( p, pObj, vSupp );
            if ( pTruth == NULL )
            {
                Abs_GiaObjRef_rec( p, pObj );
                continue;
            }
            fHasConst = !Abs_GiaCheckTruth( pTruth, Vec_IntSize(vSupp), nSize0 );
            if ( fVeryVerbose )
            {
                printf( "Nodes =%3d ",  nNodes );
                printf( "Size =%3d ",   Vec_IntSize(vSupp) );
                printf( "Size0 =%3d  ", nSize0 );
                printf( "%3s", fHasConst ? "yes" : "no" );
                Abs_GiaCofPrint( pTruth, Vec_IntSize(vSupp), nSize0, fHasConst );
            }
            if ( fHasConst )
            {
                Abs_GiaObjRef_rec( p, pObj );
                continue;
            }
            // pObj can be reparamed
            pObj->fMark1 = 1;
            fChanges = 1;
        }
        Vec_IntFree( vDoms );
    }
    // count the number of PIs and internal nodes
    if ( fVeryVerbose )
    {
        Gia_ManCountPisNodes( p, vPis, vAnds );
        printf( "Iter %3d :  ", Iter );
        printf( "PI = %5d  (%6.2f %%)  ",  Vec_IntSize(vPis),  100.0 * Vec_IntSize(vPis)  / Gia_ManPiNum(p)  );
        printf( "And = %6d  (%6.2f %%) ",  Vec_IntSize(vAnds), 100.0 * Vec_IntSize(vAnds) / Gia_ManAndNum(p) );
//        printf( "Dom = %5d  (%6.2f %%)  ", Vec_IntSize(vDoms), 100.0 * Vec_IntSize(vDoms) / Gia_ManAndNum(p)  );
        printf( "\n" );
    }
    // cleanup
    Vec_IntFree( vPis );
    Vec_IntFree( vAnds );
    Vec_IntFree( vSupp1 );
    Vec_IntFree( vSupp2 );
//    Gia_ManCleanMark1( p ); // this will erase markings
    ABC_FREE( p->pRefs );
}

/**Function*************************************************************

  Synopsis    [Assumed that fMark1 marks the internal PIs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManDupRpm( Gia_Man_t * p )
{
    Vec_Int_t * vPis, * vAnds;
    Gia_Man_t * pNew;
    Gia_Obj_t * pObj;
    int i;
    // derive PIs and internal nodes
    vPis = Vec_IntAlloc( 100 );
    vAnds = Vec_IntAlloc( 100 );
    Gia_ManCountPisNodes( p, vPis, vAnds );

    // duplicate AIG
    Gia_ManFillValue( p );
    pNew = Gia_ManStart( Gia_ManObjNum(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    Gia_ManConst0(p)->Value = 0;
    // create PIs
    Gia_ManForEachObjVec( vPis, p, pObj, i )
        pObj->Value = Gia_ManAppendCi(pNew);
    // create flops
    Gia_ManForEachRo( p, pObj, i )
        pObj->Value = Gia_ManAppendCi( pNew );
    // create internal nodes
    Gia_ManForEachObjVec( vAnds, p, pObj, i )
        pObj->Value = Gia_ManAppendAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
    // create COs
    Gia_ManForEachCo( p, pObj, i )
        Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
    Gia_ManSetRegNum( pNew, Gia_ManRegNum(p) );

    // cleanup
    Vec_IntFree( vPis );
    Vec_IntFree( vAnds );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Performs structural reparametrization.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Abs_RpmPerform( Gia_Man_t * p, int nCutMax, int fVerbose, int fVeryVerbose )
{
    Gia_Man_t * pNew;
//    Gia_ManTestDoms( p );
//    return NULL;
    // perform structural analysis
    Gia_ObjComputeTruthTableStart( p, nCutMax );
    Abs_RpmPerformMark( p, nCutMax, fVerbose, fVeryVerbose );
    Gia_ObjComputeTruthTableStop( p );
    // derive new AIG
    pNew = Gia_ManDupRpm( p );
    Gia_ManCleanMark1( p );
    return pNew;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

