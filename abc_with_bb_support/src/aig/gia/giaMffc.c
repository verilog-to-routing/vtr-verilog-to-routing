/**CFile****************************************************************

  FileName    [gia.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Scalable AIG package.]

  Synopsis    []

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: gia.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "gia.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static inline int   Gia_ObjDom( Gia_Man_t * p, Gia_Obj_t * pObj )            { return Vec_IntEntry(p->vDoms, Gia_ObjId(p, pObj));   }
static inline void  Gia_ObjSetDom( Gia_Man_t * p, Gia_Obj_t * pObj, int d )  { Vec_IntWriteEntry(p->vDoms, Gia_ObjId(p, pObj), d);  }

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
static inline void Gia_ManAddDom( Gia_Man_t * p, Gia_Obj_t * pObj, int iDom0 )
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
static inline void Gia_ManComputeDoms( Gia_Man_t * p )
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

  Synopsis    [Returns the number of internal nodes in the MFFC.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Gia_NodeDeref_rec( Gia_Man_t * p, Gia_Obj_t * pNode )
{
    Gia_Obj_t * pFanin;
    int Counter = 0;
    if ( Gia_ObjIsCi(pNode) )
        return 0;
    assert( Gia_ObjIsAnd(pNode) );
    pFanin = Gia_ObjFanin0(pNode);
    assert( Gia_ObjRefNum(p, pFanin) > 0 );
    if ( Gia_ObjRefDec(p, pFanin) == 0 )
        Counter += Gia_NodeDeref_rec( p, pFanin );
    pFanin = Gia_ObjFanin1(pNode);
    assert( Gia_ObjRefNum(p, pFanin) > 0 );
    if ( Gia_ObjRefDec(p, pFanin) == 0 )
        Counter += Gia_NodeDeref_rec( p, pFanin );
    return Counter + 1;
}
static inline int Gia_NodeRef_rec( Gia_Man_t * p, Gia_Obj_t * pNode )
{
    Gia_Obj_t * pFanin;
    int Counter = 0;
    if ( Gia_ObjIsCi(pNode) )
        return 0;
    assert( Gia_ObjIsAnd(pNode) );
    pFanin = Gia_ObjFanin0(pNode);
    if ( Gia_ObjRefInc(p, pFanin) == 0 )
        Counter += Gia_NodeRef_rec( p, pFanin );
    pFanin = Gia_ObjFanin1(pNode);
    if ( Gia_ObjRefInc(p, pFanin) == 0 )
        Counter += Gia_NodeRef_rec( p, pFanin );
    return Counter + 1;
}
static inline void Gia_NodeCollect_rec( Gia_Man_t * p, Gia_Obj_t * pNode, Vec_Int_t * vSupp, Vec_Int_t * vSuppRefs  )
{
    if ( Gia_ObjIsTravIdCurrent(p, pNode) )
        return;
    Gia_ObjSetTravIdCurrent(p, pNode);
    if ( Gia_ObjRefNum(p, pNode) || Gia_ObjIsCi(pNode) )
    {
        Vec_IntPush( vSupp, Gia_ObjId(p, pNode) );
        Vec_IntPush( vSuppRefs, Gia_ObjRefNum(p, pNode) );
        return;
    }
    assert( Gia_ObjIsAnd(pNode) );
    Gia_NodeCollect_rec( p, Gia_ObjFanin0(pNode), vSupp, vSuppRefs );
    Gia_NodeCollect_rec( p, Gia_ObjFanin1(pNode), vSupp, vSuppRefs );
}
static inline int Gia_NodeMffcSizeSupp( Gia_Man_t * p, Gia_Obj_t * pNode, Vec_Int_t * vSupp, Vec_Int_t * vSuppRefs )
{
    int ConeSize1, ConeSize2, i, iObj;
    assert( !Gia_IsComplement(pNode) );
    assert( Gia_ObjIsAnd(pNode) );
    Vec_IntClear( vSupp );
    Vec_IntClear( vSuppRefs );
    Gia_ManIncrementTravId( p );
    ConeSize1 = Gia_NodeDeref_rec( p, pNode );
    Gia_NodeCollect_rec( p, Gia_ObjFanin0(pNode), vSupp, vSuppRefs );
    Gia_NodeCollect_rec( p, Gia_ObjFanin1(pNode), vSupp, vSuppRefs );
    ConeSize2 = Gia_NodeRef_rec( p, pNode );
    assert( ConeSize1 == ConeSize2 );
    assert( ConeSize1 >= 0 );
    // record supp refs    
    Vec_IntForEachEntry( vSupp, iObj, i )
        Vec_IntAddToEntry( vSuppRefs, i, -Gia_ObjRefNumId(p, iObj) );
    return ConeSize1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManDomDerive_rec( Gia_Man_t * pNew, Gia_Man_t * p, Gia_Obj_t * pNode )
{
    if ( Gia_ObjIsTravIdCurrent(p, pNode) )
        return pNode->Value;
    Gia_ObjSetTravIdCurrent(p, pNode);
    assert( Gia_ObjIsAnd(pNode) );
    Gia_ManDomDerive_rec( pNew, p, Gia_ObjFanin0(pNode) );
    Gia_ManDomDerive_rec( pNew, p, Gia_ObjFanin1(pNode) );
    return pNode->Value = Gia_ManHashAnd( pNew, Gia_ObjFanin0Copy(pNode), Gia_ObjFanin1Copy(pNode) );
}
Gia_Man_t * Gia_ManDomDerive( Gia_Man_t * p, Gia_Obj_t * pRoot, Vec_Int_t * vSupp, int nVars )
{
    Gia_Man_t * pNew, * pTemp;
    int nMints = 1 << nVars;
    int i, m, iResLit;
    assert( nVars >= 0 && nVars <= 5 );
    pNew = Gia_ManStart( Gia_ManObjNum(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    Gia_ManConst0(p)->Value = 0;
    Gia_ManHashAlloc( pNew );
    for ( i = 0; i < Vec_IntSize(vSupp); i++ )
        Gia_ManAppendCi(pNew);
    for ( m = 0; m < nMints; m++ )
    {
        Gia_Obj_t * pObj;
        Gia_ManIncrementTravId( p );
        Gia_ManForEachObjVec( vSupp, p, pObj, i )
        {
            if ( i < nVars )
                pObj->Value = (m >> i) & 1;
            else
                pObj->Value = Gia_ObjToLit(pNew, Gia_ManCi(pNew, i));
            Gia_ObjSetTravIdCurrent( p, pObj );
        }
        iResLit = Gia_ManDomDerive_rec( pNew, p, pRoot );
        Gia_ManAppendCo( pNew, iResLit );
    }
    Gia_ManHashStop( pNew );
    pNew = Gia_ManCleanup( pTemp = pNew );
    Gia_ManStop( pTemp );
    return pNew;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManComputeDomsTry( Gia_Man_t * p )
{
    extern void Gia_ManCollapseTestTest( Gia_Man_t * p );

    Vec_Int_t * vSupp, * vSuppRefs;
    Gia_Man_t * pNew;
    Gia_Obj_t * pObj;
    int i, nSize, Entry, k;
    abctime clk = Abc_Clock();
    ABC_FREE( p->pRefs );
    Gia_ManLevelNum( p );
    Gia_ManCreateRefs( p );
    Gia_ManComputeDoms( p );
    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
    vSupp = Vec_IntAlloc( 1000 );
    vSuppRefs = Vec_IntAlloc( 1000 );
    Gia_ManForEachObj( p, pObj, i )
    {
        if ( !Gia_ObjIsAnd(pObj) && !Gia_ObjIsCo(pObj) )
            continue;
        if ( Gia_ObjDom(p, pObj) != i )
            continue;
        if ( Gia_ObjIsCo(pObj) )
            pObj = Gia_ObjFanin0(pObj);
        if ( !Gia_ObjIsAnd(pObj) )
            continue;
        nSize = Gia_NodeMffcSizeSupp( p, pObj, vSupp, vSuppRefs );
        if ( nSize < 10 )//|| nSize > 100 )
            continue;
        // sort by cost
        Vec_IntSelectSortCost2( Vec_IntArray(vSupp), Vec_IntSize(vSupp), Vec_IntArray(vSuppRefs) );

        printf( "Obj %6d : ", i );
        printf( "Cone = %4d  ", nSize );
        printf( "Supp = %4d  ", Vec_IntSize(vSupp) );
//        Vec_IntForEachEntry( vSuppRefs, Entry, k )
//            printf( "%d(%d) ", -Entry, Gia_ObjLevelId(p, Vec_IntEntry(vSupp, k)) );
        printf( "\n" );

        // selected k
        for ( k = 0; k < Vec_IntSize(vSupp); k++ )
            if ( Vec_IntEntry(vSuppRefs, k) == 1 )
                break;
        k = Abc_MinInt( k, 3 );
        k = 0;

        // dump
        pNew = Gia_ManDomDerive( p, pObj, vSupp, k );
        Gia_DumpAiger( pNew, "mffc", i, 6 );
        Gia_ManCollapseTestTest( pNew );

        Gia_ManStop( pNew );
    }
    Vec_IntFree( vSuppRefs );
    Vec_IntFree( vSupp );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

