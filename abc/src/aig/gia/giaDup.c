/**CFile****************************************************************

  FileName    [giaDup.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Scalable AIG package.]

  Synopsis    [Duplication procedures.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: giaDup.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "gia.h"
#include "misc/tim/tim.h"
#include "misc/vec/vecWec.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Removes pointers to the unmarked nodes..]

  Description [Array vLits contains literals of p. At the same time, 
  each object pObj of p points to a literal of pNew. This procedure
  remaps literals in array vLits into literals of pNew.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManDupRemapLiterals( Vec_Int_t * vLits, Gia_Man_t * p )
{
    Gia_Obj_t * pObj;
    int i, iLit, iLitNew;
    Vec_IntForEachEntry( vLits, iLit, i )
    {
        if ( iLit < 0 )
            continue;
        pObj = Gia_ManObj( p, Abc_Lit2Var(iLit) );
        if ( ~pObj->Value == 0 )
            iLitNew = -1;
        else
            iLitNew = Abc_LitNotCond( pObj->Value, Abc_LitIsCompl(iLit) );
        Vec_IntWriteEntry( vLits, i, iLitNew );
    }
}

/**Function*************************************************************

  Synopsis    [Removes pointers to the unmarked nodes..]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManDupRemapEquiv( Gia_Man_t * pNew, Gia_Man_t * p )
{
    Vec_Int_t * vClass;
    int i, k, iNode, iRepr, iPrev;
    if ( p->pReprs == NULL )
        return;
    assert( pNew->pReprs == NULL && pNew->pNexts == NULL );
    // start representatives
    pNew->pReprs = ABC_CALLOC( Gia_Rpr_t, Gia_ManObjNum(pNew) );
    for ( i = 0; i < Gia_ManObjNum(pNew); i++ )
        Gia_ObjSetRepr( pNew, i, GIA_VOID );
    // iterate over constant candidates
    Gia_ManForEachConst( p, i )
        Gia_ObjSetRepr( pNew, Abc_Lit2Var(Gia_ManObj(p, i)->Value), 0 );
    // iterate over class candidates
    vClass = Vec_IntAlloc( 100 );
    Gia_ManForEachClass( p, i )
    {
        Vec_IntClear( vClass );
        Gia_ClassForEachObj( p, i, k )
            Vec_IntPushUnique( vClass, Abc_Lit2Var(Gia_ManObj(p, k)->Value) );
        assert( Vec_IntSize( vClass ) > 1 );
        Vec_IntSort( vClass, 0 );
        iRepr = iPrev = Vec_IntEntry( vClass, 0 );
        Vec_IntForEachEntryStart( vClass, iNode, k, 1 )
        {
            Gia_ObjSetRepr( pNew, iNode, iRepr );
            assert( iPrev < iNode );
            iPrev = iNode;
        }
    }
    Vec_IntFree( vClass );
    pNew->pNexts = Gia_ManDeriveNexts( pNew );
}

/**Function*************************************************************

  Synopsis    [Remaps combinational inputs when objects are DFS ordered.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManDupRemapCis( Gia_Man_t * pNew, Gia_Man_t * p )
{
    Gia_Obj_t * pObj, * pObjNew;
    int i;
    assert( Vec_IntSize(p->vCis) == Vec_IntSize(pNew->vCis) );
    Gia_ManForEachCi( p, pObj, i )
    {
        assert( Gia_ObjCioId(pObj) == i );
        pObjNew = Gia_ObjFromLit( pNew, pObj->Value );
        assert( !Gia_IsComplement(pObjNew) );
        Vec_IntWriteEntry( pNew->vCis, i, Gia_ObjId(pNew, pObjNew) );
        Gia_ObjSetCioId( pObjNew, i );
    }
}

/**Function*************************************************************

  Synopsis    [Remaps combinational outputs when objects are DFS ordered.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManDupRemapCos( Gia_Man_t * pNew, Gia_Man_t * p )
{
    Gia_Obj_t * pObj, * pObjNew;
    int i;
    assert( Vec_IntSize(p->vCos) == Vec_IntSize(pNew->vCos) );
    Gia_ManForEachCo( p, pObj, i )
    {
        assert( Gia_ObjCioId(pObj) == i );
        pObjNew = Gia_ObjFromLit( pNew, pObj->Value );
        assert( !Gia_IsComplement(pObjNew) );
        Vec_IntWriteEntry( pNew->vCos, i, Gia_ObjId(pNew, pObjNew) );
        Gia_ObjSetCioId( pObjNew, i );
    }
}

/**Function*************************************************************

  Synopsis    [Duplicates the AIG in the DFS order.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManDupOrderDfs_rec( Gia_Man_t * pNew, Gia_Man_t * p, Gia_Obj_t * pObj )
{
    if ( ~pObj->Value )
        return pObj->Value;
    if ( Gia_ObjIsCi(pObj) )
        return pObj->Value = Gia_ManAppendCi(pNew);
//    if ( p->pNexts && Gia_ObjNext(p, Gia_ObjId(p, pObj)) )
//        Gia_ManDupOrderDfs_rec( pNew, p, Gia_ObjNextObj(p, Gia_ObjId(p, pObj)) );
    Gia_ManDupOrderDfs_rec( pNew, p, Gia_ObjFanin0(pObj) );
    if ( Gia_ObjIsCo(pObj) )
        return pObj->Value = Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
    Gia_ManDupOrderDfs_rec( pNew, p, Gia_ObjFanin1(pObj) );
    return pObj->Value = Gia_ManAppendAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
} 

/**Function*************************************************************

  Synopsis    [Duplicates AIG while putting objects in the DFS order.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManDupOrderDfs( Gia_Man_t * p )
{
    Gia_Man_t * pNew;
    Gia_Obj_t * pObj;
    int i;
    Gia_ManFillValue( p );
    pNew = Gia_ManStart( Gia_ManObjNum(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    Gia_ManConst0(p)->Value = 0;
    Gia_ManForEachCo( p, pObj, i )
        Gia_ManDupOrderDfs_rec( pNew, p, pObj );
    Gia_ManForEachCi( p, pObj, i )
        if ( !~pObj->Value )
            pObj->Value = Gia_ManAppendCi(pNew);
    assert( Gia_ManCiNum(pNew) == Gia_ManCiNum(p) );
    Gia_ManDupRemapCis( pNew, p );
    Gia_ManDupRemapEquiv( pNew, p );
    Gia_ManSetRegNum( pNew, Gia_ManRegNum(p) );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Duplicates AIG while putting objects in the DFS order.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManDupAbs( Gia_Man_t * p, Vec_Int_t * vMapPpi2Ff, Vec_Int_t * vMapFf2Ppi )
{
    Gia_Man_t * pNew;
    Gia_Obj_t * pObj;
    int k, Flop, Used;
    assert( Vec_IntSize(vMapFf2Ppi) == Vec_IntSize(vMapPpi2Ff) + Vec_IntCountEntry(vMapFf2Ppi, -1) );
    Gia_ManFillValue( p );
    pNew = Gia_ManStart( Gia_ManObjNum(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    Gia_ManConst0(p)->Value = 0;
    // create inputs
    Gia_ManForEachPi( p, pObj, k )
        pObj->Value = Gia_ManAppendCi(pNew);
    Vec_IntForEachEntry( vMapPpi2Ff, Flop, k )
    {
        pObj = Gia_ManCi( p, Gia_ManPiNum(p) + Flop );
        pObj->Value = Gia_ManAppendCi(pNew);
    }
    Vec_IntForEachEntry( vMapFf2Ppi, Used, Flop )
    {
        pObj = Gia_ManCi( p, Gia_ManPiNum(p) + Flop );
        if ( Used >= 0 )
        {
            assert( pObj->Value != ~0 );
            continue;
        }
        assert( pObj->Value == ~0 );
        pObj->Value = Gia_ManAppendCi(pNew);
    }
    Gia_ManForEachCi( p, pObj, k )
        assert( pObj->Value != ~0 );
    // create nodes
    Gia_ManForEachPo( p, pObj, k )
        Gia_ManDupOrderDfs_rec( pNew, p, Gia_ObjFanin0(pObj) );
    Vec_IntForEachEntry( vMapFf2Ppi, Used, Flop )
    {
        if ( Used >= 0 )
            continue;
        pObj = Gia_ManCi( p, Gia_ManPiNum(p) + Flop );
        pObj = Gia_ObjRoToRi( p, pObj );
        Gia_ManDupOrderDfs_rec( pNew, p, Gia_ObjFanin0(pObj) );
    }
    // create outputs
    Gia_ManForEachPo( p, pObj, k )
        pObj->Value = Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
    Vec_IntForEachEntry( vMapFf2Ppi, Used, Flop )
    {
        if ( Used >= 0 )
            continue;
        pObj = Gia_ManCi( p, Gia_ManPiNum(p) + Flop );
        pObj = Gia_ObjRoToRi( p, pObj );
        pObj->Value = Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
    }
    Gia_ManSetRegNum( pNew, Gia_ManRegNum(p) - Vec_IntSize(vMapPpi2Ff) );
    assert( Gia_ManPiNum(pNew) == Gia_ManPiNum(p) + Vec_IntSize(vMapPpi2Ff) );
    assert( Gia_ManCiNum(pNew) == Gia_ManCiNum(p) );
    assert( Gia_ManPoNum(pNew) == Gia_ManPoNum(p) );
    assert( Gia_ManCoNum(pNew) == Gia_ManCoNum(p) - Vec_IntSize(vMapPpi2Ff) );
    return pNew;
}


/**Function*************************************************************

  Synopsis    [Duplicates AIG while putting objects in the DFS order.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManDupOutputGroup( Gia_Man_t * p, int iOutStart, int iOutStop )
{
    Gia_Man_t * pNew;
    Gia_Obj_t * pObj;
    int i;
    Gia_ManFillValue( p );
    pNew = Gia_ManStart( Gia_ManObjNum(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    Gia_ManConst0(p)->Value = 0;
    for ( i = iOutStart; i < iOutStop; i++ )
    {
        pObj = Gia_ManCo( p, i );
        Gia_ManDupOrderDfs_rec( pNew, p, pObj );
    }
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Duplicates AIG while putting objects in the DFS order.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManDupOutputVec( Gia_Man_t * p, Vec_Int_t * vOutPres )
{
    Gia_Man_t * pNew;
    Gia_Obj_t * pObj;
    int i;
    assert( Gia_ManRegNum(p) == 0 );
    assert( Gia_ManPoNum(p) == Vec_IntSize(vOutPres) );
    Gia_ManFillValue( p );
    pNew = Gia_ManStart( Gia_ManObjNum(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    Gia_ManConst0(p)->Value = 0;
    Gia_ManForEachPi( p, pObj, i )
        pObj->Value = Gia_ManAppendCi(pNew);
    Gia_ManForEachPo( p, pObj, i )
        if ( Vec_IntEntry(vOutPres, i) )
            Gia_ManDupOrderDfs_rec( pNew, p, Gia_ObjFanin0(pObj) );
    Gia_ManForEachPo( p, pObj, i )
        if ( Vec_IntEntry(vOutPres, i) )
            pObj->Value = Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Duplicates AIG while putting objects in the DFS order.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManDupSelectedOutputs( Gia_Man_t * p, Vec_Int_t * vOutsLeft )
{
    Gia_Man_t * pNew;
    Gia_Obj_t * pObj;
    int i, iOut;
    assert( Gia_ManRegNum(p) == 0 );
    assert( Gia_ManPoNum(p) >= Vec_IntSize(vOutsLeft) );
    Gia_ManFillValue( p );
    pNew = Gia_ManStart( Gia_ManObjNum(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    Gia_ManConst0(p)->Value = 0;
    Gia_ManForEachPi( p, pObj, i )
        pObj->Value = Gia_ManAppendCi(pNew);
    Vec_IntForEachEntry( vOutsLeft, iOut, i )
        Gia_ManDupOrderDfs_rec( pNew, p, Gia_ObjFanin0(Gia_ManPo(p, iOut)) );
    Vec_IntForEachEntry( vOutsLeft, iOut, i )
        Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(Gia_ManPo(p, iOut)) );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Duplicates the AIG in the DFS order.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManDupOrderDfsChoices_rec( Gia_Man_t * pNew, Gia_Man_t * p, Gia_Obj_t * pObj )
{
    Gia_Obj_t * pNext;
    if ( ~pObj->Value )
        return;
    assert( Gia_ObjIsAnd(pObj) );
    pNext = Gia_ObjNextObj( p, Gia_ObjId(p, pObj) );
    if ( pNext )
        Gia_ManDupOrderDfsChoices_rec( pNew, p, pNext );
    Gia_ManDupOrderDfsChoices_rec( pNew, p, Gia_ObjFanin0(pObj) );
    Gia_ManDupOrderDfsChoices_rec( pNew, p, Gia_ObjFanin1(pObj) );
    pObj->Value = Gia_ManAppendAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
    if ( pNext )
    {
        pNew->pNexts[Abc_Lit2Var(pObj->Value)] = Abc_Lit2Var( Abc_Lit2Var(pNext->Value) );
        assert( Abc_Lit2Var(pObj->Value) > Abc_Lit2Var(pNext->Value) );
    }
} 

/**Function*************************************************************

  Synopsis    [Duplicates AIG while putting objects in the DFS order.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManDupOrderDfsChoices( Gia_Man_t * p )
{
    Gia_Man_t * pNew;
    Gia_Obj_t * pObj;
    int i;
    assert( p->pReprs && p->pNexts );
    Gia_ManFillValue( p );
    pNew = Gia_ManStart( Gia_ManObjNum(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    pNew->pNexts = ABC_CALLOC( int, Gia_ManObjNum(p) );
    Gia_ManConst0(p)->Value = 0;
    Gia_ManForEachCi( p, pObj, i )
        pObj->Value = Gia_ManAppendCi(pNew);
    Gia_ManForEachCo( p, pObj, i )
    {
        Gia_ManDupOrderDfsChoices_rec( pNew, p, Gia_ObjFanin0(pObj) );
        pObj->Value = Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
    }
    Gia_ManSetRegNum( pNew, Gia_ManRegNum(p) );
//    Gia_ManDeriveReprs( pNew );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Duplicates AIG while putting objects in the DFS order.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManDupOrderDfsReverse( Gia_Man_t * p )
{
    Gia_Man_t * pNew;
    Gia_Obj_t * pObj;
    int i;
    Gia_ManFillValue( p );
    pNew = Gia_ManStart( Gia_ManObjNum(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    Gia_ManConst0(p)->Value = 0;
    Gia_ManForEachCoReverse( p, pObj, i )
        Gia_ManDupOrderDfs_rec( pNew, p, pObj );
    Gia_ManForEachCi( p, pObj, i )
        if ( !~pObj->Value )
            pObj->Value = Gia_ManAppendCi(pNew);
    assert( Gia_ManCiNum(pNew) == Gia_ManCiNum(p) );
    Gia_ManDupRemapCis( pNew, p );
    Gia_ManDupRemapCos( pNew, p );
    Gia_ManDupRemapEquiv( pNew, p );
    Gia_ManSetRegNum( pNew, Gia_ManRegNum(p) );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Duplicates AIG while putting first PIs, then nodes, then POs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManDupOrderAiger( Gia_Man_t * p )
{
    Gia_Man_t * pNew;
    Gia_Obj_t * pObj;
    int i;
    pNew = Gia_ManStart( Gia_ManObjNum(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    Gia_ManConst0(p)->Value = 0;
    Gia_ManForEachCi( p, pObj, i )
        pObj->Value = Gia_ManAppendCi(pNew);
    Gia_ManForEachAnd( p, pObj, i )
        pObj->Value = Gia_ManAppendAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
    Gia_ManForEachCo( p, pObj, i )
        pObj->Value = Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
    Gia_ManDupRemapEquiv( pNew, p );
    Gia_ManSetRegNum( pNew, Gia_ManRegNum(p) );
    assert( Gia_ManIsNormalized(pNew) );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Duplicates AIG while putting first PIs, then nodes, then POs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManDupOnsetOffset( Gia_Man_t * p )
{
    Gia_Man_t * pNew;
    Gia_Obj_t * pObj;
    int i;
    pNew = Gia_ManStart( Gia_ManObjNum(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    Gia_ManConst0(p)->Value = 0;
    Gia_ManForEachCi( p, pObj, i )
        pObj->Value = Gia_ManAppendCi(pNew);
    Gia_ManForEachAnd( p, pObj, i )
        pObj->Value = Gia_ManAppendAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
    Gia_ManForEachCo( p, pObj, i )
    {
        pObj->Value = Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
        pObj->Value = Gia_ManAppendCo( pNew, Abc_LitNot(Gia_ObjFanin0Copy(pObj)) );
    }
    Gia_ManSetRegNum( pNew, Gia_ManRegNum(p) );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Duplicates AIG while putting first PIs, then nodes, then POs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManDupLastPis( Gia_Man_t * p, int nLastPis )
{
    Gia_Man_t * pNew;
    Gia_Obj_t * pObj;
    int i;
    assert( Gia_ManRegNum(p) == 0 );
    pNew = Gia_ManStart( Gia_ManObjNum(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    Gia_ManConst0(p)->Value = 0;
    Gia_ManForEachCi( p, pObj, i )
        pObj->Value = (i < Gia_ManCiNum(p) - nLastPis) ? ~0 : Gia_ManAppendCi(pNew);
    Gia_ManForEachAnd( p, pObj, i )
        pObj->Value = Gia_ManAppendAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
    Gia_ManForEachCo( p, pObj, i )
        pObj->Value = Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Duplicates AIG while complementing the flops.]

  Description [The array of initial state contains the init state
  for each state bit of the flops in the design.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManDupFlip( Gia_Man_t * p, int * pInitState )
{
    Gia_Man_t * pNew;
    Gia_Obj_t * pObj;
    int i;
    pNew = Gia_ManStart( Gia_ManObjNum(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    Gia_ManConst0(p)->Value = 0;
    Gia_ManForEachObj1( p, pObj, i )
    {
        if ( Gia_ObjIsAnd(pObj) )
            pObj->Value = Gia_ManAppendAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
        else if ( Gia_ObjIsCi(pObj) )
        {
            pObj->Value = Gia_ManAppendCi( pNew );
            if ( Gia_ObjCioId(pObj) >= Gia_ManPiNum(p) )
                pObj->Value = Abc_LitNotCond( pObj->Value, Abc_InfoHasBit((unsigned *)pInitState, Gia_ObjCioId(pObj) - Gia_ManPiNum(p)) );
        }
        else if ( Gia_ObjIsCo(pObj) )
        {
            pObj->Value = Gia_ObjFanin0Copy(pObj);
            if ( Gia_ObjCioId(pObj) >= Gia_ManPoNum(p) )
                pObj->Value = Abc_LitNotCond( pObj->Value, Abc_InfoHasBit((unsigned *)pInitState, Gia_ObjCioId(pObj) - Gia_ManPoNum(p)) );
            pObj->Value = Gia_ManAppendCo( pNew, pObj->Value );
        }
    }
    Gia_ManSetRegNum( pNew, Gia_ManRegNum(p) );
    return pNew;
}


/**Function*************************************************************

  Synopsis    [Cycles AIG using random input.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManCycle( Gia_Man_t * p, Abc_Cex_t * pCex, int nFrames )
{
    Gia_Obj_t * pObj, * pObjRi, * pObjRo;
    int i, k;
    Gia_ManRandom( 1 );
    assert( pCex == NULL || nFrames <= pCex->iFrame );
    // iterate for the given number of frames
    for ( i = 0; i < nFrames; i++ )
    {
        Gia_ManForEachPi( p, pObj, k )
            pObj->fMark0 = pCex ? Abc_InfoHasBit(pCex->pData, pCex->nRegs+i*pCex->nPis+k) : (1 & Gia_ManRandom(0));
        Gia_ManForEachAnd( p, pObj, k )
            pObj->fMark0 = (Gia_ObjFanin0(pObj)->fMark0 ^ Gia_ObjFaninC0(pObj)) & 
                           (Gia_ObjFanin1(pObj)->fMark0 ^ Gia_ObjFaninC1(pObj));
        Gia_ManForEachCo( p, pObj, k )
            pObj->fMark0 = Gia_ObjFanin0(pObj)->fMark0 ^ Gia_ObjFaninC0(pObj);
        Gia_ManForEachRiRo( p, pObjRi, pObjRo, k )
            pObjRo->fMark0 = pObjRi->fMark0;
    }
}
Gia_Man_t * Gia_ManDupCycled( Gia_Man_t * p, Abc_Cex_t * pCex, int nFrames )
{
    Gia_Man_t * pNew;
    Vec_Bit_t * vInits;
    Gia_Obj_t * pObj;
    int i;
    Gia_ManCleanMark0(p);
    Gia_ManCycle( p, pCex, nFrames );
    vInits = Vec_BitAlloc( Gia_ManRegNum(p) );
    Gia_ManForEachRo( p, pObj, i )
        Vec_BitPush( vInits, pObj->fMark0 );
    pNew = Gia_ManDupFlip( p, Vec_BitArray(vInits) );
    Vec_BitFree( vInits );
    Gia_ManCleanMark0(p);
    return pNew;
}


/**Function*************************************************************

  Synopsis    [Duplicates AIG without any changes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManDup( Gia_Man_t * p )
{
    Gia_Man_t * pNew;
    Gia_Obj_t * pObj;
    int i;
    pNew = Gia_ManStart( Gia_ManObjNum(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    if ( Gia_ManHasChoices(p) )
        pNew->pSibls = ABC_CALLOC( int, Gia_ManObjNum(p) );
    Gia_ManConst0(p)->Value = 0;
    Gia_ManForEachObj1( p, pObj, i )
    {
        if ( Gia_ObjIsBuf(pObj) )
            pObj->Value = Gia_ManAppendBuf( pNew, Gia_ObjFanin0Copy(pObj) );
        else if ( Gia_ObjIsAnd(pObj) )
        {
            pObj->Value = Gia_ManAppendAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
            if ( Gia_ObjSibl(p, Gia_ObjId(p, pObj)) )
                pNew->pSibls[Abc_Lit2Var(pObj->Value)] = Abc_Lit2Var(Gia_ObjSiblObj(p, Gia_ObjId(p, pObj))->Value);  
        }
        else if ( Gia_ObjIsCi(pObj) )
            pObj->Value = Gia_ManAppendCi( pNew );
        else if ( Gia_ObjIsCo(pObj) )
            pObj->Value = Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
    }
    Gia_ManSetRegNum( pNew, Gia_ManRegNum(p) );
    if ( p->pCexSeq )
        pNew->pCexSeq = Abc_CexDup( p->pCexSeq, Gia_ManRegNum(p) );
    return pNew;
}
Gia_Man_t * Gia_ManDup2( Gia_Man_t * p1, Gia_Man_t * p2 )
{
    Gia_Man_t * pNew;
    Gia_Obj_t * pObj;
    int i;
    assert( Gia_ManCiNum(p1) == Gia_ManCiNum(p2) );
    assert( Gia_ManCoNum(p1) == Gia_ManCoNum(p2) );
    pNew = Gia_ManStart( Gia_ManObjNum(p1) + Gia_ManObjNum(p2) );
    Gia_ManHashStart( pNew );
    Gia_ManConst0(p1)->Value = 0;
    Gia_ManConst0(p2)->Value = 0;
    Gia_ManForEachCi( p1, pObj, i )
        pObj->Value = Gia_ManCi(p2, i)->Value = Gia_ManAppendCi( pNew );
    Gia_ManForEachAnd( p1, pObj, i )
        pObj->Value = Gia_ManHashAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
    Gia_ManForEachAnd( p2, pObj, i )
        pObj->Value = Gia_ManHashAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
    Gia_ManForEachCo( p1, pObj, i )
        pObj->Value = Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
    Gia_ManForEachCo( p2, pObj, i )
        pObj->Value = Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
    Gia_ManSetRegNum( pNew, Gia_ManRegNum(p1) );
    Gia_ManHashStop( pNew );
    return pNew;
}
Gia_Man_t * Gia_ManDupWithAttributes( Gia_Man_t * p )
{
    Gia_Man_t * pNew = Gia_ManDup(p);
    Gia_ManTransferMapping( pNew, p );
    Gia_ManTransferPacking( pNew, p );
    if ( p->pManTime )
        pNew->pManTime = Tim_ManDup( (Tim_Man_t *)p->pManTime, 0 );
    if ( p->pAigExtra )
        pNew->pAigExtra = Gia_ManDup( p->pAigExtra );
    if ( p->nAnd2Delay )
        pNew->nAnd2Delay = p->nAnd2Delay;
    if ( p->vRegClasses )
        pNew->vRegClasses = Vec_IntDup( p->vRegClasses );
    if ( p->vRegInits )
        pNew->vRegInits = Vec_IntDup( p->vRegInits );
    if ( p->vConfigs )
        pNew->vConfigs = Vec_IntDup( p->vConfigs );
    if ( p->pCellStr )
        pNew->pCellStr = Abc_UtilStrsav( p->pCellStr );
    return pNew;
}
Gia_Man_t * Gia_ManDupRemovePis( Gia_Man_t * p, int nRemPis )
{
    Gia_Man_t * pNew;
    Gia_Obj_t * pObj;
    int i;
    pNew = Gia_ManStart( Gia_ManObjNum(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    Gia_ManConst0(p)->Value = 0;
    Gia_ManForEachObj1( p, pObj, i )
    {
        if ( Gia_ObjIsAnd(pObj) )
            pObj->Value = Gia_ManAppendAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
        else if ( Gia_ObjIsCi(pObj) && Gia_ObjCioId(pObj) < Gia_ManCiNum(p)-nRemPis )
            pObj->Value = Gia_ManAppendCi( pNew );
        else if ( Gia_ObjIsCo(pObj) )
            pObj->Value = Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
    }
    Gia_ManSetRegNum( pNew, Gia_ManRegNum(p) );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Duplicates AIG without any changes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManDupZero( Gia_Man_t * p )
{
    Gia_Man_t * pNew; int i;
    pNew = Gia_ManStart( 1 + Gia_ManCiNum(p) + Gia_ManCoNum(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    for ( i = 0; i < Gia_ManCiNum(p); i++ )
        Gia_ManAppendCi( pNew );
    for ( i = 0; i < Gia_ManCoNum(p); i++ )
        Gia_ManAppendCo( pNew, 0 );
    Gia_ManSetRegNum( pNew, Gia_ManRegNum(p) );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Duplicates AIG without any changes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManDupPerm( Gia_Man_t * p, Vec_Int_t * vPiPerm )
{
//    Vec_Int_t * vPiPermInv;
    Gia_Man_t * pNew;
    Gia_Obj_t * pObj;
    int i;
    assert( Vec_IntSize(vPiPerm) == Gia_ManPiNum(p) );
    pNew = Gia_ManStart( Gia_ManObjNum(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    Gia_ManConst0(p)->Value = 0;
//    vPiPermInv = Vec_IntInvert( vPiPerm, -1 );
    Gia_ManForEachPi( p, pObj, i )
//        Gia_ManPi(p, Vec_IntEntry(vPiPermInv,i))->Value = Gia_ManAppendCi( pNew );
        Gia_ManPi(p, Vec_IntEntry(vPiPerm,i))->Value = Gia_ManAppendCi( pNew );
//    Vec_IntFree( vPiPermInv );
    Gia_ManForEachObj1( p, pObj, i )
    {
        if ( Gia_ObjIsAnd(pObj) )
            pObj->Value = Gia_ManAppendAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
        else if ( Gia_ObjIsCi(pObj) )
        {
            if ( Gia_ObjIsRo(p, pObj) )
                pObj->Value = Gia_ManAppendCi( pNew );
        }
        else if ( Gia_ObjIsCo(pObj) )
            pObj->Value = Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
    }
    Gia_ManSetRegNum( pNew, Gia_ManRegNum(p) );
    return pNew;
}
Gia_Man_t * Gia_ManDupPermFlop( Gia_Man_t * p, Vec_Int_t * vFfPerm )
{
    Vec_Int_t * vPermInv;
    Gia_Man_t * pNew;
    Gia_Obj_t * pObj;
    int i;
    assert( Vec_IntSize(vFfPerm) == Gia_ManRegNum(p) );
    vPermInv = Vec_IntInvert( vFfPerm, -1 );
    pNew = Gia_ManStart( Gia_ManObjNum(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    Gia_ManConst0(p)->Value = 0;
    Gia_ManForEachPi( p, pObj, i )
        pObj->Value = Gia_ManAppendCi(pNew);
    Gia_ManForEachRo( p, pObj, i )
        Gia_ManRo(p, Vec_IntEntry(vPermInv, i))->Value = Gia_ManAppendCi(pNew);
    Gia_ManForEachAnd( p, pObj, i )
        pObj->Value = Gia_ManAppendAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
    Gia_ManForEachPo( p, pObj, i )
        pObj->Value = Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
    Gia_ManForEachRi( p, pObj, i )
        pObj->Value = Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy( Gia_ManRi(p, Vec_IntEntry(vPermInv, i)) ) );
    Vec_IntFree( vPermInv );
    Gia_ManSetRegNum( pNew, Gia_ManRegNum(p) );
    return pNew;
}
Gia_Man_t * Gia_ManDupSpreadFlop( Gia_Man_t * p, Vec_Int_t * vFfMask )
{
    Gia_Man_t * pNew;
    Gia_Obj_t * pObj;
    int i, k, Entry;
    assert( Vec_IntSize(vFfMask) >= Gia_ManRegNum(p) );
    pNew = Gia_ManStart( Gia_ManObjNum(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    Gia_ManConst0(p)->Value = 0;
    Gia_ManForEachPi( p, pObj, i )
        pObj->Value = Gia_ManAppendCi(pNew);
    k = 0;
    Vec_IntForEachEntry( vFfMask, Entry, i )
        if ( Entry == -1 )
            Gia_ManAppendCi(pNew);
        else
            Gia_ManRo(p, k++)->Value = Gia_ManAppendCi(pNew);
    assert( k == Gia_ManRegNum(p) );
    Gia_ManForEachAnd( p, pObj, i )
        pObj->Value = Gia_ManAppendAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
    Gia_ManForEachPo( p, pObj, i )
        pObj->Value = Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
    k = 0;
    Vec_IntForEachEntry( vFfMask, Entry, i )
        if ( Entry == -1 )
            Gia_ManAppendCo( pNew, 0 );
        else
        {
            pObj = Gia_ManRi( p, k++ );
            pObj->Value = Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
        }
    assert( k == Gia_ManRegNum(p) );
    Gia_ManSetRegNum( pNew, Vec_IntSize(vFfMask) );
    return pNew;
}
Gia_Man_t * Gia_ManDupPermFlopGap( Gia_Man_t * p, Vec_Int_t * vFfMask )
{
    Vec_Int_t * vPerm = Vec_IntCondense( vFfMask, -1 );
    Gia_Man_t * pPerm = Gia_ManDupPermFlop( p, vPerm );
    Gia_Man_t * pSpread = Gia_ManDupSpreadFlop( pPerm, vFfMask );
    Vec_IntFree( vPerm );
    Gia_ManStop( pPerm );
    return pSpread;
}

/**Function*************************************************************

  Synopsis    [Appends second AIG without any changes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManDupAppend( Gia_Man_t * pNew, Gia_Man_t * pTwo )
{
    Gia_Obj_t * pObj;
    int i;
    if ( pNew->nRegs > 0 )
        pNew->nRegs = 0;
    if ( Vec_IntSize(&pNew->vHTable) == 0 )
        Gia_ManHashStart( pNew );
    Gia_ManConst0(pTwo)->Value = 0;
    Gia_ManForEachObj1( pTwo, pObj, i )
    {
        if ( Gia_ObjIsAnd(pObj) )
            pObj->Value = Gia_ManAppendAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
        else if ( Gia_ObjIsCi(pObj) )
            pObj->Value = Gia_ManAppendCi( pNew );
        else if ( Gia_ObjIsCo(pObj) )
            pObj->Value = Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
    }
}
void Gia_ManDupAppendShare( Gia_Man_t * pNew, Gia_Man_t * pTwo )
{
    Gia_Obj_t * pObj;
    int i;
    assert( Gia_ManCiNum(pNew) == Gia_ManCiNum(pTwo) );
    if ( Vec_IntSize(&pNew->vHTable) == 0 )
        Gia_ManHashStart( pNew );
    Gia_ManConst0(pTwo)->Value = 0;
    Gia_ManForEachObj1( pTwo, pObj, i )
    {
        if ( Gia_ObjIsAnd(pObj) )
            pObj->Value = Gia_ManHashAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
        else if ( Gia_ObjIsCi(pObj) )
            pObj->Value = Gia_Obj2Lit( pNew, Gia_ManCi( pNew, Gia_ObjCioId(pObj) ) );
        else if ( Gia_ObjIsCo(pObj) )
            pObj->Value = Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
    }
}
Gia_Man_t * Gia_ManDupAppendNew( Gia_Man_t * pOne, Gia_Man_t * pTwo )
{
    Gia_Man_t * pNew;
    Gia_Obj_t * pObj;
    int i;
    pNew = Gia_ManStart( Gia_ManObjNum(pOne) + Gia_ManObjNum(pTwo) );
    pNew->pName = Abc_UtilStrsav( pOne->pName );
    pNew->pSpec = Abc_UtilStrsav( pOne->pSpec );
    Gia_ManHashAlloc( pNew );
    Gia_ManConst0(pOne)->Value = 0;
    Gia_ManForEachObj1( pOne, pObj, i )
    {
        if ( Gia_ObjIsAnd(pObj) )
            pObj->Value = Gia_ManHashAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
        else if ( Gia_ObjIsCi(pObj) )
            pObj->Value = Gia_ManAppendCi( pNew );
    }
    Gia_ManConst0(pTwo)->Value = 0;
    Gia_ManForEachObj1( pTwo, pObj, i )
    {
        if ( Gia_ObjIsAnd(pObj) )
            pObj->Value = Gia_ManHashAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
        else if ( Gia_ObjIsPi(pTwo, pObj) )
            pObj->Value = Gia_ManPi(pOne, Gia_ObjCioId(pObj))->Value;
        else if ( Gia_ObjIsCi(pObj) )
            pObj->Value = Gia_ManAppendCi( pNew );
    }
    Gia_ManHashStop( pNew );
    // primary outputs
    Gia_ManForEachPo( pOne, pObj, i )
        pObj->Value = Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
    Gia_ManForEachPo( pTwo, pObj, i )
        pObj->Value = Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
    // flop inputs
    Gia_ManForEachRi( pOne, pObj, i )
        pObj->Value = Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
    Gia_ManForEachRi( pTwo, pObj, i )
        pObj->Value = Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
    Gia_ManSetRegNum( pNew, Gia_ManRegNum(pOne) + Gia_ManRegNum(pTwo) );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Creates a miter for inductive checking of the invariant.]

  Description [The first GIA (p) is a sequential AIG whose transition
  relation is used. The second GIA (pInv) is a combinational AIG representing
  the invariant over the register outputs. If the resulting combination miter
  is UNSAT, the invariant holds by simple induction.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManDupInvMiter( Gia_Man_t * p, Gia_Man_t * pInv )
{
    Gia_Man_t * pNew, * pTemp;
    Gia_Obj_t * pObj;
    int i, Node1, Node2, Node;
    assert( Gia_ManRegNum(p) > 0 );
    assert( Gia_ManRegNum(pInv) == 0 );
    assert( Gia_ManCoNum(pInv) == 1 );
    assert( Gia_ManRegNum(p) == Gia_ManCiNum(pInv) );
    Gia_ManFillValue(p);
    pNew = Gia_ManStart( Gia_ManObjNum(p) + 2*Gia_ManObjNum(pInv) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    Gia_ManHashAlloc( pNew );
    Gia_ManConst0(p)->Value = 0;
    Gia_ManForEachObj1( p, pObj, i )
    {
        if ( Gia_ObjIsAnd(pObj) )
            pObj->Value = Gia_ManHashAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
        else if ( Gia_ObjIsCi(pObj) )
            pObj->Value = Gia_ManAppendCi( pNew );
        else if ( Gia_ObjIsCo(pObj) )
            pObj->Value = Gia_ObjFanin0Copy(pObj);
    }
    // build invariant on top of register outputs in the first frame
    Gia_ManForEachRo( p, pObj, i )
        Gia_ManCi(pInv, i)->Value = pObj->Value;
    Gia_ManForEachAnd( pInv, pObj, i )
        pObj->Value = Gia_ManHashAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
    pObj = Gia_ManCo( pInv, 0 );
    Node1 = Gia_ObjFanin0Copy(pObj);
    // build invariant on top of register outputs in the second frame
    Gia_ManForEachRi( p, pObj, i )
        Gia_ManCi(pInv, i)->Value = pObj->Value;
    Gia_ManForEachAnd( pInv, pObj, i )
        pObj->Value = Gia_ManHashAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
    pObj = Gia_ManCo( pInv, 0 );
    Node2 = Gia_ObjFanin0Copy(pObj);
    // create miter output
    Node = Gia_ManHashAnd( pNew, Node1, Abc_LitNot(Node2) );
    Gia_ManAppendCo( pNew, Node );
    // cleanup
    pNew = Gia_ManCleanup( pTemp = pNew );
    Gia_ManStop( pTemp );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Appends logic cones as additional property outputs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManDupAppendCones( Gia_Man_t * p, Gia_Man_t ** ppCones, int nCones, int fOnlyRegs )
{
    Gia_Man_t * pNew, * pOne;
    Gia_Obj_t * pObj;
    int i, k;
    pNew = Gia_ManStart( Gia_ManObjNum(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    Gia_ManHashAlloc( pNew );
    Gia_ManConst0(p)->Value = 0;
    Gia_ManForEachCi( p, pObj, i )
        pObj->Value = Gia_ManAppendCi( pNew );
    Gia_ManForEachAnd( p, pObj, i )
        pObj->Value = Gia_ManHashAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
    Gia_ManForEachPo( p, pObj, i )
        pObj->Value = Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
    for ( k = 0; k < nCones; k++ )
    {
        pOne = ppCones[k];
        assert( Gia_ManPoNum(pOne) == 1 );
        assert( Gia_ManRegNum(pOne) == 0 );
        if ( fOnlyRegs )
            assert( Gia_ManPiNum(pOne) == Gia_ManRegNum(p) );
        else
            assert( Gia_ManPiNum(pOne) == Gia_ManCiNum(p) );
        Gia_ManConst0(pOne)->Value = 0;
        Gia_ManForEachPi( pOne, pObj, i )
            pObj->Value = Gia_ManCiLit( pNew, fOnlyRegs ? Gia_ManPiNum(p) + i : i );
        Gia_ManForEachAnd( pOne, pObj, i )
            pObj->Value = Gia_ManHashAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
        Gia_ManForEachPo( pOne, pObj, i )
            pObj->Value = Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
    }
    Gia_ManForEachRi( p, pObj, i )
        pObj->Value = Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
    Gia_ManHashStop( pNew );
    Gia_ManSetRegNum( pNew, Gia_ManRegNum(p) );
    pNew = Gia_ManCleanup( pOne = pNew );
    Gia_ManStop( pOne );
    return pNew;
}


/**Function*************************************************************

  Synopsis    [Duplicates while adding self-loops to the registers.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManDupSelf( Gia_Man_t * p )
{
    Gia_Man_t * pNew, * pTemp; 
    Gia_Obj_t * pObj;
    int i, iCtrl;
    pNew = Gia_ManStart( Gia_ManObjNum(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    Gia_ManHashAlloc( pNew );
    Gia_ManFillValue( p );
    Gia_ManConst0(p)->Value = 0;
    iCtrl = Gia_ManAppendCi( pNew );
    Gia_ManForEachCi( p, pObj, i )
        pObj->Value = Gia_ManAppendCi( pNew );
    Gia_ManForEachAnd( p, pObj, i )
        pObj->Value = Gia_ManHashAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
    Gia_ManForEachPo( p, pObj, i )
        pObj->Value = Gia_ObjFanin0Copy(pObj);
    Gia_ManForEachRi( p, pObj, i )
        pObj->Value = Gia_ManHashMux( pNew, iCtrl, Gia_ObjFanin0Copy(pObj), Gia_ObjRiToRo(p, pObj)->Value );
    Gia_ManForEachCo( p, pObj, i )
        Gia_ManAppendCo( pNew, pObj->Value );
    Gia_ManHashStop( pNew );
    Gia_ManSetRegNum( pNew, Gia_ManRegNum(p) );
    pNew = Gia_ManCleanup( pTemp = pNew );
    Gia_ManStop( pTemp );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Duplicates while adding self-loops to the registers.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManDupFlopClass( Gia_Man_t * p, int iClass )
{
    Gia_Man_t * pNew; 
    Gia_Obj_t * pObj;
    int i, Counter1 = 0, Counter2 = 0;
    assert( p->vFlopClasses != NULL );
    pNew = Gia_ManStart( Gia_ManObjNum(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    Gia_ManFillValue( p );
    Gia_ManConst0(p)->Value = 0;
    Gia_ManForEachPi( p, pObj, i )
        pObj->Value = Gia_ManAppendCi( pNew );
    Gia_ManForEachRo( p, pObj, i )
        if ( Vec_IntEntry(p->vFlopClasses, i) != iClass )
            pObj->Value = Gia_ManAppendCi( pNew );
    Gia_ManForEachRo( p, pObj, i )
        if ( Vec_IntEntry(p->vFlopClasses, i) == iClass )
            pObj->Value = Gia_ManAppendCi( pNew ), Counter1++;
    Gia_ManForEachAnd( p, pObj, i )
        pObj->Value = Gia_ManAppendAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
    Gia_ManForEachPo( p, pObj, i )
        Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
    Gia_ManForEachRi( p, pObj, i )
        if ( Vec_IntEntry(p->vFlopClasses, i) != iClass )
            Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
    Gia_ManForEachRi( p, pObj, i )
        if ( Vec_IntEntry(p->vFlopClasses, i) == iClass )
            Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) ), Counter2++;
    assert( Counter1 == Counter2 );
    Gia_ManSetRegNum( pNew, Counter1 );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Duplicates AIG without any changes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManDupMarked( Gia_Man_t * p )
{
    Gia_Man_t * pNew;
    Gia_Obj_t * pObj;
    int i, nRos = 0, nRis = 0;
    int CountMarked = 0;
    Gia_ManForEachObj( p, pObj, i )
        CountMarked += pObj->fMark0;
    Gia_ManFillValue( p );
    pNew = Gia_ManStart( Gia_ManObjNum(p) - CountMarked );
    if ( p->pMuxes )
        pNew->pMuxes = ABC_CALLOC( unsigned, pNew->nObjsAlloc );
    pNew->nConstrs = p->nConstrs;
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    Gia_ManConst0(p)->Value = 0;
    Gia_ManForEachObj1( p, pObj, i )
    {
        if ( pObj->fMark0 )
        {
            assert( !Gia_ObjIsBuf(pObj) );
            pObj->fMark0 = 0;
            continue;
        }
        if ( Gia_ObjIsBuf(pObj) )
            pObj->Value = Gia_ManAppendBuf( pNew, Gia_ObjFanin0Copy(pObj) );
        else if ( Gia_ObjIsAnd(pObj) )
        {
            if ( Gia_ObjIsXor(pObj) )
                pObj->Value = Gia_ManAppendXorReal( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
            else if ( Gia_ObjIsMux(p, pObj) )
                pObj->Value = Gia_ManAppendMuxReal( pNew, Gia_ObjFanin2Copy(p, pObj), Gia_ObjFanin1Copy(pObj), Gia_ObjFanin0Copy(pObj) );
            else
                pObj->Value = Gia_ManAppendAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
        }
        else if ( Gia_ObjIsCi(pObj) )
        {
            pObj->Value = Gia_ManAppendCi( pNew );
            nRos += Gia_ObjIsRo(p, pObj);
        }
        else if ( Gia_ObjIsCo(pObj) )
        {
//            Gia_Obj_t * pFanin = Gia_ObjFanin0(pObj);
            pObj->Value = Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
            nRis += Gia_ObjIsRi(p, pObj);
        }
    }
    assert( pNew->nObjsAlloc == pNew->nObjs );
    assert( nRos == nRis );
    Gia_ManSetRegNum( pNew, nRos );
    if ( p->pReprs && p->pNexts )
    {
        Gia_Obj_t * pRepr;
        pNew->pReprs = ABC_CALLOC( Gia_Rpr_t, Gia_ManObjNum(pNew) );
        for ( i = 0; i < Gia_ManObjNum(p); i++ )
            Gia_ObjSetRepr( pNew, i, GIA_VOID );
        Gia_ManForEachObj1( p, pObj, i )
        {
            if ( !~pObj->Value )
                continue;
            pRepr = Gia_ObjReprObj( p, i );
            if ( pRepr == NULL )
                continue;
            if ( !~pRepr->Value )
                continue;
            assert( !Gia_ObjIsBuf(pObj) );
            if ( Abc_Lit2Var(pObj->Value) != Abc_Lit2Var(pRepr->Value) )
                Gia_ObjSetRepr( pNew, Abc_Lit2Var(pObj->Value), Abc_Lit2Var(pRepr->Value) ); 
        }
        pNew->pNexts = Gia_ManDeriveNexts( pNew );
    }
    if ( Gia_ManHasChoices(p) )
    {
        Gia_Obj_t * pSibl;
        pNew->pSibls = ABC_CALLOC( int, Gia_ManObjNum(pNew) );
        Gia_ManForEachObj1( p, pObj, i )
        {
            if ( !~pObj->Value )
                continue;
            pSibl = Gia_ObjSiblObj( p, i );
            if ( pSibl == NULL )
                continue;
            if ( !~pSibl->Value )
                continue;
            assert( !Gia_ObjIsBuf(pObj) );
            assert( Abc_Lit2Var(pObj->Value) > Abc_Lit2Var(pSibl->Value) );
            pNew->pSibls[Abc_Lit2Var(pObj->Value)] = Abc_Lit2Var(pSibl->Value);
        }
    }
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Duplicates AIG while creating "parallel" copies.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManDupTimes( Gia_Man_t * p, int nTimes )
{
    Gia_Man_t * pNew;
    Gia_Obj_t * pObj;
    Vec_Int_t * vPis, * vPos, * vRis, * vRos;
    int i, t, Entry;
    assert( nTimes > 0 );
    pNew = Gia_ManStart( Gia_ManObjNum(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    Gia_ManConst0(p)->Value = 0;
    vPis = Vec_IntAlloc( Gia_ManPiNum(p) * nTimes );
    vPos = Vec_IntAlloc( Gia_ManPoNum(p) * nTimes );
    vRis = Vec_IntAlloc( Gia_ManRegNum(p) * nTimes );
    vRos = Vec_IntAlloc( Gia_ManRegNum(p) * nTimes );
    for ( t = 0; t < nTimes; t++ )
    {
        Gia_ManForEachObj1( p, pObj, i )
        {
            if ( Gia_ObjIsAnd(pObj) )
                pObj->Value = Gia_ManAppendAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
            else if ( Gia_ObjIsCi(pObj) )
            {
                pObj->Value = Gia_ManAppendCi( pNew );
                if ( Gia_ObjIsPi(p, pObj) )
                    Vec_IntPush( vPis, Abc_Lit2Var(pObj->Value) );
                else
                    Vec_IntPush( vRos, Abc_Lit2Var(pObj->Value) );
            }
            else if ( Gia_ObjIsCo(pObj) )
            {
                pObj->Value = Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
                if ( Gia_ObjIsPo(p, pObj) )
                    Vec_IntPush( vPos, Abc_Lit2Var(pObj->Value) );
                else
                    Vec_IntPush( vRis, Abc_Lit2Var(pObj->Value) );
            }
        }
    }
    Vec_IntClear( pNew->vCis );
    Vec_IntForEachEntry( vPis, Entry, i )
    {
        Gia_ObjSetCioId( Gia_ManObj(pNew, Entry), Vec_IntSize(pNew->vCis) );
        Vec_IntPush( pNew->vCis, Entry );
    }
    Vec_IntForEachEntry( vRos, Entry, i )
    {
        Gia_ObjSetCioId( Gia_ManObj(pNew, Entry), Vec_IntSize(pNew->vCis) );
        Vec_IntPush( pNew->vCis, Entry );
    }
    Vec_IntClear( pNew->vCos );
    Vec_IntForEachEntry( vPos, Entry, i )
    {
        Gia_ObjSetCioId( Gia_ManObj(pNew, Entry), Vec_IntSize(pNew->vCos) );
        Vec_IntPush( pNew->vCos, Entry );
    }
    Vec_IntForEachEntry( vRis, Entry, i )
    {
        Gia_ObjSetCioId( Gia_ManObj(pNew, Entry), Vec_IntSize(pNew->vCos) );
        Vec_IntPush( pNew->vCos, Entry );
    }
    Vec_IntFree( vPis );
    Vec_IntFree( vPos );
    Vec_IntFree( vRis );
    Vec_IntFree( vRos );
    Gia_ManSetRegNum( pNew, nTimes * Gia_ManRegNum(p) );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Duplicates the AIG in the DFS order.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManDupDfs2_rec( Gia_Man_t * pNew, Gia_Man_t * p, Gia_Obj_t * pObj )
{
    if ( ~pObj->Value )
        return pObj->Value;
    if ( p->pReprsOld && ~p->pReprsOld[Gia_ObjId(p, pObj)] )
    {
        Gia_Obj_t * pRepr = Gia_ManObj( p, p->pReprsOld[Gia_ObjId(p, pObj)] );
        pRepr->Value = Gia_ManDupDfs2_rec( pNew, p, pRepr );
        return pObj->Value = Abc_LitNotCond( pRepr->Value, Gia_ObjPhaseReal(pRepr) ^ Gia_ObjPhaseReal(pObj) );
    }
    if ( Gia_ObjIsCi(pObj) )
        return pObj->Value = Gia_ManAppendCi(pNew);
    Gia_ManDupDfs2_rec( pNew, p, Gia_ObjFanin0(pObj) );
    if ( Gia_ObjIsCo(pObj) )
        return pObj->Value = Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
    Gia_ManDupDfs2_rec( pNew, p, Gia_ObjFanin1(pObj) );
    if ( Vec_IntSize(&pNew->vHTable) )
        return pObj->Value = Gia_ManHashAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
    return pObj->Value = Gia_ManAppendAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
} 

/**Function*************************************************************

  Synopsis    [Duplicates AIG in the DFS order.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManDupDfs2( Gia_Man_t * p )
{
    Gia_Man_t * pNew;
    Gia_Obj_t * pObj, * pObjNew;
    int i;
    Gia_ManFillValue( p );
    pNew = Gia_ManStart( Gia_ManObjNum(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    Gia_ManConst0(p)->Value = 0;
    Gia_ManForEachCo( p, pObj, i )
        Gia_ManDupDfs2_rec( pNew, p, pObj );
    Gia_ManForEachCi( p, pObj, i )
        if ( ~pObj->Value == 0 )
            pObj->Value = Gia_ManAppendCi(pNew);
    assert( Gia_ManCiNum(pNew) == Gia_ManCiNum(p) );
    // remap combinational inputs
    Gia_ManForEachCi( p, pObj, i )
    {
        pObjNew = Gia_ObjFromLit( pNew, pObj->Value );
        assert( !Gia_IsComplement(pObjNew) );
        Vec_IntWriteEntry( pNew->vCis, Gia_ObjCioId(pObj), Gia_ObjId(pNew, pObjNew) );
        Gia_ObjSetCioId( pObjNew, Gia_ObjCioId(pObj) );
    }
    Gia_ManSetRegNum( pNew, Gia_ManRegNum(p) );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Duplicates the AIG in the DFS order.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManDupDfs_rec( Gia_Man_t * pNew, Gia_Man_t * p, Gia_Obj_t * pObj )
{
    if ( ~pObj->Value )
        return;
    assert( Gia_ObjIsAnd(pObj) );
    Gia_ManDupDfs_rec( pNew, p, Gia_ObjFanin0(pObj) );
    Gia_ManDupDfs_rec( pNew, p, Gia_ObjFanin1(pObj) );
    pObj->Value = Gia_ManAppendAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
}
Gia_Man_t * Gia_ManDupDfs( Gia_Man_t * p )
{
    Gia_Man_t * pNew;
    Gia_Obj_t * pObj;
    int i;
    pNew = Gia_ManStart( Gia_ManObjNum(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    Gia_ManFillValue( p );
    Gia_ManConst0(p)->Value = 0;
    Gia_ManForEachCi( p, pObj, i )
        pObj->Value = Gia_ManAppendCi(pNew);
    Gia_ManForEachCo( p, pObj, i )
        Gia_ManDupDfs_rec( pNew, p, Gia_ObjFanin0(pObj) );
    Gia_ManForEachCo( p, pObj, i )
        pObj->Value = Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
    Gia_ManSetRegNum( pNew, Gia_ManRegNum(p) );
    pNew->nConstrs = p->nConstrs;
    if ( p->pCexSeq )
        pNew->pCexSeq = Abc_CexDup( p->pCexSeq, Gia_ManRegNum(p) );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Cofactors w.r.t. a primary input variable.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManDupCofactorVar_rec( Gia_Man_t * pNew, Gia_Man_t * p, Gia_Obj_t * pObj )
{
    if ( ~pObj->Value )
        return;
    assert( Gia_ObjIsAnd(pObj) );
    Gia_ManDupCofactorVar_rec( pNew, p, Gia_ObjFanin0(pObj) );
    Gia_ManDupCofactorVar_rec( pNew, p, Gia_ObjFanin1(pObj) );
    pObj->Value = Gia_ManHashAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
}
Gia_Man_t * Gia_ManDupCofactorVar( Gia_Man_t * p, int iVar, int Value )
{
    Gia_Man_t * pNew, * pTemp;
    Gia_Obj_t * pObj;
    int i;
    assert( iVar >= 0 && iVar < Gia_ManPiNum(p) );
    assert( Value == 0 || Value == 1 );
    pNew = Gia_ManStart( Gia_ManObjNum(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    Gia_ManFillValue( p );
    Gia_ManConst0(p)->Value = 0;
    Gia_ManForEachCi( p, pObj, i )
        pObj->Value = Gia_ManAppendCi(pNew);
    Gia_ManPi( p, iVar )->Value = Value; // modification!
    Gia_ManHashAlloc( pNew );
    Gia_ManForEachCo( p, pObj, i )
        Gia_ManDupCofactorVar_rec( pNew, p, Gia_ObjFanin0(pObj) );
    Gia_ManForEachCo( p, pObj, i )
        Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
    Gia_ManSetRegNum( pNew, Gia_ManRegNum(p) );
    pNew->nConstrs = p->nConstrs;
    pNew = Gia_ManCleanup( pTemp = pNew );
    Gia_ManStop( pTemp );
    return pNew;
}
Gia_Man_t * Gia_ManDupMux( int iVar, Gia_Man_t * pCof1, Gia_Man_t * pCof0 )
{
    Gia_Man_t * pGia[2] = {pCof0, pCof1};
    Gia_Man_t * pNew, * pTemp;
    Gia_Obj_t * pObj;
    int i, n;
    assert( Gia_ManRegNum(pCof0) == 0 );
    assert( Gia_ManRegNum(pCof1) == 0 );
    assert( Gia_ManCoNum(pCof0) == 1 );
    assert( Gia_ManCoNum(pCof1) == 1 );
    assert( Gia_ManCiNum(pCof1) == Gia_ManCiNum(pCof0) );
    assert( iVar >= 0 && iVar < Gia_ManCiNum(pCof1) );
    pNew = Gia_ManStart( Gia_ManObjNum(pCof1) + Gia_ManObjNum(pCof0) );
    pNew->pName = Abc_UtilStrsav( pCof1->pName );
    pNew->pSpec = Abc_UtilStrsav( pCof1->pSpec );
    Gia_ManHashAlloc( pNew );
    for ( n = 0; n < 2; n++ )
    {
        Gia_ManFillValue( pGia[n] );
        Gia_ManConst0(pGia[n])->Value = 0;
        Gia_ManForEachCi( pGia[n], pObj, i )
            pObj->Value = n ? Gia_ManCi(pGia[0], i)->Value : Gia_ManAppendCi(pNew);
        Gia_ManForEachCo( pGia[n], pObj, i )
            Gia_ManDupCofactorVar_rec( pNew, pGia[n], Gia_ObjFanin0(pObj) );
    }
    Gia_ManForEachCo( pGia[0], pObj, i )
    {
        int Ctrl = Gia_ManCi(pGia[0], iVar)->Value;
        int Lit1 = Gia_ObjFanin0Copy(Gia_ManCo(pGia[1], i));
        int Lit0 = Gia_ObjFanin0Copy(pObj);
        Gia_ManAppendCo( pNew, Gia_ManHashMux( pNew, Ctrl, Lit1, Lit0 ) );
    }
    pNew = Gia_ManCleanup( pTemp = pNew );
    Gia_ManStop( pTemp );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Cofactors w.r.t. an internal node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManDupCofactorObj( Gia_Man_t * p, int iObj, int Value )
{
    Gia_Man_t * pNew, * pTemp;
    Gia_Obj_t * pObj;
    int i, iObjValue = -1;
    assert( Gia_ManRegNum(p) == 0 );
    assert( iObj > 0 && iObj < Gia_ManObjNum(p) );
    assert( Gia_ObjIsCand(Gia_ManObj(p, iObj)) );
    assert( Value == 0 || Value == 1 );
    pNew = Gia_ManStart( Gia_ManObjNum(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    Gia_ManConst0(p)->Value = 0;
    Gia_ManHashAlloc( pNew );
    Gia_ManForEachObj1( p, pObj, i )
    {
        if ( Gia_ObjIsCi(pObj) )
            pObj->Value = Gia_ManAppendCi( pNew );
        else if ( Gia_ObjIsAnd(pObj) )
            pObj->Value = Gia_ManHashAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
        else if ( Gia_ObjIsCo(pObj) )
            pObj->Value = Gia_ManAppendCo( pNew, Gia_ManHashAnd(pNew, Gia_ObjFanin0Copy(pObj), iObjValue) );
        if ( i == iObj )
            iObjValue = Abc_LitNotCond(pObj->Value, !Value), pObj->Value = Value;
    }
    pNew = Gia_ManCleanup( pTemp = pNew );
    Gia_ManStop( pTemp );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Reduce bit-width of GIA assuming it is Boolean.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManDupBlock( Gia_Man_t * p, int nBlock )
{
    Gia_Man_t * pNew, * pTemp;
    Gia_Obj_t * pObj; int i;
    assert( Gia_ManCiNum(p) % nBlock == 0 );
    assert( Gia_ManCoNum(p) % nBlock == 0 );
    pNew = Gia_ManStart( Gia_ManObjNum(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    Gia_ManFillValue( p );
    Gia_ManConst0(p)->Value = 0;
    Gia_ManForEachCi( p, pObj, i )
        pObj->Value = (i % nBlock == 0) ? Gia_ManAppendCi(pNew) : 0;
    Gia_ManHashAlloc( pNew );
    Gia_ManForEachAnd( p, pObj, i )
        pObj->Value = Gia_ManHashAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
    Gia_ManForEachCo( p, pObj, i )
        if ( i % nBlock == 0 )
            Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
    Gia_ManSetRegNum( pNew, Gia_ManRegNum(p)/nBlock );
    pNew = Gia_ManCleanup( pTemp = pNew );
    Gia_ManStop( pTemp );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Existentially quantified given variable.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManDupExist( Gia_Man_t * p, int iVar )
{
    Gia_Man_t * pNew, * pTemp;
    Gia_Obj_t * pObj;
    int i;
    assert( iVar >= 0 && iVar < Gia_ManPiNum(p) );
    assert( Gia_ManPoNum(p) == 1 );
    assert( Gia_ManRegNum(p) == 0 );
    Gia_ManFillValue( p );
    // find the cofactoring variable
    pNew = Gia_ManStart( Gia_ManObjNum(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    Gia_ManHashAlloc( pNew );
    // compute negative cofactor
    Gia_ManConst0(p)->Value = 0;
    Gia_ManForEachCi( p, pObj, i )
        pObj->Value = Gia_ManAppendCi(pNew);
    Gia_ManPi( p, iVar )->Value = Abc_Var2Lit( 0, 0 );
    Gia_ManForEachAnd( p, pObj, i )
        pObj->Value = Gia_ManHashAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
    Gia_ManForEachPo( p, pObj, i )
        pObj->Value = Gia_ObjFanin0Copy(pObj);
    // compute the positive cofactor
    Gia_ManPi( p, iVar )->Value = Abc_Var2Lit( 0, 1 );
    Gia_ManForEachAnd( p, pObj, i )
        pObj->Value = Gia_ManHashAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
    // create OR gate
    Gia_ManForEachPo( p, pObj, i )
        pObj->Value = Gia_ManAppendCo( pNew, Gia_ManHashOr(pNew, Gia_ObjFanin0Copy(pObj), pObj->Value) );
    Gia_ManHashStop( pNew );
    pNew = Gia_ManCleanup( pTemp = pNew );
    Gia_ManStop( pTemp );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Existentially quantified given variable.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManDupUniv( Gia_Man_t * p, int iVar )
{
    Gia_Man_t * pNew, * pTemp;
    Gia_Obj_t * pObj;
    int i;
    assert( iVar >= 0 && iVar < Gia_ManPiNum(p) );
    assert( Gia_ManRegNum(p) == 0 );
    Gia_ManFillValue( p );
    // find the cofactoring variable
    pNew = Gia_ManStart( Gia_ManObjNum(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    Gia_ManHashAlloc( pNew );
    // compute negative cofactor
    Gia_ManConst0(p)->Value = 0;
    Gia_ManForEachCi( p, pObj, i )
        pObj->Value = Gia_ManAppendCi(pNew);
    Gia_ManPi( p, iVar )->Value = Abc_Var2Lit( 0, 0 );
    Gia_ManForEachAnd( p, pObj, i )
        pObj->Value = Gia_ManHashAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
    Gia_ManForEachPo( p, pObj, i )
        pObj->Value = Gia_ObjFanin0Copy(pObj);
    // compute the positive cofactor
    Gia_ManPi( p, iVar )->Value = Abc_Var2Lit( 0, 1 );
    Gia_ManForEachAnd( p, pObj, i )
        pObj->Value = Gia_ManHashAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
    // create OR gate
    Gia_ManForEachPo( p, pObj, i )
    {
        if ( i == 0 )
            pObj->Value = Gia_ManAppendCo( pNew, Gia_ManHashAnd(pNew, Gia_ObjFanin0Copy(pObj), pObj->Value) );
        else 
            pObj->Value = Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
    }
    Gia_ManHashStop( pNew );
    pNew = Gia_ManCleanup( pTemp = pNew );
    Gia_ManStop( pTemp );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Existentially quantifies the given variable.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManDupExist2( Gia_Man_t * p, int iVar )
{
    Gia_Man_t * pNew, * pTemp;
    Gia_Obj_t * pObj;
    int i;
    assert( iVar >= 0 && iVar < Gia_ManPiNum(p) );
    pNew = Gia_ManStart( Gia_ManObjNum(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    Gia_ManFillValue( p );
    Gia_ManHashAlloc( pNew );
    Gia_ManConst0(p)->Value = 0;
    Gia_ManForEachCi( p, pObj, i )
        pObj->Value = Gia_ManAppendCi(pNew);
    // first part
    Gia_ManPi( p, iVar )->Value = 0; // modification!
    Gia_ManForEachCo( p, pObj, i )
        Gia_ManDupCofactorVar_rec( pNew, p, Gia_ObjFanin0(pObj) );
    Gia_ManForEachCo( p, pObj, i )
        pObj->Value = Gia_ObjFanin0Copy(pObj);
    // second part
    Gia_ManPi( p, iVar )->Value = 1; // modification!
    Gia_ManForEachAnd( p, pObj, i )
        pObj->Value = ~0;
    Gia_ManForEachCo( p, pObj, i )
        Gia_ManDupCofactorVar_rec( pNew, p, Gia_ObjFanin0(pObj) );
    // combination
    Gia_ManForEachCo( p, pObj, i )
        Gia_ManAppendCo( pNew, Gia_ManHashOr(pNew, Gia_ObjFanin0Copy(pObj), pObj->Value) );
    Gia_ManSetRegNum( pNew, Gia_ManRegNum(p) );
    pNew->nConstrs = p->nConstrs;
    pNew = Gia_ManCleanup( pTemp = pNew );
    Gia_ManStop( pTemp );
    return pNew;
}


/**Function*************************************************************

  Synopsis    [Duplicates AIG in the DFS order while putting CIs first.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManDupDfsSkip( Gia_Man_t * p )
{
    Gia_Man_t * pNew;
    Gia_Obj_t * pObj;
    int i;
    Gia_ManFillValue( p );
    pNew = Gia_ManStart( Gia_ManObjNum(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    Gia_ManConst0(p)->Value = 0;
    Gia_ManForEachCi( p, pObj, i )
        pObj->Value = Gia_ManAppendCi(pNew);
    Gia_ManForEachCo( p, pObj, i )
        if ( pObj->fMark1 == 0 )
            Gia_ManDupDfs_rec( pNew, p, pObj );
    Gia_ManSetRegNum( pNew, Gia_ManRegNum(p) );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Duplicates AIG in the DFS order while putting CIs first.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManDupDfsCone( Gia_Man_t * p, Gia_Obj_t * pRoot )
{
    Gia_Man_t * pNew;
    Gia_Obj_t * pObj;
    int i;
    assert( Gia_ObjIsCo(pRoot) );
    Gia_ManFillValue( p );
    pNew = Gia_ManStart( Gia_ManObjNum(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    Gia_ManConst0(p)->Value = 0;
    Gia_ManForEachCi( p, pObj, i )
        pObj->Value = Gia_ManAppendCi(pNew);
    Gia_ManDupDfs_rec( pNew, p, Gia_ObjFanin0(pRoot) );
    Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pRoot) );
    Gia_ManSetRegNum( pNew, 0 );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Duplicates logic cone of the literal and inserts it back.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManDupConeSupp_rec( Gia_Man_t * pNew, Gia_Man_t * p, Gia_Obj_t * pObj, Vec_Int_t * vObjs )
{
    int iLit0, iLit1, iObj = Gia_ObjId( p, pObj );
    int iLit = Gia_ObjCopyArray( p, iObj );
    if ( iLit >= 0 )
        return;
    assert( Gia_ObjIsAnd(pObj) );
    Gia_ManDupConeSupp_rec( pNew, p, Gia_ObjFanin0(pObj), vObjs );
    Gia_ManDupConeSupp_rec( pNew, p, Gia_ObjFanin1(pObj), vObjs );
    iLit0 = Gia_ObjCopyArray( p, Gia_ObjFaninId0(pObj, iObj) );
    iLit1 = Gia_ObjCopyArray( p, Gia_ObjFaninId1(pObj, iObj) );
    iLit0 = Abc_LitNotCond( iLit0, Gia_ObjFaninC0(pObj) );
    iLit1 = Abc_LitNotCond( iLit1, Gia_ObjFaninC1(pObj) );
    iLit  = Gia_ManAppendAnd( pNew, iLit0, iLit1 );
    Gia_ObjSetCopyArray( p, iObj, iLit );
    Vec_IntPush( vObjs, iObj );
}
Gia_Man_t * Gia_ManDupConeSupp( Gia_Man_t * p, int iLit, Vec_Int_t * vCiIds )
{
    Gia_Man_t * pNew; int i, iLit0;
    Gia_Obj_t * pObj, * pRoot = Gia_ManObj( p, Abc_Lit2Var(iLit) );
    Vec_Int_t * vObjs = Vec_IntAlloc( 1000 );
    //assert( Gia_ObjIsAnd(pRoot) );
    if ( Vec_IntSize(&p->vCopies) < Gia_ManObjNum(p) )
        Vec_IntFillExtra( &p->vCopies, Gia_ManObjNum(p), -1 );
    pNew = Gia_ManStart( Gia_ManObjNum(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    Gia_ManForEachCiVec( vCiIds, p, pObj, i )
        Gia_ObjSetCopyArray( p, Gia_ObjId(p, pObj), Gia_ManAppendCi(pNew) );
    Gia_ManDupConeSupp_rec( pNew, p, pRoot, vObjs );
    iLit0 = Gia_ObjCopyArray( p, Abc_Lit2Var(iLit) );
    iLit0 = Abc_LitNotCond( iLit0, Abc_LitIsCompl(iLit) );
    Gia_ManAppendCo( pNew, iLit0 );
    Gia_ManForEachCiVec( vCiIds, p, pObj, i )
        Gia_ObjSetCopyArray( p, Gia_ObjId(p, pObj), -1 );
    Gia_ManForEachObjVec( vObjs, p, pObj, i )
        Gia_ObjSetCopyArray( p, Gia_ObjId(p, pObj), -1 );
    Vec_IntFree( vObjs );
    //assert( Vec_IntCountLarger(&p->vCopies, -1) == 0 );
    return pNew;
}
void Gia_ManDupConeBack_rec( Gia_Man_t * pNew, Gia_Man_t * p, Gia_Obj_t * pObj )
{
    if ( ~pObj->Value )
        return;
    assert( Gia_ObjIsAnd(pObj) );
    Gia_ManDupConeBack_rec( pNew, p, Gia_ObjFanin0(pObj) );
    Gia_ManDupConeBack_rec( pNew, p, Gia_ObjFanin1(pObj) );
    pObj->Value = Gia_ManHashAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
//    pObj->Value = Gia_ManAppendAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
}
int Gia_ManDupConeBack( Gia_Man_t * p, Gia_Man_t * pNew, Vec_Int_t * vCiIds )
{
    Gia_Obj_t * pObj, * pRoot; int i;
    assert( Gia_ManCiNum(pNew) == Vec_IntSize(vCiIds) );
    Gia_ManFillValue(pNew);
    Gia_ManConst0(pNew)->Value = 0;
    Gia_ManForEachCi( pNew, pObj, i )
        pObj->Value = Gia_Obj2Lit( p, Gia_ManCi(p, Vec_IntEntry(vCiIds, i)) );
    pRoot = Gia_ManCo(pNew, 0);
    Gia_ManDupConeBack_rec( p, pNew, Gia_ObjFanin0(pRoot) );
    return Gia_ObjFanin0Copy(pRoot);
}
int Gia_ManDupConeBackObjs( Gia_Man_t * p, Gia_Man_t * pNew, Vec_Int_t * vObjs )
{
    Gia_Obj_t * pObj, * pRoot; int i;
    assert( Gia_ManCiNum(pNew) == Vec_IntSize(vObjs) );
    Gia_ManFillValue(pNew);
    Gia_ManConst0(pNew)->Value = 0;
    Gia_ManForEachCi( pNew, pObj, i )
        pObj->Value = Abc_Var2Lit( Vec_IntEntry(vObjs, i), 0 );
    pRoot = Gia_ManCo(pNew, 0);
    Gia_ManDupConeBack_rec( p, pNew, Gia_ObjFanin0(pRoot) );
    return Gia_ObjFanin0Copy(pRoot);
}


/**Function*************************************************************

  Synopsis    [Duplicates AIG in the DFS order while putting CIs first.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManDupDfs3_rec( Gia_Man_t * pNew, Gia_Man_t * p, Gia_Obj_t * pObj )
{
    if ( ~pObj->Value )
        return;
    if ( Gia_ObjIsCi(pObj) )
    {
        pObj->Value = Gia_ManAppendCi(pNew);
        return;
    }
    assert( Gia_ObjIsAnd(pObj) );
    Gia_ManDupDfs3_rec( pNew, p, Gia_ObjFanin0(pObj) );
    Gia_ManDupDfs3_rec( pNew, p, Gia_ObjFanin1(pObj) );
    pObj->Value = Gia_ManAppendAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
}
Gia_Man_t * Gia_ManDupDfsNode( Gia_Man_t * p, Gia_Obj_t * pRoot )
{
    Gia_Man_t * pNew;
    assert( Gia_ObjIsAnd(pRoot) );
    Gia_ManFillValue( p );
    pNew = Gia_ManStart( Gia_ManObjNum(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    Gia_ManConst0(p)->Value = 0;
    Gia_ManDupDfs3_rec( pNew, p, pRoot );
    Gia_ManAppendCo( pNew, pRoot->Value );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Duplicates AIG in the DFS order while putting CIs first.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManDupDfsLitArray( Gia_Man_t * p, Vec_Int_t * vLits )
{
    Gia_Man_t * pNew;
    Gia_Obj_t * pObj;
    int i, iLit, iLitRes;
    Gia_ManFillValue( p );
    pNew = Gia_ManStart( Gia_ManObjNum(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    Gia_ManConst0(p)->Value = 0;
    Gia_ManForEachCi( p, pObj, i )
        pObj->Value = Gia_ManAppendCi(pNew);
    Vec_IntForEachEntry( vLits, iLit, i )
    {
        iLitRes = Gia_ManDupDfs2_rec( pNew, p, Gia_ManObj(p, Abc_Lit2Var(iLit)) );
        Gia_ManAppendCo( pNew, Abc_LitNotCond( iLitRes, Abc_LitIsCompl(iLit)) );
    }
    Gia_ManSetRegNum( pNew, 0 );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Returns the array of non-const-0 POs of the dual-output miter.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Gia_ManDupTrimmedNonZero( Gia_Man_t * p )
{
    Vec_Int_t * vNonZero;
    Gia_Man_t * pTemp, * pNonDual;
    Gia_Obj_t * pObj;
    int i;
    assert( (Gia_ManPoNum(p) & 1) == 0 );
    pNonDual = Gia_ManTransformMiter( p );
    pNonDual = Gia_ManSeqStructSweep( pTemp = pNonDual, 1, 1, 0 );
    Gia_ManStop( pTemp );
    assert( Gia_ManPiNum(pNonDual) > 0 );
    assert( 2 * Gia_ManPoNum(pNonDual) == Gia_ManPoNum(p) );
    // skip PO pairs corresponding to const0 POs of the non-dual miter
    vNonZero = Vec_IntAlloc( 100 );
    Gia_ManForEachPo( pNonDual, pObj, i )
        if ( !Gia_ObjIsConst0(Gia_ObjFanin0(pObj)) )
            Vec_IntPush( vNonZero, i );
    Gia_ManStop( pNonDual );
    return vNonZero;
}


/**Function*************************************************************

  Synopsis    [Returns 1 if PO can be removed.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManPoIsToRemove( Gia_Man_t * p, Gia_Obj_t * pObj, int Value )
{
    assert( Gia_ObjIsCo(pObj) );
    if ( Value == -1 )
        return Gia_ObjIsConst0(Gia_ObjFanin0(pObj));
    assert( Value == 0 || Value == 1 );
    return Gia_ObjIsConst0(Gia_ObjFanin0(pObj)) && Value == Gia_ObjFaninC0(pObj);
}

/**Function*************************************************************

  Synopsis    [Duplicates AIG in the DFS order while putting CIs first.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManDupTrimmed( Gia_Man_t * p, int fTrimCis, int fTrimCos, int fDualOut, int OutValue )
{
    Vec_Int_t * vNonZero = NULL;
    Gia_Man_t * pNew, * pTemp;
    Gia_Obj_t * pObj;
    int i, Entry;
    // collect non-zero
    if ( fDualOut && fTrimCos )
        vNonZero = Gia_ManDupTrimmedNonZero( p );
    // start new manager
    pNew = Gia_ManStart( Gia_ManObjNum(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    // check if there are PIs to be added
    Gia_ManCreateRefs( p );
    Gia_ManForEachPi( p, pObj, i )
        if ( !fTrimCis || Gia_ObjRefNum(p, pObj) )
            break;
    if ( i == Gia_ManPiNum(p) ) // there is no PIs - add dummy PI
        Gia_ManAppendCi(pNew);
    // add the ROs
    Gia_ManFillValue( p );
    Gia_ManConst0(p)->Value = 0;
    Gia_ManForEachCi( p, pObj, i )
        if ( !fTrimCis || Gia_ObjRefNum(p, pObj) || Gia_ObjIsRo(p, pObj) )
            pObj->Value = Gia_ManAppendCi(pNew);
    Gia_ManForEachAnd( p, pObj, i )
        pObj->Value = Gia_ManAppendAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
    if ( fDualOut && fTrimCos )
    {
        Vec_IntForEachEntry( vNonZero, Entry, i )
        {
            Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(Gia_ManPo(p, 2*Entry+0)) );
            Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(Gia_ManPo(p, 2*Entry+1)) );
        }
        if ( Gia_ManPoNum(pNew) == 0 ) // nothing - add dummy PO
        {
//            Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(Gia_ManPo(p, 0)) );
//            Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(Gia_ManPo(p, 1)) );
            Gia_ManAppendCo( pNew, 0 );
            Gia_ManAppendCo( pNew, 0 );
        }
        Gia_ManForEachRi( p, pObj, i )
            Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
        Gia_ManSetRegNum( pNew, Gia_ManRegNum(p) );
        // cleanup
        pNew = Gia_ManSeqStructSweep( pTemp = pNew, 1, 1, 0 );
        Gia_ManStop( pTemp );
        // trim the PIs
//        pNew = Gia_ManDupTrimmed( pTemp = pNew, 1, 0, 0 );
//        Gia_ManStop( pTemp );
    }
    else
    {
        // check if there are POs to be added
        Gia_ManForEachPo( p, pObj, i )
            if ( !fTrimCos || !Gia_ManPoIsToRemove(p, pObj, OutValue) )
                break;
        if ( i == Gia_ManPoNum(p) ) // there is no POs - add dummy PO
            Gia_ManAppendCo( pNew, 0 );
        Gia_ManForEachCo( p, pObj, i )
            if ( !fTrimCos || !Gia_ManPoIsToRemove(p, pObj, OutValue) || Gia_ObjIsRi(p, pObj) )
                Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
        Gia_ManSetRegNum( pNew, Gia_ManRegNum(p) );
    }
    Vec_IntFreeP( &vNonZero );
    assert( !Gia_ManHasDangling( pNew ) );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Removes POs driven by PIs and PIs without fanout.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManDupTrimmed2( Gia_Man_t * p )
{
    Gia_Man_t * pNew;
    Gia_Obj_t * pObj;
    int i;
    // start new manager
    pNew = Gia_ManStart( Gia_ManObjNum(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    // check if there are PIs to be added
    Gia_ManCreateRefs( p );
    // discount references of POs
    Gia_ManForEachPo( p, pObj, i )
        Gia_ObjRefFanin0Dec( p, pObj );
    // check if PIs are left
    Gia_ManForEachPi( p, pObj, i )
        if ( Gia_ObjRefNum(p, pObj) )
            break;
    if ( i == Gia_ManPiNum(p) ) // there is no PIs - add dummy PI
        Gia_ManAppendCi(pNew);
    // add the ROs
    Gia_ManFillValue( p );
    Gia_ManConst0(p)->Value = 0;
    Gia_ManForEachCi( p, pObj, i )
        if ( Gia_ObjRefNum(p, pObj) || Gia_ObjIsRo(p, pObj) )
            pObj->Value = Gia_ManAppendCi(pNew);
    Gia_ManForEachAnd( p, pObj, i )
        pObj->Value = Gia_ManAppendAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
    // check if there are POs to be added
    Gia_ManForEachPo( p, pObj, i )
        if ( !Gia_ObjIsConst0(Gia_ObjFanin0(pObj)) && !Gia_ObjIsPi(p, Gia_ObjFanin0(pObj)) )
            break;
    if ( i == Gia_ManPoNum(p) ) // there is no POs - add dummy PO
        Gia_ManAppendCo( pNew, 0 );
    Gia_ManForEachCo( p, pObj, i )
        if ( (!Gia_ObjIsConst0(Gia_ObjFanin0(pObj)) && !Gia_ObjIsPi(p, Gia_ObjFanin0(pObj))) || Gia_ObjIsRi(p, pObj) )
            Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
    Gia_ManSetRegNum( pNew, Gia_ManRegNum(p) );
    assert( !Gia_ManHasDangling( pNew ) );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Duplicates AIG in the DFS order while putting CIs first.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManDupOntop( Gia_Man_t * p, Gia_Man_t * p2 )
{
    Gia_Man_t * pTemp, * pNew;
    Gia_Obj_t * pObj;
    int i;
    assert( Gia_ManPoNum(p) == Gia_ManPiNum(p2) );
    assert( Gia_ManRegNum(p) == 0 );
    assert( Gia_ManRegNum(p2) == 0 );
    pNew = Gia_ManStart( Gia_ManObjNum(p)+Gia_ManObjNum(p2) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    Gia_ManHashAlloc( pNew );
    // dup first AIG
    Gia_ManConst0(p)->Value = 0;
    Gia_ManForEachCi( p, pObj, i )
        pObj->Value = Gia_ManAppendCi(pNew);
    Gia_ManForEachAnd( p, pObj, i )
        pObj->Value = Gia_ManHashAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
    // dup second AIG
    Gia_ManConst0(p2)->Value = 0;
    Gia_ManForEachCo( p, pObj, i )
        Gia_ManPi(p2, i)->Value = Gia_ObjFanin0Copy(pObj);
    Gia_ManForEachAnd( p2, pObj, i )
        pObj->Value = Gia_ManHashAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
    Gia_ManForEachCo( p2, pObj, i )
        pObj->Value = Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
    Gia_ManHashStop( pNew );
//    Gia_ManPrintStats( pGiaNew, 0 );
    pNew = Gia_ManCleanup( pTemp = pNew );
    Gia_ManStop( pTemp );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Duplicates transition relation from p1 and property from p2.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManDupWithNewPo( Gia_Man_t * p1, Gia_Man_t * p2 )
{
    Gia_Man_t * pTemp, * pNew;
    Gia_Obj_t * pObj;
    int i;
    // there is no flops in p2
    assert( Gia_ManRegNum(p2) == 0 );
    // there is only one PO in p2
//    assert( Gia_ManPoNum(p2) == 1 );
    // input count of p2 is equal to flop count of p1 
    assert( Gia_ManPiNum(p2) == Gia_ManRegNum(p1) );

    // start new AIG
    pNew = Gia_ManStart( Gia_ManObjNum(p1)+Gia_ManObjNum(p2) );
    pNew->pName = Abc_UtilStrsav( p1->pName );
    pNew->pSpec = Abc_UtilStrsav( p1->pSpec );
    Gia_ManHashAlloc( pNew );
    // dup first AIG
    Gia_ManConst0(p1)->Value = 0;
    Gia_ManForEachCi( p1, pObj, i )
        pObj->Value = Gia_ManAppendCi(pNew);
    Gia_ManForEachAnd( p1, pObj, i )
        pObj->Value = Gia_ManHashAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
    // dup second AIG
    Gia_ManConst0(p2)->Value = 0;
    Gia_ManForEachPi( p2, pObj, i )
        pObj->Value = Gia_ManRo(p1, i)->Value;
    Gia_ManForEachAnd( p2, pObj, i )
        pObj->Value = Gia_ManHashAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
    // add property output
    Gia_ManForEachPo( p2, pObj, i )
        Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
    // add flop inputs
    Gia_ManForEachRi( p1, pObj, i )
        Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
    Gia_ManHashStop( pNew );
    Gia_ManSetRegNum( pNew, Gia_ManRegNum(p1) );
    pNew = Gia_ManCleanup( pTemp = pNew );
    Gia_ManStop( pTemp );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Print representatives.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManPrintRepr( Gia_Man_t * p )
{
    Gia_Obj_t * pObj;
    int i;
    Gia_ManForEachObj( p, pObj, i )
        if ( ~p->pReprsOld[i] )
            printf( "%d->%d ", i, p->pReprs[i].iRepr );
    printf( "\n" );
}

/**Function*************************************************************

  Synopsis    [Duplicates AIG in the DFS order.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManDupDfsCiMap( Gia_Man_t * p, int * pCi2Lit, Vec_Int_t * vLits )
{
    Gia_Man_t * pNew;
    Gia_Obj_t * pObj;
    int i;
    Gia_ManFillValue( p );
    pNew = Gia_ManStart( Gia_ManObjNum(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    Gia_ManConst0(p)->Value = 0;
    Gia_ManForEachCi( p, pObj, i )
    {
        pObj->Value = Gia_ManAppendCi(pNew);
        if ( ~pCi2Lit[i] )
            pObj->Value = Abc_LitNotCond( Gia_ManObj(p, Abc_Lit2Var(pCi2Lit[i]))->Value, Abc_LitIsCompl(pCi2Lit[i]) );
    }
    Gia_ManHashAlloc( pNew );
    if ( vLits )
    {
        int iLit, iLitRes;
        Vec_IntForEachEntry( vLits, iLit, i )
        {
            iLitRes = Gia_ManDupDfs2_rec( pNew, p, Gia_ManObj(p, Abc_Lit2Var(iLit)) );
            Gia_ManAppendCo( pNew, Abc_LitNotCond( iLitRes, Abc_LitIsCompl(iLit)) );
        }
    }
    else
    {
        Gia_ManForEachCo( p, pObj, i )
        {
            Gia_ManDupDfs2_rec( pNew, p, Gia_ObjFanin0(pObj) );
            Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
        }
    }
    Gia_ManHashStop( pNew );
    Gia_ManSetRegNum( pNew, Gia_ManRegNum(p) );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Permute inputs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManPermuteInputs( Gia_Man_t * p, int nPpis, int nExtra )
{
    Gia_Man_t * pNew;
    Gia_Obj_t * pObj;
    int i;
    pNew = Gia_ManStart( Gia_ManObjNum(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    Gia_ManConst0(p)->Value = 0;
    for ( i = 0; i < Gia_ManPiNum(p) - nPpis - nExtra; i++ ) // regular PIs
        Gia_ManCi(p, i)->Value = Gia_ManAppendCi( pNew );
    for ( i = Gia_ManPiNum(p) - nExtra; i < Gia_ManPiNum(p); i++ ) // extra PIs due to DC values
        Gia_ManCi(p, i)->Value = Gia_ManAppendCi( pNew );
    for ( i = Gia_ManPiNum(p) - nPpis - nExtra; i < Gia_ManPiNum(p) - nExtra; i++ ) // pseudo-PIs
        Gia_ManCi(p, i)->Value = Gia_ManAppendCi( pNew );
    for ( i = Gia_ManPiNum(p); i < Gia_ManCiNum(p); i++ ) // flop outputs
        Gia_ManCi(p, i)->Value = Gia_ManAppendCi( pNew );
    assert( Gia_ManCiNum(pNew) == Gia_ManCiNum(p) );
    Gia_ManForEachAnd( p, pObj, i )
        pObj->Value = Gia_ManAppendAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
    Gia_ManForEachCo( p, pObj, i )
        pObj->Value = Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
    Gia_ManSetRegNum( pNew, Gia_ManRegNum(p) );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Duplicates AIG in the DFS order.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManDupDfsClasses( Gia_Man_t * p )
{
    Gia_Man_t * pNew, * pTemp;
    Gia_Obj_t * pObj;
    int i;
    assert( p->pReprsOld != NULL );
    Gia_ManFillValue( p );
    pNew = Gia_ManStart( Gia_ManObjNum(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    Gia_ManConst0(p)->Value = 0;
    Gia_ManForEachCi( p, pObj, i )
        pObj->Value = Gia_ManAppendCi(pNew);
    Gia_ManHashAlloc( pNew );
    Gia_ManForEachCo( p, pObj, i )
        Gia_ManDupDfs_rec( pNew, p, pObj );
    Gia_ManHashStop( pNew );
    Gia_ManSetRegNum( pNew, Gia_ManRegNum(p) );
    pNew = Gia_ManCleanup( pTemp = pNew );
    Gia_ManStop( pTemp );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Detect topmost gate.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManDupTopAnd_iter( Gia_Man_t * p, int fVerbose )  
{
    Gia_Man_t * pNew;
    Gia_Obj_t * pObj;
    Vec_Int_t * vFront, * vLeaves;
    int i, iLit, iObjId, nCiLits, * pCi2Lit;
    char * pVar2Val;
    // collect the frontier
    vFront = Vec_IntAlloc( 1000 );
    vLeaves = Vec_IntAlloc( 1000 );
    Gia_ManForEachCo( p, pObj, i )
    {
        if ( Gia_ObjIsConst0( Gia_ObjFanin0(pObj) ) )
            continue;
        if ( Gia_ObjFaninC0(pObj) )
            Vec_IntPush( vLeaves, Gia_ObjFaninLit0p(p, pObj) );
        else
            Vec_IntPush( vFront, Gia_ObjFaninId0p(p, pObj) );
    }
    if ( Vec_IntSize(vFront) == 0 )
    {
        if ( fVerbose )
            printf( "The AIG cannot be decomposed using AND-decomposition.\n" );
        Vec_IntFree( vFront );
        Vec_IntFree( vLeaves );
        return Gia_ManDupNormalize( p, 0 );
    }
    // expand the frontier
    Gia_ManForEachObjVec( vFront, p, pObj, i )
    {
        if ( Gia_ObjIsCi(pObj) )
        {
            Vec_IntPush( vLeaves, Abc_Var2Lit( Gia_ObjId(p, pObj), 0 ) );
            continue;
        }
        assert( Gia_ObjIsAnd(pObj) );
        if ( Gia_ObjFaninC0(pObj) )
            Vec_IntPush( vLeaves, Gia_ObjFaninLit0p(p, pObj) );
        else
            Vec_IntPush( vFront, Gia_ObjFaninId0p(p, pObj) );
        if ( Gia_ObjFaninC1(pObj) )
            Vec_IntPush( vLeaves, Gia_ObjFaninLit1p(p, pObj) );
        else
            Vec_IntPush( vFront, Gia_ObjFaninId1p(p, pObj) );
    }
    Vec_IntFree( vFront );
    // sort the literals
    nCiLits = 0;
    pCi2Lit = ABC_FALLOC( int, Gia_ManObjNum(p) );
    pVar2Val = ABC_FALLOC( char, Gia_ManObjNum(p) );
    Vec_IntForEachEntry( vLeaves, iLit, i )
    {
        iObjId = Abc_Lit2Var(iLit);
        pObj = Gia_ManObj(p, iObjId);
        if ( Gia_ObjIsCi(pObj) )
        {
            pCi2Lit[Gia_ObjCioId(pObj)] = !Abc_LitIsCompl(iLit);
            nCiLits++;
        }
        if ( pVar2Val[iObjId] != 0 && pVar2Val[iObjId] != 1 )
            pVar2Val[iObjId] = Abc_LitIsCompl(iLit);
        else if ( pVar2Val[iObjId] != Abc_LitIsCompl(iLit) )
            break;
    }
    if ( i < Vec_IntSize(vLeaves) )
    {
        printf( "Problem is trivially UNSAT.\n" );
        ABC_FREE( pCi2Lit );
        ABC_FREE( pVar2Val );
        Vec_IntFree( vLeaves );
        return Gia_ManDupNormalize( p, 0 );
    }
    // create array of input literals
    Vec_IntClear( vLeaves );
    Gia_ManForEachObj( p, pObj, i )
        if ( !Gia_ObjIsCi(pObj) && (pVar2Val[i] == 0 || pVar2Val[i] == 1) )
            Vec_IntPush( vLeaves, Abc_Var2Lit(i, pVar2Val[i]) );
    if ( fVerbose )
        printf( "Detected %6d AND leaves and %6d CI leaves.\n", Vec_IntSize(vLeaves), nCiLits );
    // create the input map
    if ( nCiLits == 0 )
        pNew = Gia_ManDupDfsLitArray( p, vLeaves );
    else
        pNew = Gia_ManDupDfsCiMap( p, pCi2Lit, vLeaves );
    ABC_FREE( pCi2Lit );
    ABC_FREE( pVar2Val );
    Vec_IntFree( vLeaves );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Detect topmost gate.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManDupTopAnd( Gia_Man_t * p, int fVerbose )  
{
    Gia_Man_t * pNew, * pTemp;
    int fContinue, iIter = 0;
    pNew = Gia_ManDupNormalize( p, 0 );
    for ( fContinue = 1; fContinue; )
    {
        pNew = Gia_ManDupTopAnd_iter( pTemp = pNew, fVerbose );
        if ( Gia_ManCoNum(pNew) == Gia_ManCoNum(pTemp) && Gia_ManAndNum(pNew) == Gia_ManAndNum(pTemp) )
            fContinue = 0;
        Gia_ManStop( pTemp );
        if ( fVerbose )
        {
            printf( "Iter %2d : ", ++iIter );
            Gia_ManPrintStatsShort( pNew );
        }
    }
    return pNew;
}


/**Function*************************************************************

  Synopsis    [Duplicates the AIG in the DFS order.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManMiter_rec( Gia_Man_t * pNew, Gia_Man_t * p, Gia_Obj_t * pObj )
{
    if ( ~pObj->Value )
        return pObj->Value;
    assert( Gia_ObjIsAnd(pObj) );
    Gia_ManMiter_rec( pNew, p, Gia_ObjFanin0(pObj) );
    Gia_ManMiter_rec( pNew, p, Gia_ObjFanin1(pObj) );
    return pObj->Value = Gia_ManHashAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
}

/**Function*************************************************************

  Synopsis    [Creates miter of two designs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManMiter( Gia_Man_t * p0, Gia_Man_t * p1, int nInsDup, int fDualOut, int fSeq, int fImplic, int fVerbose )
{
    Gia_Man_t * pNew, * pTemp;
    Gia_Obj_t * pObj;
    int i, iLit;
    if ( fSeq )
    {
        if ( Gia_ManPiNum(p0) != Gia_ManPiNum(p1) )
        {
            printf( "Gia_ManMiter(): Designs have different number of PIs.\n" );
            return NULL;
        }
        if ( Gia_ManPoNum(p0) != Gia_ManPoNum(p1) )
        {
            printf( "Gia_ManMiter(): Designs have different number of POs.\n" );
            return NULL;
        }
        if ( Gia_ManRegNum(p0) == 0 || Gia_ManRegNum(p1) == 0 )
        {
            printf( "Gia_ManMiter(): At least one of the designs has no registers.\n" );
            return NULL;
        }
    }
    else
    {
        if ( Gia_ManCiNum(p0) != Gia_ManCiNum(p1) )
        {
            printf( "Gia_ManMiter(): Designs have different number of CIs.\n" );
            return NULL;
        }
        if ( Gia_ManCoNum(p0) != Gia_ManCoNum(p1) )
        {
            printf( "Gia_ManMiter(): Designs have different number of COs.\n" );
            return NULL;
        }
    }
    // start the manager
    pNew = Gia_ManStart( Gia_ManObjNum(p0) + Gia_ManObjNum(p1) );
    pNew->pName = Abc_UtilStrsav( "miter" );
    // map combinational inputs
    Gia_ManFillValue( p0 );
    Gia_ManFillValue( p1 );
    Gia_ManConst0(p0)->Value = 0;
    Gia_ManConst0(p1)->Value = 0;
    // map internal nodes and outputs
    Gia_ManHashAlloc( pNew );
    if ( fSeq )
    {
        // create primary inputs
        Gia_ManForEachPi( p0, pObj, i )
            pObj->Value = Gia_ManAppendCi( pNew );
        Gia_ManForEachPi( p1, pObj, i )
            if ( i < Gia_ManPiNum(p1) - nInsDup )
                pObj->Value = Gia_ObjToLit( pNew, Gia_ManPi(pNew, i) );
            else
                pObj->Value = Gia_ManAppendCi( pNew );
        // create latch outputs
        Gia_ManForEachRo( p0, pObj, i )
            pObj->Value = Gia_ManAppendCi( pNew );
        Gia_ManForEachRo( p1, pObj, i )
            pObj->Value = Gia_ManAppendCi( pNew );
        // create primary outputs
        Gia_ManForEachPo( p0, pObj, i )
        {
            Gia_ManMiter_rec( pNew, p0, Gia_ObjFanin0(pObj) );
            Gia_ManMiter_rec( pNew, p1, Gia_ObjFanin0(Gia_ManPo(p1,i)) );
            if ( fDualOut )
            {
                Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
                Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(Gia_ManPo(p1,i)) );
            }
            else if ( fImplic )
            {
                iLit = Gia_ManHashAnd( pNew, Gia_ObjFanin0Copy(pObj), Abc_LitNot(Gia_ObjFanin0Copy(Gia_ManPo(p1,i))) );
                Gia_ManAppendCo( pNew, iLit );
            }
            else 
            {
                iLit = Gia_ManHashXor( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin0Copy(Gia_ManPo(p1,i)) );
                Gia_ManAppendCo( pNew, iLit );
            }
        }
        // create register inputs
        Gia_ManForEachRi( p0, pObj, i )
        {
            Gia_ManMiter_rec( pNew, p0, Gia_ObjFanin0(pObj) );
            pObj->Value = Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
        }
        Gia_ManForEachRi( p1, pObj, i )
        {
            Gia_ManMiter_rec( pNew, p1, Gia_ObjFanin0(pObj) );
            pObj->Value = Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
        }
        Gia_ManSetRegNum( pNew, Gia_ManRegNum(p0) + Gia_ManRegNum(p1) );
    }
    else
    {
        // create combinational inputs
        Gia_ManForEachCi( p0, pObj, i )
            pObj->Value = Gia_ManAppendCi( pNew );
        Gia_ManForEachCi( p1, pObj, i )
            if ( i < Gia_ManCiNum(p1) - nInsDup )
                pObj->Value = Gia_ObjToLit( pNew, Gia_ManCi(pNew, i) );
            else
                pObj->Value = Gia_ManAppendCi( pNew );
        // create combinational outputs
        Gia_ManForEachCo( p0, pObj, i )
        {
            Gia_ManMiter_rec( pNew, p0, Gia_ObjFanin0(pObj) );
            Gia_ManMiter_rec( pNew, p1, Gia_ObjFanin0(Gia_ManCo(p1,i)) );
            if ( fDualOut )
            {
                Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
                Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(Gia_ManCo(p1,i)) );
            }
            else if ( fImplic )
            {
                iLit = Gia_ManHashAnd( pNew, Gia_ObjFanin0Copy(pObj), Abc_LitNot(Gia_ObjFanin0Copy(Gia_ManPo(p1,i))) );
                Gia_ManAppendCo( pNew, iLit );
            }
            else
            {
                iLit = Gia_ManHashXor( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin0Copy(Gia_ManCo(p1,i)) );
                Gia_ManAppendCo( pNew, iLit );
            }
        }
    }
    Gia_ManHashStop( pNew );
    pNew = Gia_ManCleanup( pTemp = pNew );
    Gia_ManStop( pTemp );

    pNew = Gia_ManDupNormalize( pTemp = pNew, 0 );
    Gia_ManStop( pTemp );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Computes the AND of all POs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManDupAndOr( Gia_Man_t * p, int nOuts, int fUseOr, int fCompl )
{
    Gia_Man_t * pNew, * pTemp;
    Gia_Obj_t * pObj;
    int i, iResult;
    assert( Gia_ManRegNum(p) == 0 );
    pNew = Gia_ManStart( Gia_ManObjNum(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    Gia_ManConst0(p)->Value = 0;
    Gia_ManHashAlloc( pNew );
    Gia_ManForEachPi( p, pObj, i )
        pObj->Value = Gia_ManAppendCi( pNew );
    Gia_ManForEachAnd( p, pObj, i )
        pObj->Value = Gia_ManHashAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
    if ( fUseOr ) // construct OR of all POs
    {
        iResult = 0;
        Gia_ManForEachPo( p, pObj, i )
            iResult = Gia_ManHashOr( pNew, iResult, Gia_ObjFanin0Copy(pObj) );
    }
    else // construct AND of all POs
    {
        iResult = 1;
        Gia_ManForEachPo( p, pObj, i )
            iResult = Gia_ManHashAnd( pNew, iResult, Gia_ObjFanin0Copy(pObj) );
    }
    iResult = Abc_LitNotCond( iResult, (int)(fCompl > 0) );
//    Gia_ManForEachPo( p, pObj, i )
//        pObj->Value = Gia_ManAppendCo( pNew, iResult );
    for ( i = 0; i < nOuts; i++ )
        Gia_ManAppendCo( pNew, iResult );
    Gia_ManHashStop( pNew );
    Gia_ManSetRegNum( pNew, Gia_ManRegNum(p) );
    pNew = Gia_ManCleanup( pTemp = pNew );
    Gia_ManStop( pTemp );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Transforms the circuit into a regular miter.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManTransformMiter( Gia_Man_t * p )
{
    Gia_Man_t * pNew, * pTemp;
    Gia_Obj_t * pObj, * pObj2;
    int i, iLit;
    assert( (Gia_ManPoNum(p) & 1) == 0 );
    pNew = Gia_ManStart( Gia_ManObjNum(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    Gia_ManConst0(p)->Value = 0;
    Gia_ManHashAlloc( pNew );
    Gia_ManForEachCi( p, pObj, i )
        pObj->Value = Gia_ManAppendCi( pNew );
    Gia_ManForEachAnd( p, pObj, i )
        pObj->Value = Gia_ManHashAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
    Gia_ManForEachPo( p, pObj, i )
    {
        pObj2 = Gia_ManPo( p, ++i );
        iLit = Gia_ManHashXor( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin0Copy(pObj2) );
        Gia_ManAppendCo( pNew, iLit );
    }
    Gia_ManForEachRi( p, pObj, i )
        pObj->Value = Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
    Gia_ManHashStop( pNew );
    Gia_ManSetRegNum( pNew, Gia_ManRegNum(p) );
    pNew = Gia_ManCleanup( pTemp = pNew );
    Gia_ManStop( pTemp );
    return pNew;
}
Gia_Man_t * Gia_ManTransformMiter2( Gia_Man_t * p )
{
    Gia_Man_t * pNew, * pTemp;
    Gia_Obj_t * pObj, * pObj2;
    int i, iLit, nPart = Gia_ManPoNum(p)/2;
    assert( (Gia_ManPoNum(p) & 1) == 0 );
    pNew = Gia_ManStart( Gia_ManObjNum(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    Gia_ManConst0(p)->Value = 0;
    Gia_ManHashAlloc( pNew );
    Gia_ManForEachCi( p, pObj, i )
        pObj->Value = Gia_ManAppendCi( pNew );
    Gia_ManForEachAnd( p, pObj, i )
        pObj->Value = Gia_ManHashAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
    Gia_ManForEachPo( p, pObj, i )
    {
        if ( i == nPart )
            break;
        pObj2 = Gia_ManPo( p, nPart + i );
        iLit = Gia_ManHashXor( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin0Copy(pObj2) );
        Gia_ManAppendCo( pNew, iLit );
    }
    Gia_ManForEachRi( p, pObj, i )
        pObj->Value = Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
    Gia_ManHashStop( pNew );
    Gia_ManSetRegNum( pNew, Gia_ManRegNum(p) );
    pNew = Gia_ManCleanup( pTemp = pNew );
    Gia_ManStop( pTemp );
    return pNew;
}
Gia_Man_t * Gia_ManTransformToDual( Gia_Man_t * p )
{
    Gia_Man_t * pNew;
    Gia_Obj_t * pObj;
    int i;
    pNew = Gia_ManStart( Gia_ManObjNum(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    Gia_ManConst0(p)->Value = 0;
    Gia_ManHashAlloc( pNew );
    Gia_ManForEachCi( p, pObj, i )
        pObj->Value = Gia_ManAppendCi( pNew );
    Gia_ManForEachAnd( p, pObj, i )
        pObj->Value = Gia_ManHashAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
    Gia_ManForEachPo( p, pObj, i )
    {
        Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
        Gia_ManAppendCo( pNew, 0 );
    }
    Gia_ManForEachRi( p, pObj, i )
        pObj->Value = Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
    Gia_ManHashStop( pNew );
    Gia_ManSetRegNum( pNew, Gia_ManRegNum(p) );
    return pNew;
}
Gia_Man_t * Gia_ManTransformTwoWord2DualOutput( Gia_Man_t * p )
{
    Gia_Man_t * pNew, * pTemp;
    Gia_Obj_t * pObj, * pObj2;
    int i, nPart = Gia_ManPoNum(p)/2;
    assert( (Gia_ManPoNum(p) & 1) == 0 );
    pNew = Gia_ManStart( Gia_ManObjNum(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    Gia_ManConst0(p)->Value = 0;
    Gia_ManHashAlloc( pNew );
    Gia_ManForEachCi( p, pObj, i )
        pObj->Value = Gia_ManAppendCi( pNew );
    Gia_ManForEachAnd( p, pObj, i )
        pObj->Value = Gia_ManHashAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
    Gia_ManForEachPo( p, pObj, i )
    {
        if ( i == nPart )
            break;
        pObj2 = Gia_ManPo( p, nPart + i );
        Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
        Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj2) );
    }
    Gia_ManForEachRi( p, pObj, i )
        pObj->Value = Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
    Gia_ManHashStop( pNew );
    Gia_ManSetRegNum( pNew, Gia_ManRegNum(p) );
    pNew = Gia_ManCleanup( pTemp = pNew );
    Gia_ManStop( pTemp );
    return pNew;
}

void Gia_ManCollectOneSide_rec( Gia_Man_t * p, Gia_Obj_t * pObj, Vec_Int_t * vNodes )
{
    if ( Gia_ObjIsTravIdCurrent(p, pObj) )
        return;
    Gia_ObjSetTravIdCurrent(p, pObj);
    if ( !Gia_ObjIsAnd(pObj) )
        return;
    Gia_ManCollectOneSide_rec( p, Gia_ObjFanin0(pObj), vNodes );
    Gia_ManCollectOneSide_rec( p, Gia_ObjFanin1(pObj), vNodes );
    Vec_IntPush( vNodes, Gia_ObjId(p, pObj) );
}
Vec_Int_t * Gia_ManCollectOneSide( Gia_Man_t * p, int iSide )
{
    Gia_Obj_t * pObj; int i;
    Vec_Int_t * vNodes = Vec_IntAlloc( Gia_ManAndNum(p) );
    Gia_ManIncrementTravId( p );
    Gia_ManForEachPo( p, pObj, i )
        if ( (i & 1) == iSide )
            Gia_ManCollectOneSide_rec( p, Gia_ObjFanin0(pObj), vNodes );
    return vNodes;
}
Gia_Man_t * Gia_ManTransformDualOutput( Gia_Man_t * p )
{
    Vec_Int_t * vNodes0 = Gia_ManCollectOneSide( p, 0 );
    Vec_Int_t * vNodes1 = Gia_ManCollectOneSide( p, 1 );
    Gia_Man_t * pNew, * pTemp;
    Gia_Obj_t * pObj, * pObj2;
    int i, fSwap = 0;
    assert( Gia_ManRegNum(p) == 0 );
    assert( (Gia_ManPoNum(p) & 1) == 0 );
    if ( Vec_IntSize(vNodes0) > Vec_IntSize(vNodes1) )
    {
        ABC_SWAP( Vec_Int_t *, vNodes0, vNodes1 );
        fSwap = 1;
    }
    assert( Vec_IntSize(vNodes0) <= Vec_IntSize(vNodes1) );
    pNew = Gia_ManStart( Gia_ManObjNum(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    Gia_ManConst0(p)->Value = 0;
    Gia_ManHashAlloc( pNew );
    Gia_ManForEachCi( p, pObj, i )
        pObj->Value = Gia_ManAppendCi( pNew );
    Gia_ManForEachObjVec( vNodes0, p, pObj, i )
        pObj->Value = Gia_ManHashAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
    Gia_ManForEachObjVec( vNodes1, p, pObj, i )
        pObj->Value = Gia_ManHashAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
    Vec_IntFree( vNodes0 );
    Vec_IntFree( vNodes1 );
    Gia_ManForEachPo( p, pObj, i )
    {
        pObj2 = Gia_ManPo( p, i^fSwap );
        pObj->Value = Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj2) );
    }
    Gia_ManHashStop( pNew );
    Gia_ManSetRegNum( pNew, Gia_ManRegNum(p) );
    pNew = Gia_ManCleanup( pTemp = pNew );
    Gia_ManStop( pTemp );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Performs 'zero' and 'undc' operation.]

  Description [The init string specifies 0/1/X for each flop.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManDupZeroUndc( Gia_Man_t * p, char * pInit, int fGiaSimple, int fVerbose )
{
    Gia_Man_t * pNew;
    Gia_Obj_t * pObj;
    int CountPis = Gia_ManPiNum(p), * pPiLits;
    int i, iResetFlop = -1, Count1 = 0;
    //printf( "Using %s\n", pInit );
    // map X-valued flops into new PIs
    assert( (int)strlen(pInit) == Gia_ManRegNum(p) );
    pPiLits = ABC_FALLOC( int, Gia_ManRegNum(p) );
    for ( i = 0; i < Gia_ManRegNum(p); i++ )
        if ( pInit[i] == 'x' || pInit[i] == 'X' )
            pPiLits[i] = CountPis++;
    // create new manager
    pNew = Gia_ManStart( Gia_ManObjNum(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    pNew->fGiaSimple = fGiaSimple;
    Gia_ManConst0(p)->Value = 0;
    // create primary inputs
    Gia_ManForEachPi( p, pObj, i )
        pObj->Value = Gia_ManAppendCi( pNew );
    // create additional primary inputs
    for ( i = Gia_ManPiNum(p); i < CountPis; i++ )
        Gia_ManAppendCi( pNew );
    // create flop outputs
    Gia_ManForEachRo( p, pObj, i )
        pObj->Value = Gia_ManAppendCi( pNew );
    // create reset flop output
    if ( CountPis > Gia_ManPiNum(p) )
        iResetFlop = Gia_ManAppendCi( pNew );
    // update flop outputs
    Gia_ManMarkFanoutDrivers( p );
    Gia_ManForEachRo( p, pObj, i )
    {
        if ( pInit[i] == '1' )
            pObj->Value = Abc_LitNot(pObj->Value), Count1++;
        else if ( pInit[i] == 'x' || pInit[i] == 'X' )
        {
            if ( pObj->fMark0 ) // only add MUX if the flop has fanout
                pObj->Value = Gia_ManAppendMux( pNew, iResetFlop, pObj->Value, Gia_Obj2Lit(pNew, Gia_ManPi(pNew, pPiLits[i])) );
        }
        else if ( pInit[i] != '0' )
            assert( 0 );
    }
    Gia_ManCleanMark0( p );
    ABC_FREE( pPiLits );
    // build internal nodes
    Gia_ManForEachAnd( p, pObj, i )
        pObj->Value = Gia_ManAppendAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
    // create POs
    Gia_ManForEachPo( p, pObj, i )
        pObj->Value = Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
    // create flop inputs
    Gia_ManForEachRi( p, pObj, i )
        if ( pInit[i] == '1' )
            pObj->Value = Gia_ManAppendCo( pNew, Abc_LitNot(Gia_ObjFanin0Copy(pObj)) );
        else
            pObj->Value = Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
    // create reset flop input
    if ( CountPis > Gia_ManPiNum(p) )
        Gia_ManAppendCo( pNew, 1 );
    Gia_ManSetRegNum( pNew, Gia_ManRegNum(p) + (int)(CountPis > Gia_ManPiNum(p)) );
    if ( fVerbose )
        printf( "Converted %d 1-valued FFs and %d DC-valued FFs.\n", Count1, CountPis-Gia_ManPiNum(p) );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Creates miter of two designs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManMiter2( Gia_Man_t * pStart, char * pInit, int fVerbose )
{
    Vec_Int_t * vCiValues, * vCoValues0, * vCoValues1;
    Gia_Man_t * pNew, * pUndc, * pTemp;
    Gia_Obj_t * pObj;
    char * pInitNew;
    int i, k;
    // check PI values
    for ( i = 0; i < Gia_ManPiNum(pStart); i++ )
        assert( pInit[i] == 'x' || pInit[i] == 'X' );
    // normalize the manager
    pUndc = Gia_ManDupZeroUndc( pStart, pInit + Gia_ManPiNum(pStart), 0, fVerbose );
    // create new init string
    pInitNew = ABC_ALLOC( char, Gia_ManPiNum(pUndc)+1 );
    for ( i = 0; i < Gia_ManPiNum(pStart); i++ )
        pInitNew[i] = pInit[i];
    for ( i = k = Gia_ManPiNum(pStart); i < Gia_ManCiNum(pStart); i++ )
        if ( pInit[i] == 'x' || pInit[i] == 'X' )
            pInitNew[k++] = pInit[i];
    pInitNew[k] = 0;
    assert( k == Gia_ManPiNum(pUndc) );
    // derive miter
    pNew = Gia_ManStart( Gia_ManObjNum(pUndc) );
    pNew->pName = Abc_UtilStrsav( pUndc->pName );
    pNew->pSpec = Abc_UtilStrsav( pUndc->pSpec );
    Gia_ManConst0(pUndc)->Value = 0;
    Gia_ManHashAlloc( pNew );
    // add PIs of the first side
    Gia_ManForEachPi( pUndc, pObj, i )
        pObj->Value = Gia_ManAppendCi( pNew );
    // add PIs of the second side
    vCiValues = Vec_IntAlloc( Gia_ManPiNum(pUndc) );
    Gia_ManForEachPi( pUndc, pObj, i )
        if ( pInitNew[i] == 'x' )
            Vec_IntPush( vCiValues, Gia_Obj2Lit( pNew, Gia_ManPi(pNew, i) ) );
        else if ( pInitNew[i] == 'X' ) 
            Vec_IntPush( vCiValues, Gia_ManAppendCi( pNew ) );
        else assert( 0 );
    // build flops and internal nodes
    Gia_ManForEachRo( pUndc, pObj, i )
        pObj->Value = Gia_ManAppendCi( pNew );
    Gia_ManForEachAnd( pUndc, pObj, i )
        pObj->Value = Gia_ManHashAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
    // collect CO values
    vCoValues0 = Vec_IntAlloc( Gia_ManPoNum(pUndc) );
    Gia_ManForEachCo( pUndc, pObj, i )
        Vec_IntPush( vCoValues0, Gia_ObjFanin0Copy(pObj) );
    // build the other side
    Gia_ManForEachPi( pUndc, pObj, i )
        pObj->Value = Vec_IntEntry( vCiValues, i );
    Gia_ManForEachRo( pUndc, pObj, i )
        pObj->Value = Gia_ManAppendCi( pNew );
    Gia_ManForEachAnd( pUndc, pObj, i )
        pObj->Value = Gia_ManHashAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
    // collect CO values
    vCoValues1 = Vec_IntAlloc( Gia_ManPoNum(pUndc) );
    Gia_ManForEachCo( pUndc, pObj, i )
        Vec_IntPush( vCoValues1, Gia_ObjFanin0Copy(pObj) );
    // create POs
    Gia_ManForEachPo( pUndc, pObj, i )
        pObj->Value = Gia_ManAppendCo( pNew, Gia_ManHashXor( pNew, Vec_IntEntry(vCoValues0, i), Vec_IntEntry(vCoValues1, i) ) );
    // create flop inputs
    Gia_ManForEachRi( pUndc, pObj, i )
        pObj->Value = Gia_ManAppendCo( pNew, Vec_IntEntry(vCoValues0, Gia_ManPoNum(pUndc)+i) );
    Gia_ManForEachRi( pUndc, pObj, i )
        pObj->Value = Gia_ManAppendCo( pNew, Vec_IntEntry(vCoValues1, Gia_ManPoNum(pUndc)+i) );
    Vec_IntFree( vCoValues0 );
    Vec_IntFree( vCoValues1 );
    Vec_IntFree( vCiValues );
    ABC_FREE( pInitNew );
    // cleanup
    Gia_ManHashStop( pNew );
    Gia_ManSetRegNum( pNew, 2*Gia_ManRegNum(pUndc) );
    Gia_ManStop( pUndc );
    pNew = Gia_ManCleanup( pTemp = pNew );
    Gia_ManStop( pTemp );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Duplicates the AIG in the DFS order.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManChoiceMiter_rec( Gia_Man_t * pNew, Gia_Man_t * p, Gia_Obj_t * pObj )
{
    if ( ~pObj->Value )
        return pObj->Value;
    Gia_ManChoiceMiter_rec( pNew, p, Gia_ObjFanin0(pObj) );
    if ( Gia_ObjIsCo(pObj) )
        return pObj->Value = Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
    Gia_ManChoiceMiter_rec( pNew, p, Gia_ObjFanin1(pObj) );
    return pObj->Value = Gia_ManHashAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
} 

/**Function*************************************************************

  Synopsis    [Derives the miter of several AIGs for choice computation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManChoiceMiter( Vec_Ptr_t * vGias )
{
    Gia_Man_t * pNew, * pGia, * pGia0;
    int i, k, iNode, nNodes;
    // make sure they have equal parameters
    assert( Vec_PtrSize(vGias) > 0 );
    pGia0 = (Gia_Man_t *)Vec_PtrEntry( vGias, 0 );
    Vec_PtrForEachEntry( Gia_Man_t *, vGias, pGia, i )
    {
        assert( Gia_ManCiNum(pGia)  == Gia_ManCiNum(pGia0) );
        assert( Gia_ManCoNum(pGia)  == Gia_ManCoNum(pGia0) );
        assert( Gia_ManRegNum(pGia) == Gia_ManRegNum(pGia0) );
        Gia_ManFillValue( pGia );
        Gia_ManConst0(pGia)->Value = 0;
    }
    // start the new manager
    pNew = Gia_ManStart( Vec_PtrSize(vGias) * Gia_ManObjNum(pGia0) );
    pNew->pName = Abc_UtilStrsav( pGia0->pName );
    pNew->pSpec = Abc_UtilStrsav( pGia0->pSpec );
    // create new CIs and assign them to the old manager CIs
    for ( k = 0; k < Gia_ManCiNum(pGia0); k++ )
    {
        iNode = Gia_ManAppendCi(pNew);
        Vec_PtrForEachEntry( Gia_Man_t *, vGias, pGia, i )
            Gia_ManCi( pGia, k )->Value = iNode; 
    }
    // create internal nodes
    Gia_ManHashAlloc( pNew );
    for ( k = 0; k < Gia_ManCoNum(pGia0); k++ )
    {
        Vec_PtrForEachEntry( Gia_Man_t *, vGias, pGia, i )
            Gia_ManChoiceMiter_rec( pNew, pGia, Gia_ManCo( pGia, k ) );
    }
    Gia_ManHashStop( pNew );
    // check the presence of dangling nodes
    nNodes = Gia_ManHasDangling( pNew );
    //assert( nNodes == 0 );
    // finalize
    Gia_ManSetRegNum( pNew, Gia_ManRegNum(pGia0) );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Duplicates AIG while putting first PIs, then nodes, then POs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManDupWithConstraints( Gia_Man_t * p, Vec_Int_t * vPoTypes )
{
    Gia_Man_t * pNew;
    Gia_Obj_t * pObj;
    int i, nConstr = 0;
    pNew = Gia_ManStart( Gia_ManObjNum(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    Gia_ManConst0(p)->Value = 0;
    Gia_ManForEachCi( p, pObj, i )
        pObj->Value = Gia_ManAppendCi(pNew);
    Gia_ManForEachAnd( p, pObj, i )
        pObj->Value = Gia_ManAppendAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
    Gia_ManForEachPo( p, pObj, i )
        if ( Vec_IntEntry(vPoTypes, i) == 0 ) // regular PO
            pObj->Value = Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
    Gia_ManForEachPo( p, pObj, i )
        if ( Vec_IntEntry(vPoTypes, i) == 1 ) // constraint (should be complemented!)
            pObj->Value = Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) ^ 1 ), nConstr++;
    Gia_ManForEachRi( p, pObj, i )
        pObj->Value = Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
//    Gia_ManDupRemapEquiv( pNew, p );
    Gia_ManSetRegNum( pNew, Gia_ManRegNum(p) );
    pNew->nConstrs = nConstr;
    assert( Gia_ManIsNormalized(pNew) );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Copy an AIG structure related to the selected POs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ObjCompareByCioId( Gia_Obj_t ** pp1, Gia_Obj_t ** pp2 )
{
    Gia_Obj_t * pObj1 = *pp1;
    Gia_Obj_t * pObj2 = *pp2;
    return Gia_ObjCioId(pObj1) - Gia_ObjCioId(pObj2);
}
void Gia_ManDupCones_rec( Gia_Man_t * p, Gia_Obj_t * pObj, Vec_Ptr_t * vLeaves, Vec_Ptr_t * vNodes, Vec_Ptr_t * vRoots )
{
    if ( Gia_ObjIsTravIdCurrent(p, pObj) )
        return;
    Gia_ObjSetTravIdCurrent(p, pObj);
    if ( Gia_ObjIsAnd(pObj) )
    {
        Gia_ManDupCones_rec( p, Gia_ObjFanin0(pObj), vLeaves, vNodes, vRoots );
        Gia_ManDupCones_rec( p, Gia_ObjFanin1(pObj), vLeaves, vNodes, vRoots );
        Vec_PtrPush( vNodes, pObj );
    }
    else if ( Gia_ObjIsCo(pObj) )
        Gia_ManDupCones_rec( p, Gia_ObjFanin0(pObj), vLeaves, vNodes, vRoots );
    else if ( Gia_ObjIsRo(p, pObj) )
        Vec_PtrPush( vRoots, Gia_ObjRoToRi(p, pObj) );
    else if ( Gia_ObjIsPi(p, pObj) )
        Vec_PtrPush( vLeaves, pObj );
    else assert( 0 );
}
Gia_Man_t * Gia_ManDupCones( Gia_Man_t * p, int * pPos, int nPos, int fTrimPis )
{
    Gia_Man_t * pNew;
    Vec_Ptr_t * vLeaves, * vNodes, * vRoots;
    Gia_Obj_t * pObj;
    int i;

    // collect initial POs
    vLeaves = Vec_PtrAlloc( 100 );
    vNodes = Vec_PtrAlloc( 100 );
    vRoots = Vec_PtrAlloc( 100 );
    for ( i = 0; i < nPos; i++ )
        Vec_PtrPush( vRoots, Gia_ManPo(p, pPos[i]) );

    // mark internal nodes
    Gia_ManIncrementTravId( p );
    Gia_ObjSetTravIdCurrent( p, Gia_ManConst0(p) );
    Vec_PtrForEachEntry( Gia_Obj_t *, vRoots, pObj, i )
        Gia_ManDupCones_rec( p, pObj, vLeaves, vNodes, vRoots );
    Vec_PtrSort( vLeaves, (int (*)(void))Gia_ObjCompareByCioId );

    // start the new manager
//    Gia_ManFillValue( p );
    pNew = Gia_ManStart( (fTrimPis ? Vec_PtrSize(vLeaves) : Gia_ManCiNum(p)) + Vec_PtrSize(vNodes) + Vec_PtrSize(vRoots) + 1 );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    // map the constant node
    Gia_ManConst0(p)->Value = 0;
    // create PIs
    if ( fTrimPis )
    {
        Vec_PtrForEachEntry( Gia_Obj_t *, vLeaves, pObj, i )
            pObj->Value = Gia_ManAppendCi( pNew );
    }
    else
    {
        Gia_ManForEachPi( p, pObj, i )
            pObj->Value = Gia_ManAppendCi( pNew );
    }
    // create LOs
    Vec_PtrForEachEntryStart( Gia_Obj_t *, vRoots, pObj, i, nPos )
        Gia_ObjRiToRo(p, pObj)->Value = Gia_ManAppendCi( pNew );
    // create internal nodes
    Vec_PtrForEachEntry( Gia_Obj_t *, vNodes, pObj, i )
        pObj->Value = Gia_ManAppendAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
    // create COs
    Vec_PtrForEachEntry( Gia_Obj_t *, vRoots, pObj, i )
        Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
    // finalize
    Gia_ManSetRegNum( pNew, Vec_PtrSize(vRoots)-nPos );
    Vec_PtrFree( vLeaves );
    Vec_PtrFree( vNodes );
    Vec_PtrFree( vRoots );
    return pNew;

}
Gia_Man_t * Gia_ManDupAndCones( Gia_Man_t * p, int * pAnds, int nAnds, int fTrimPis )
{
    Gia_Man_t * pNew;
    Vec_Ptr_t * vLeaves, * vNodes, * vRoots;
    Gia_Obj_t * pObj;
    int i;

    // collect initial POs
    vLeaves = Vec_PtrAlloc( 100 );
    vNodes = Vec_PtrAlloc( 100 );
    vRoots = Vec_PtrAlloc( 100 );
    for ( i = 0; i < nAnds; i++ )
//        Vec_PtrPush( vRoots, Gia_ManPo(p, pPos[i]) );
        Vec_PtrPush( vRoots, Gia_ManObj(p, pAnds[i]) );

    // mark internal nodes
    Gia_ManIncrementTravId( p );
    Gia_ObjSetTravIdCurrent( p, Gia_ManConst0(p) );
    Vec_PtrForEachEntry( Gia_Obj_t *, vRoots, pObj, i )
        Gia_ManDupCones_rec( p, pObj, vLeaves, vNodes, vRoots );
    Vec_PtrSort( vLeaves, (int (*)(void))Gia_ObjCompareByCioId );

    // start the new manager
//    Gia_ManFillValue( p );
    pNew = Gia_ManStart( Vec_PtrSize(vLeaves) + Vec_PtrSize(vNodes) + Vec_PtrSize(vRoots) + 1);
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    // map the constant node
    Gia_ManConst0(p)->Value = 0;
    // create PIs
    if ( fTrimPis )
    {
        Vec_PtrForEachEntry( Gia_Obj_t *, vLeaves, pObj, i )
            pObj->Value = Gia_ManAppendCi( pNew );
    }
    else
    {
        Gia_ManForEachPi( p, pObj, i )
            pObj->Value = Gia_ManAppendCi( pNew );
    }
    // create LOs
//    Vec_PtrForEachEntryStart( Gia_Obj_t *, vRoots, pObj, i, nPos )
//        Gia_ObjRiToRo(p, pObj)->Value = Gia_ManAppendCi( pNew );
    // create internal nodes
    Vec_PtrForEachEntry( Gia_Obj_t *, vNodes, pObj, i )
        if ( Gia_ObjIsMux(p, pObj) )
            pObj->Value = Gia_ManAppendMux( pNew, Gia_ObjFanin2Copy(p, pObj), Gia_ObjFanin1Copy(pObj), Gia_ObjFanin0Copy(pObj) );
        else if ( Gia_ObjIsXor(pObj) )
            pObj->Value = Gia_ManAppendXor( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
        else 
            pObj->Value = Gia_ManAppendAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
    // create COs
    Vec_PtrForEachEntry( Gia_Obj_t *, vRoots, pObj, i )
//        Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
        Gia_ManAppendCo( pNew, pObj->Value );
    // finalize
//    Gia_ManSetRegNum( pNew, Vec_PtrSize(vRoots)-nPos );
    Gia_ManSetRegNum( pNew, 0 );
    Vec_PtrFree( vLeaves );
    Vec_PtrFree( vNodes );
    Vec_PtrFree( vRoots );
    return pNew;

}
void Gia_ManDupAndConesLimit_rec( Gia_Man_t * pNew, Gia_Man_t * p, int iObj, int Level )
{
    Gia_Obj_t * pObj = Gia_ManObj(p, iObj);
    if ( ~pObj->Value )
        return;
    if ( !Gia_ObjIsAnd(pObj) || Gia_ObjLevel(p, pObj) < Level )
    {
        pObj->Value = Gia_ManAppendCi( pNew );
        //printf( "PI %d for %d.\n", Abc_Lit2Var(pObj->Value), iObj );
        return;
    }
    Gia_ManDupAndConesLimit_rec( pNew, p, Gia_ObjFaninId0(pObj, iObj), Level );
    Gia_ManDupAndConesLimit_rec( pNew, p, Gia_ObjFaninId1(pObj, iObj), Level );
    pObj->Value = Gia_ManAppendAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
    //printf( "Obj %d for %d.\n", Abc_Lit2Var(pObj->Value), iObj );
}
Gia_Man_t * Gia_ManDupAndConesLimit( Gia_Man_t * p, int * pAnds, int nAnds, int Level )
{
    Gia_Man_t * pNew;
    int i;
    pNew = Gia_ManStart( 1000 );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    Gia_ManLevelNum( p );
    Gia_ManFillValue( p );
    Gia_ManConst0(p)->Value = 0;
    for ( i = 0; i < nAnds; i++ )
        Gia_ManDupAndConesLimit_rec( pNew, p, pAnds[i], Level );
    for ( i = 0; i < nAnds; i++ )
        Gia_ManAppendCo( pNew, Gia_ManObj(p, pAnds[i])->Value );
    return pNew;
}

void Gia_ManDupAndConesLimit2_rec( Gia_Man_t * pNew, Gia_Man_t * p, int iObj, int Level )
{
    Gia_Obj_t * pObj = Gia_ManObj(p, iObj);
    if ( ~pObj->Value )
        return;
    if ( !Gia_ObjIsAnd(pObj) || Level <= 0 )
    {
        pObj->Value = Gia_ManAppendCi( pNew );
        //printf( "PI %d for %d.\n", Abc_Lit2Var(pObj->Value), iObj );
        return;
    }
    Gia_ManDupAndConesLimit2_rec( pNew, p, Gia_ObjFaninId0(pObj, iObj), Level-1 );
    Gia_ManDupAndConesLimit2_rec( pNew, p, Gia_ObjFaninId1(pObj, iObj), Level-1 );
    pObj->Value = Gia_ManAppendAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
    //printf( "Obj %d for %d.\n", Abc_Lit2Var(pObj->Value), iObj );
}
Gia_Man_t * Gia_ManDupAndConesLimit2( Gia_Man_t * p, int * pAnds, int nAnds, int Level )
{
    Gia_Man_t * pNew;
    int i;
    pNew = Gia_ManStart( 1000 );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    Gia_ManFillValue( p );
    Gia_ManConst0(p)->Value = 0;
    for ( i = 0; i < nAnds; i++ )
        Gia_ManDupAndConesLimit2_rec( pNew, p, pAnds[i], Level );
    for ( i = 0; i < nAnds; i++ )
        Gia_ManAppendCo( pNew, Gia_ManObj(p, pAnds[i])->Value );
    return pNew;

}

/**Function*************************************************************

  Synopsis    [Generates AIG representing 1-hot condition for N inputs.]

  Description [The condition is true of all POs are 0.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManOneHot( int nSkips, int nVars )
{
    Gia_Man_t * p;
    int i, b, Shift, iGiaLit, nLogVars = Abc_Base2Log( nVars );
    int * pTemp = ABC_CALLOC( int, (1 << nLogVars) );
    p = Gia_ManStart( nSkips + 4 * nVars + 1 );
    p->pName = Abc_UtilStrsav( "onehot" );
    for ( i = 0; i < nSkips; i++ )
        Gia_ManAppendCi( p );
    for ( i = 0; i < nVars; i++ )
        pTemp[i] = Gia_ManAppendCi( p );
    Gia_ManHashStart( p );
    for ( b = 0; b < nLogVars; b++ )
        for ( i = 0, Shift = (1<<b); i < (1 << nLogVars); i += 2*Shift )
        {
            iGiaLit = Gia_ManHashAnd( p, pTemp[i], pTemp[i + Shift] );
            if ( iGiaLit )
                Gia_ManAppendCo( p, iGiaLit );
            pTemp[i] = Gia_ManHashOr( p, pTemp[i], pTemp[i + Shift] );
        }
    Gia_ManHashStop( p );
    Gia_ManAppendCo( p, Abc_LitNot(pTemp[0]) );
    ABC_FREE( pTemp );
    assert( Gia_ManObjNum(p) <= nSkips + 4 * nVars + 1 );
    return p;
}
Gia_Man_t * Gia_ManDupOneHot( Gia_Man_t * p )
{
    Gia_Man_t * pOneHot, * pNew = Gia_ManDup( p );
    if ( Gia_ManRegNum(pNew) == 0 )
    {
        Abc_Print( 0, "Appending 1-hotness constraints to the PIs.\n" );
        pOneHot = Gia_ManOneHot( 0, Gia_ManCiNum(pNew) );
    }
    else
        pOneHot = Gia_ManOneHot( Gia_ManPiNum(pNew), Gia_ManRegNum(pNew) );
    Gia_ManDupAppendShare( pNew, pOneHot );
    pNew->nConstrs += Gia_ManPoNum(pOneHot);
    Gia_ManStop( pOneHot );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Duplicates the AIG with nodes ordered by level.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManDupLevelized( Gia_Man_t * p )
{
    Gia_Man_t * pNew;
    Gia_Obj_t * pObj;
    int i, nLevels = Gia_ManLevelNum( p );
    int * pCounts = ABC_CALLOC( int, nLevels + 1 );
    int * pNodes = ABC_ALLOC( int, Gia_ManAndNum(p) );
    Gia_ManForEachAnd( p, pObj, i )
        pCounts[Gia_ObjLevel(p, pObj)]++;
    for ( i = 1; i <= nLevels; i++ )
        pCounts[i] += pCounts[i-1];
    Gia_ManForEachAnd( p, pObj, i )
        pNodes[pCounts[Gia_ObjLevel(p, pObj)-1]++] = i;
    // duplicate
    pNew = Gia_ManStart( Gia_ManObjNum(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    Gia_ManConst0(p)->Value = 0;
    Gia_ManForEachCi( p, pObj, i )
        pObj->Value = Gia_ManAppendCi( pNew );
    for ( i = 0; i < Gia_ManAndNum(p) && (pObj = Gia_ManObj(p, pNodes[i])); i++ )
        pObj->Value = Gia_ManAppendAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
    Gia_ManForEachCo( p, pObj, i )
        pObj->Value = Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
    Gia_ManSetRegNum( pNew, Gia_ManRegNum(p) );
    ABC_FREE( pCounts );
    ABC_FREE( pNodes );
    return pNew;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManDupFromVecs( Gia_Man_t * p, Vec_Int_t * vCis, Vec_Int_t * vAnds, Vec_Int_t * vCos, int nRegs )
{
    Gia_Man_t * pNew;
    Gia_Obj_t * pObj;
    int i;
    // start the new manager
    pNew = Gia_ManStart( 5000 );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    // create constant
    Gia_ManConst0(p)->Value = 0;
    // create PIs
    Gia_ManForEachObjVec( vCis, p, pObj, i )
        pObj->Value = Gia_ManAppendCi( pNew );
    // create internal nodes
    Gia_ManForEachObjVec( vAnds, p, pObj, i )
        pObj->Value = Gia_ManAppendAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
    // create ROs
    Gia_ManForEachObjVec( vCos, p, pObj, i )
        pObj->Value = Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
    Gia_ManSetRegNum( pNew, nRegs );
    return pNew;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManDupSliced( Gia_Man_t * p, int nSuppMax )
{
    Gia_Man_t * pNew;
    Gia_Obj_t * pObj;
    int i;
    // start the new manager
    pNew = Gia_ManStart( 5000 );
    pNew->pName = Abc_UtilStrsav( p->pName );
    // create constant and PIs
    Gia_ManConst0(p)->Value = 0;
    Gia_ManForEachCi( p, pObj, i )
        pObj->Value = Gia_ManAppendCi( pNew );
    // create internal nodes
    Gia_ManCleanMark01(p);
    Gia_ManForEachAnd( p, pObj, i )
        if ( Gia_ManSuppSize(p, &i, 1) <= nSuppMax )
        {
            pObj->Value = Gia_ManAppendAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
            pObj->fMark0 = 1;
        }
        else 
        {
            Gia_ObjFanin0(pObj)->fMark1 = 1;
            Gia_ObjFanin1(pObj)->fMark1 = 1;
        }
    Gia_ManForEachCo( p, pObj, i )
        Gia_ObjFanin0(pObj)->fMark1 = 1;
    // add POs for the nodes pointed to
    Gia_ManForEachAnd( p, pObj, i )
        if ( pObj->fMark0 && pObj->fMark1 )
            Gia_ManAppendCo( pNew, pObj->Value );
    // cleanup and leave
    Gia_ManCleanMark01(p);
    return pNew;
}


/**Function*************************************************************

  Synopsis    [Extract constraints.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManDupWithConstrCollectAnd_rec( Gia_Man_t * p, Gia_Obj_t * pObj, Vec_Int_t * vSuper, int fFirst )
{
    if ( (Gia_IsComplement(pObj) || !Gia_ObjIsAnd(pObj)) && !fFirst )
    {
        Vec_IntPushUnique( vSuper, Gia_ObjToLit(p, pObj) );
        return;
    }
    Gia_ManDupWithConstrCollectAnd_rec( p, Gia_ObjChild0(pObj), vSuper, 0 );
    Gia_ManDupWithConstrCollectAnd_rec( p, Gia_ObjChild1(pObj), vSuper, 0 );
}
Gia_Man_t * Gia_ManDupWithConstr( Gia_Man_t * p )
{
    Vec_Int_t * vSuper;
    Gia_Man_t * pNew, * pTemp;
    Gia_Obj_t * pObj;
    int i, iDriver, iLit, iLitBest = -1, LevelBest = -1;
    assert( Gia_ManPoNum(p) == 1 );
    assert( Gia_ManRegNum(p) == 0 );
    pObj = Gia_ManPo( p, 0 );
    if ( Gia_ObjFaninC0(pObj) )
    {
        printf( "The miter's output is not AND-decomposable.\n" );
        return NULL;
    }
    if ( Gia_ObjFaninId0p(p, pObj) == 0 )
    {
        printf( "The miter's output is a constant.\n" );
        return NULL;
    }
    vSuper = Vec_IntAlloc( 100 );
    Gia_ManDupWithConstrCollectAnd_rec( p, Gia_ObjChild0(pObj), vSuper, 1 );
    assert( Vec_IntSize(vSuper) > 1 );
    // find the highest level
    Gia_ManLevelNum( p );
    Vec_IntForEachEntry( vSuper, iLit, i )
        if ( LevelBest < Gia_ObjLevelId(p, Abc_Lit2Var(iLit)) )
            LevelBest = Gia_ObjLevelId(p, Abc_Lit2Var(iLit)), iLitBest = iLit;
    assert( iLitBest != -1 );
    // create new manager
    pNew = Gia_ManStart( Gia_ManObjNum(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    Gia_ManConst0(p)->Value = 0;
    Gia_ManHashAlloc( pNew );
    Gia_ManForEachObj1( p, pObj, i )
    {
        if ( Gia_ObjIsAnd(pObj) )
            pObj->Value = Gia_ManHashAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
        else if ( Gia_ObjIsCi(pObj) )
            pObj->Value = Gia_ManAppendCi( pNew );
    }
    // create AND of nodes
    iDriver = -1;
    Vec_IntForEachEntry( vSuper, iLit, i )
    {
        if ( iLit == iLitBest )
            continue;
        if ( iDriver == -1 )
            iDriver = Gia_ObjLitCopy(p, iLit);
        else
            iDriver = Gia_ManHashAnd( pNew, iDriver, Gia_ObjLitCopy(p, iLit) );
    }
    // create the main PO
    Gia_ManAppendCo( pNew, Gia_ObjLitCopy(p, iLitBest) );
    // create the constraint PO
    Gia_ManAppendCo( pNew, Abc_LitNot(iDriver) );
    pNew->nConstrs = 1;
    // rehash
    pNew = Gia_ManCleanup( pTemp = pNew );
    Gia_ManStop( pTemp );
    Vec_IntFree( vSuper );
    return pNew;

}

/**Function*************************************************************

  Synopsis    [Compares two objects by their distance.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManSortByValue( Gia_Obj_t ** pp1, Gia_Obj_t ** pp2 )
{
    int Diff = Gia_Regular(*pp1)->Value - Gia_Regular(*pp2)->Value;
    if ( Diff < 0 )
        return -1;
    if ( Diff > 0 ) 
        return 1;
    return 0; 
}

/**Function*************************************************************

  Synopsis    [Decomposes the miter outputs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManDupOuts( Gia_Man_t * p )
{
    Gia_Man_t * pNew;
    Gia_Obj_t * pObj;
    int i;
    pNew = Gia_ManStart( Gia_ManObjNum(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    Gia_ManConst0(p)->Value = 0;
    Gia_ManForEachCi( p, pObj, i )
        pObj->Value = Gia_ManAppendCi(pNew);
    Gia_ManForEachAnd( p, pObj, i )
        pObj->Value = Gia_ManAppendAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
    Gia_ManForEachPo( p, pObj, i )
        Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
    Gia_ManForEachAnd( p, pObj, i )
        Gia_ManAppendCo( pNew, pObj->Value );
    Gia_ManForEachRi( p, pObj, i )
        Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
    Gia_ManSetRegNum( pNew, Gia_ManRegNum(p) );
    assert( Gia_ManIsNormalized(pNew) );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Computes supports for each node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Wec_t * Gia_ManCreateNodeSupps( Gia_Man_t * p, Vec_Int_t * vNodes, int fVerbose )
{
    abctime clk = Abc_Clock();
    Gia_Obj_t * pObj; int i, Id;
    Vec_Wec_t * vSuppsNo = Vec_WecStart( Vec_IntSize(vNodes) );
    Vec_Wec_t * vSupps = Vec_WecStart( Gia_ManObjNum(p) );
    Gia_ManForEachCiId( p, Id, i )
        Vec_IntPush( Vec_WecEntry(vSupps, Id), i );
    Gia_ManForEachAnd( p, pObj, Id )
        Vec_IntTwoMerge2( Vec_WecEntry(vSupps, Gia_ObjFaninId0(pObj, Id)), 
                          Vec_WecEntry(vSupps, Gia_ObjFaninId1(pObj, Id)), 
                          Vec_WecEntry(vSupps, Id) ); 
    Gia_ManForEachObjVec( vNodes, p, pObj, i )
        Vec_IntAppend( Vec_WecEntry(vSuppsNo, i), Vec_WecEntry(vSupps, Gia_ObjId(p, pObj)) );
    Vec_WecFree( vSupps );
    if ( fVerbose )
        Abc_PrintTime( 1, "Support computation", Abc_Clock() - clk );
    return vSuppsNo;
}

Vec_Wec_t * Gia_ManCreateCoSupps( Gia_Man_t * p, int fVerbose )
{
    abctime clk = Abc_Clock();
    Gia_Obj_t * pObj; int i, Id;
    Vec_Wec_t * vSuppsCo = Vec_WecStart( Gia_ManCoNum(p) );
    Vec_Wec_t * vSupps = Vec_WecStart( Gia_ManObjNum(p) );
    Gia_ManForEachCiId( p, Id, i )
        Vec_IntPush( Vec_WecEntry(vSupps, Id), i );
    Gia_ManForEachAnd( p, pObj, Id )
        Vec_IntTwoMerge2( Vec_WecEntry(vSupps, Gia_ObjFaninId0(pObj, Id)), 
                          Vec_WecEntry(vSupps, Gia_ObjFaninId1(pObj, Id)), 
                          Vec_WecEntry(vSupps, Id) ); 
    Gia_ManForEachCo( p, pObj, i )
        Vec_IntAppend( Vec_WecEntry(vSuppsCo, i), Vec_WecEntry(vSupps, Gia_ObjFaninId0p(p, pObj)) );
    Vec_WecFree( vSupps );
    if ( fVerbose )
        Abc_PrintTime( 1, "Support computation", Abc_Clock() - clk );
    return vSuppsCo;
}
int Gia_ManCoSuppSizeMax( Gia_Man_t * p, Vec_Wec_t * vSupps )
{
    Gia_Obj_t * pObj; 
    Vec_Int_t * vSuppOne;
    int i, nSuppMax = 1;
    Gia_ManForEachCo( p, pObj, i )
    {
        vSuppOne = Vec_WecEntry( vSupps, i );
        nSuppMax = Abc_MaxInt( nSuppMax, Vec_IntSize(vSuppOne) );
    }
    return nSuppMax;
}
int Gia_ManCoLargestSupp( Gia_Man_t * p, Vec_Wec_t * vSupps )
{
    Gia_Obj_t * pObj; 
    Vec_Int_t * vSuppOne;
    int i, iCoMax = -1, nSuppMax = -1;
    Gia_ManForEachCo( p, pObj, i )
    {
        vSuppOne = Vec_WecEntry( vSupps, i );
        if ( nSuppMax < Vec_IntSize(vSuppOne) )
        {
            nSuppMax = Vec_IntSize(vSuppOne);
            iCoMax = i;
        }
    }
    return iCoMax;
}
Vec_Int_t * Gia_ManSortCoBySuppSize( Gia_Man_t * p, Vec_Wec_t * vSupps )
{
    Vec_Int_t * vOrder = Vec_IntAlloc( Gia_ManCoNum(p) );
    Vec_Wrd_t * vSortData = Vec_WrdAlloc( Gia_ManCoNum(p) );
    Vec_Int_t * vSuppOne; word Entry; int i;
    Vec_WecForEachLevel( vSupps, vSuppOne, i )
        Vec_WrdPush( vSortData, ((word)i << 32) | Vec_IntSize(vSuppOne) );
    Abc_QuickSort3( Vec_WrdArray(vSortData), Vec_WrdSize(vSortData), 1 );
    Vec_WrdForEachEntry( vSortData, Entry, i )
        Vec_IntPush( vOrder, (int)(Entry >> 32) );
    Vec_WrdFree( vSortData );
    return vOrder;
}

/**Function*************************************************************

  Synopsis    [Remaps each CO cone to depend on the first CI variables.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManDupHashDfs_rec( Gia_Man_t * pNew, Gia_Man_t * p, Gia_Obj_t * pObj )
{
    if ( ~pObj->Value )
        return pObj->Value;
    assert( Gia_ObjIsAnd(pObj) );
    Gia_ManDupHashDfs_rec( pNew, p, Gia_ObjFanin0(pObj) );
    Gia_ManDupHashDfs_rec( pNew, p, Gia_ObjFanin1(pObj) );
    return pObj->Value = Gia_ManHashAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
} 
void Gia_ManDupCleanDfs_rec( Gia_Obj_t * pObj )
{
    if ( !~pObj->Value )
        return;
    pObj->Value = ~0;
    if ( Gia_ObjIsCi(pObj) )
        return;
    assert( Gia_ObjIsAnd(pObj) );
    Gia_ManDupCleanDfs_rec( Gia_ObjFanin0(pObj) );
    Gia_ManDupCleanDfs_rec( Gia_ObjFanin1(pObj) );
} 
Gia_Man_t * Gia_ManDupStrashReduce( Gia_Man_t * p, Vec_Wec_t * vSupps, Vec_Int_t ** pvCoMap )
{
    Gia_Obj_t * pObj;
    Gia_Man_t * pNew, * pTemp;
    Vec_Int_t * vSuppOne, * vCoMapLit;
    int i, k, iCi, iLit, nSuppMax;
    assert( Gia_ManRegNum(p) == 0 );
    Gia_ManFillValue( p );
    vCoMapLit = Vec_IntAlloc( Gia_ManCoNum(p) );
    pNew = Gia_ManStart( Gia_ManObjNum(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    Gia_ManConst0(p)->Value = 0;
    nSuppMax = Gia_ManCoSuppSizeMax( p, vSupps );
    for ( i = 0; i < nSuppMax; i++ )
        Gia_ManAppendCi(pNew);
    Gia_ManHashAlloc( pNew );
    Gia_ManForEachCo( p, pObj, i )
    {
        vSuppOne = Vec_WecEntry( vSupps, i );
        if ( Vec_IntSize(vSuppOne) == 0 )
            Vec_IntPush( vCoMapLit, Abc_Var2Lit(0, Gia_ObjFaninC0(pObj)) );
        else if ( Vec_IntSize(vSuppOne) == 1 )
            Vec_IntPush( vCoMapLit, Abc_Var2Lit(1, Gia_ObjFaninC0(pObj)) );
        else
        {
            Vec_IntForEachEntry( vSuppOne, iCi, k )
                Gia_ManCi(p, iCi)->Value = Gia_Obj2Lit(pNew, Gia_ManCi(pNew, k) );
            Gia_ManDupHashDfs_rec( pNew, p, Gia_ObjFanin0(pObj) );
            assert( Gia_ObjFanin0Copy(pObj) < 2 * Gia_ManObjNum(pNew) );
            Vec_IntPush( vCoMapLit, Gia_ObjFanin0Copy(pObj) );
            Gia_ManDupCleanDfs_rec( Gia_ObjFanin0(pObj) );
        }
    }
    Gia_ManHashStop( pNew );
    assert( Vec_IntSize(vCoMapLit) == Gia_ManCoNum(p) );
    if ( pvCoMap == NULL ) // do not remap
    {
        Vec_IntForEachEntry( vCoMapLit, iLit, i )
            Gia_ManAppendCo( pNew, iLit );
    }
    else // remap
    {
        Vec_Int_t * vCoMapRes = Vec_IntAlloc( Gia_ManCoNum(p) );      // map old CO into new CO 
        Vec_Int_t * vMap = Vec_IntStartFull( 2*Gia_ManObjNum(pNew) ); // map new lit into new CO
        Vec_IntForEachEntry( vCoMapLit, iLit, i )
        {
            if ( Vec_IntEntry(vMap, iLit) == -1 )
            {
                Vec_IntWriteEntry( vMap, iLit, Gia_ManCoNum(pNew) );
                Gia_ManAppendCo( pNew, iLit );
            }
            Vec_IntPush( vCoMapRes, Vec_IntEntry(vMap, iLit) );
        }
        Vec_IntFree( vMap );
        *pvCoMap = vCoMapRes;
    }
    Vec_IntFree( vCoMapLit );
    pNew = Gia_ManCleanup( pTemp = pNew );
    Gia_ManStop( pTemp );
    return pNew;
}
Gia_Man_t * Gia_ManIsoStrashReduce2( Gia_Man_t * p, Vec_Ptr_t ** pvPosEquivs, int fVerbose )
{
    Vec_Int_t * vCoMap;
    Vec_Wec_t * vSupps = Gia_ManCreateCoSupps( p, fVerbose );
    Gia_Man_t * pNew = Gia_ManDupStrashReduce( p, vSupps, &vCoMap );
    Vec_IntFree( vCoMap );
    Vec_WecFree( vSupps );
    *pvPosEquivs = NULL;
    return pNew;
}

int Gia_ManIsoStrashReduceOne( Gia_Man_t * pNew, Gia_Man_t * p, Gia_Obj_t * pObj, Vec_Int_t * vSupp )
{
    int k, iCi, iLit;
    assert( Gia_ObjIsCo(pObj) );
    if ( Vec_IntSize(vSupp) == 0 )
        return Abc_Var2Lit(0, Gia_ObjFaninC0(pObj));
    if ( Vec_IntSize(vSupp) == 1 )
        return Abc_Var2Lit(1, Gia_ObjFaninC0(pObj));
    Vec_IntForEachEntry( vSupp, iCi, k )
        Gia_ManCi(p, iCi)->Value = Gia_Obj2Lit(pNew, Gia_ManCi(pNew, k) );
    Gia_ManDupHashDfs_rec( pNew, p, Gia_ObjFanin0(pObj) );
    iLit = Gia_ObjFanin0Copy(pObj);
    Gia_ManDupCleanDfs_rec( Gia_ObjFanin0(pObj) );
    return iLit;
}
Vec_Wec_t * Gia_ManIsoStrashReduceInt( Gia_Man_t * p, Vec_Wec_t * vSupps, int fVerbose )
{
    Gia_Man_t * pNew;
    Gia_Obj_t * pObj;
    Vec_Wec_t * vPosEquivs = Vec_WecAlloc( 100 );
    Vec_Int_t * vSuppOne, * vMap = Vec_IntAlloc( 10000 );
    int i, iLit, nSuppMax = Gia_ManCoSuppSizeMax( p, vSupps );
    // count how many times each support size appears
    Vec_Int_t * vSizeCount = Vec_IntStart( nSuppMax + 1 );
    Vec_WecForEachLevel( vSupps, vSuppOne, i )
        Vec_IntAddToEntry( vSizeCount, Vec_IntSize(vSuppOne), 1 );
    // create array of unique outputs
    Gia_ManFillValue( p );
    pNew = Gia_ManStart( Gia_ManObjNum(p) );
    Gia_ManConst0(p)->Value = 0;
    for ( i = 0; i < nSuppMax; i++ )
        Gia_ManAppendCi(pNew);
    Gia_ManHashAlloc( pNew );
    Gia_ManForEachCo( p, pObj, i )
    {
        vSuppOne = Vec_WecEntry( vSupps, i );
        if ( Vec_IntEntry(vSizeCount, Vec_IntSize(vSuppOne)) == 1 )
        {
            Vec_IntPush( Vec_WecPushLevel(vPosEquivs), i );
            continue;
        }
        iLit = Gia_ManIsoStrashReduceOne( pNew, p, pObj, vSuppOne );
        Vec_IntFillExtra( vMap, iLit + 1, -1 );
        if ( Vec_IntEntry(vMap, iLit) == -1 )
        {
            Vec_IntWriteEntry( vMap, iLit, Vec_WecSize(vPosEquivs) );
            Vec_IntPush( Vec_WecPushLevel(vPosEquivs), i );
            continue;
        }
        Vec_IntPush( Vec_WecEntry(vPosEquivs, Vec_IntEntry(vMap, iLit)), i );
    }
    Gia_ManHashStop( pNew );
    Gia_ManStop( pNew );
    Vec_IntFree( vSizeCount );
    Vec_IntFree( vMap );
    return vPosEquivs;
}
Gia_Man_t * Gia_ManIsoStrashReduce( Gia_Man_t * p, Vec_Ptr_t ** pvPosEquivs, int fVerbose )
{
    Vec_Wec_t * vSupps = Gia_ManCreateCoSupps( p, fVerbose );
    Vec_Wec_t * vPosEquivs = Gia_ManIsoStrashReduceInt( p, vSupps, fVerbose );
    // find the first outputs and derive GIA
    Vec_Int_t * vFirsts = Vec_WecCollectFirsts( vPosEquivs );
    Gia_Man_t * pNew = Gia_ManDupCones( p, Vec_IntArray(vFirsts), Vec_IntSize(vFirsts), 0 );
    Vec_IntFree( vFirsts );
    Vec_WecFree( vSupps );
    // report and return    
    if ( fVerbose )
    { 
        printf( "Nontrivial classes:\n" );
        Vec_WecPrint( vPosEquivs, 1 );
    }
    if ( pvPosEquivs )
        *pvPosEquivs = Vec_WecConvertToVecPtr( vPosEquivs );
    Vec_WecFree( vPosEquivs );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Decomposes the miter outputs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManDupDemiter( Gia_Man_t * p, int fVerbose )
{
    Vec_Int_t * vSuper;
    Vec_Ptr_t * vSuperPtr;
    Gia_Man_t * pNew, * pTemp;
    Gia_Obj_t * pObj, * pObjPo;
    int i, iLit;
    assert( Gia_ManPoNum(p) == 1 );
    // decompose
    pObjPo = Gia_ManPo( p, 0 );
    vSuper = Vec_IntAlloc( 100 );
    Gia_ManDupWithConstrCollectAnd_rec( p, Gia_ObjFanin0(pObjPo), vSuper, 1 );
    assert( Vec_IntSize(vSuper) > 1 );
    // report the result
    printf( "The miter is %s-decomposable into %d parts.\n", Gia_ObjFaninC0(pObjPo) ? "OR":"AND", Vec_IntSize(vSuper) );
    // create levels
    Gia_ManLevelNum( p );
    Vec_IntForEachEntry( vSuper, iLit, i )
        Gia_ManObj(p, Abc_Lit2Var(iLit))->Value = Gia_ObjLevelId(p, Abc_Lit2Var(iLit));
    // create pointer array
    vSuperPtr = Vec_PtrAlloc( Vec_IntSize(vSuper) );
    Vec_IntForEachEntry( vSuper, iLit, i )
        Vec_PtrPush( vSuperPtr, Gia_Lit2Obj(p, iLit) );
    Vec_PtrSort( vSuperPtr, (int (*)(void))Gia_ManSortByValue );
    // create new manager
    pNew = Gia_ManStart( Gia_ManObjNum(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    Gia_ManConst0(p)->Value = 0;
    Gia_ManHashAlloc( pNew );
    Gia_ManForEachCi( p, pObj, i )
        pObj->Value = Gia_ManAppendCi( pNew );
    Gia_ManForEachAnd( p, pObj, i )
        pObj->Value = Gia_ManHashAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
    // create the outputs
    Vec_PtrForEachEntry( Gia_Obj_t *, vSuperPtr, pObj, i )
        Gia_ManAppendCo( pNew, Gia_ObjLitCopy(p, Gia_Obj2Lit(p, pObj)) ^ Gia_ObjFaninC0(pObjPo) );
    Gia_ManForEachRi( p, pObj, i )
        Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
    Gia_ManSetRegNum( pNew, Gia_ManRegNum(p) );
    // rehash
    pNew = Gia_ManCleanup( pTemp = pNew );
    Gia_ManStop( pTemp );
    Vec_IntFree( vSuper );
    Vec_PtrFree( vSuperPtr );
    return pNew;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManDupDemiterOrderXors2( Gia_Man_t * p, Vec_Int_t * vXors )
{
    int i, iObj, * pPerm;
    Vec_Int_t * vSizes = Vec_IntAlloc( 100 );
    Vec_IntForEachEntry( vXors, iObj, i )
        Vec_IntPush( vSizes, Gia_ManSuppSize(p, &iObj, 1) );
    pPerm = Abc_MergeSortCost( Vec_IntArray(vSizes), Vec_IntSize(vSizes) );
    Vec_IntClear( vSizes );
    for ( i = 0; i < Vec_IntSize(vXors); i++ )
        Vec_IntPush( vSizes, Vec_IntEntry(vXors, pPerm[i]) );
    ABC_FREE( pPerm );
    Vec_IntClear( vXors );
    Vec_IntAppend( vXors, vSizes );
    Vec_IntFree( vSizes );
}
int Gia_ManDupDemiterFindMin( Vec_Wec_t * vSupps, Vec_Int_t * vTakenIns, Vec_Int_t * vTakenOuts )
{
    Vec_Int_t * vLevel;
    int i, k, iObj, iObjBest = -1;
    int Count, CountBest = ABC_INFINITY;
    Vec_WecForEachLevel( vSupps, vLevel, i )
    {
        if ( Vec_IntEntry(vTakenOuts, i) )
            continue;
        Count = 0;
        Vec_IntForEachEntry( vLevel, iObj, k )
            Count += !Vec_IntEntry(vTakenIns, iObj);
        if ( CountBest > Count )
        {
            CountBest = Count;
            iObjBest = i;
        }
    }
    return iObjBest;
}
void Gia_ManDupDemiterOrderXors( Gia_Man_t * p, Vec_Int_t * vXors )
{
    extern Vec_Wec_t * Gia_ManCreateNodeSupps( Gia_Man_t * p, Vec_Int_t * vNodes, int fVerbose );
    Vec_Wec_t * vSupps = Gia_ManCreateNodeSupps( p, vXors, 0 );
    Vec_Int_t * vTakenIns = Vec_IntStart( Gia_ManCiNum(p) );
    Vec_Int_t * vTakenOuts = Vec_IntStart( Vec_IntSize(vXors) );
    Vec_Int_t * vOrder = Vec_IntAlloc( Vec_IntSize(vXors) );
    int i, k, iObj;
    // add outputs in the order of increasing supports
    for ( i = 0; i < Vec_IntSize(vXors); i++ )
    {
        int Index = Gia_ManDupDemiterFindMin( vSupps, vTakenIns, vTakenOuts );
        assert( Index >= 0 && Index < Vec_IntSize(vXors) );
        Vec_IntPush( vOrder, Vec_IntEntry(vXors, Index) );
        assert( !Vec_IntEntry( vTakenOuts, Index ) );
        Vec_IntWriteEntry( vTakenOuts, Index, 1 );
        Vec_IntForEachEntry( Vec_WecEntry(vSupps, Index), iObj, k )
            Vec_IntWriteEntry( vTakenIns, iObj, 1 );
    }
    Vec_WecFree( vSupps );
    Vec_IntFree( vTakenIns );
    Vec_IntFree( vTakenOuts );
    // reload
    Vec_IntClear( vXors );
    Vec_IntAppend( vXors, vOrder );
    Vec_IntFree( vOrder );
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManSetMark0Dfs_rec( Gia_Man_t * p, int iObj )
{
    Gia_Obj_t * pObj;
    pObj = Gia_ManObj( p, iObj );
    if ( pObj->fMark0 )
        return;
    pObj->fMark0 = 1;
    if ( !Gia_ObjIsAnd(pObj) )
        return;
    Gia_ManSetMark0Dfs_rec( p, Gia_ObjFaninId0(pObj, iObj) );
    Gia_ManSetMark0Dfs_rec( p, Gia_ObjFaninId1(pObj, iObj) );
}
void Gia_ManSetMark1Dfs_rec( Gia_Man_t * p, int iObj )
{
    Gia_Obj_t * pObj;
    pObj = Gia_ManObj( p, iObj );
    if ( pObj->fMark1 )
        return;
    pObj->fMark1 = 1;
    if ( !Gia_ObjIsAnd(pObj) )
        return;
    Gia_ManSetMark1Dfs_rec( p, Gia_ObjFaninId0(pObj, iObj) );
    Gia_ManSetMark1Dfs_rec( p, Gia_ObjFaninId1(pObj, iObj) );
}

int Gia_ManCountMark0Dfs_rec( Gia_Man_t * p, int iObj )
{
    Gia_Obj_t * pObj;
    if ( Gia_ObjIsTravIdCurrentId(p, iObj) )
        return 0;
    Gia_ObjSetTravIdCurrentId(p, iObj);
    pObj = Gia_ManObj( p, iObj );
    if ( !Gia_ObjIsAnd(pObj) )
        return pObj->fMark0;
    return Gia_ManCountMark0Dfs_rec( p, Gia_ObjFaninId0(pObj, iObj) ) + 
           Gia_ManCountMark0Dfs_rec( p, Gia_ObjFaninId1(pObj, iObj) ) + pObj->fMark0;
}
int Gia_ManCountMark0Dfs( Gia_Man_t * p, int iObj )
{
    Gia_ManIncrementTravId( p );
    return Gia_ManCountMark0Dfs_rec( p, iObj );
}
int Gia_ManCountMark1Dfs_rec( Gia_Man_t * p, int iObj )
{
    Gia_Obj_t * pObj;
    if ( Gia_ObjIsTravIdCurrentId(p, iObj) )
        return 0;
    Gia_ObjSetTravIdCurrentId(p, iObj);
    pObj = Gia_ManObj( p, iObj );
    if ( !Gia_ObjIsAnd(pObj) )
        return pObj->fMark1;
    return Gia_ManCountMark1Dfs_rec( p, Gia_ObjFaninId0(pObj, iObj) ) + 
           Gia_ManCountMark1Dfs_rec( p, Gia_ObjFaninId1(pObj, iObj) ) + pObj->fMark1;
}
int Gia_ManCountMark1Dfs( Gia_Man_t * p, int iObj )
{
    Gia_ManIncrementTravId( p );
    return Gia_ManCountMark1Dfs_rec( p, iObj );
}

int Gia_ManDecideWhereToAdd( Gia_Man_t * p, Vec_Int_t * vPart[2], Gia_Obj_t * pFan[2] )
{
    int Count0 = 1, Count1 = 0;
    assert( Vec_IntSize(vPart[0]) == Vec_IntSize(vPart[1]) );
    if ( Vec_IntSize(vPart[0]) > 0 )
    {
        Count0 = Gia_ManCountMark0Dfs(p, Gia_ObjId(p, pFan[0])) + Gia_ManCountMark1Dfs(p, Gia_ObjId(p, pFan[1]));
        Count1 = Gia_ManCountMark0Dfs(p, Gia_ObjId(p, pFan[1])) + Gia_ManCountMark1Dfs(p, Gia_ObjId(p, pFan[0]));
    }
    return Count0 < Count1;
}
void Gia_ManCollectTopXors_rec( Gia_Man_t * p, Gia_Obj_t * pObj, Vec_Int_t * vXors )
{
    Gia_Obj_t * pFan0, * pFan1;
    int iObj = Gia_ObjId( p, pObj );
    if ( Gia_ObjRecognizeExor(pObj, &pFan0, &pFan1) || !Gia_ObjIsAnd(pObj) )
    {
        Vec_IntPushUnique( vXors, Gia_ObjId(p, pObj) );
        return;
    }
    if ( Gia_ObjFaninC0(pObj) )
        Vec_IntPushUnique( vXors, Gia_ObjFaninId0(pObj, iObj) );
    else
        Gia_ManCollectTopXors_rec( p, Gia_ObjFanin0(pObj), vXors );
    if ( Gia_ObjFaninC1(pObj) )
        Vec_IntPushUnique( vXors, Gia_ObjFaninId1(pObj, iObj) );
    else
        Gia_ManCollectTopXors_rec( p, Gia_ObjFanin1(pObj), vXors );
}
Vec_Int_t * Gia_ManCollectTopXors( Gia_Man_t * p )
{
    int i, iObj, iObj2, fFlip, Count1 = 0;
    Vec_Int_t * vXors, * vPart[2], * vOrder; 
    Gia_Obj_t * pFan[2], * pObj = Gia_ManCo(p, 0);
    vXors = Vec_IntAlloc( 100 );
    if ( Gia_ManCoNum(p) == 1 )
    {
        if ( Gia_ObjFaninC0(pObj) )
            Gia_ManCollectTopXors_rec( p, Gia_ObjFanin0(pObj), vXors );
        else
            Vec_IntPush( vXors, Gia_ObjId(p, Gia_ObjFanin0(pObj)) );
    }
    else
    {
        Gia_ManForEachCo( p, pObj, i )
            if ( Gia_ObjFaninId0p(p, pObj) > 0 )
                Vec_IntPush( vXors, Gia_ObjFaninId0p(p, pObj) );
    }
    // order by support size
    Gia_ManDupDemiterOrderXors( p, vXors );
    //Vec_IntPrint( vXors );
    Vec_IntReverseOrder( vXors ); // from MSB to LSB
    // divide into groups
    Gia_ManCleanMark01(p);
    vPart[0] = Vec_IntAlloc( 100 );
    vPart[1] = Vec_IntAlloc( 100 );
    Gia_ManForEachObjVec( vXors, p, pObj, i )
    {
        int fCompl = 0;
        if ( !Gia_ObjRecognizeExor(pObj, &pFan[0], &pFan[1]) )
            pFan[0] = pObj, pFan[1] = Gia_ManConst0(p), Count1++;
        else
        {
            fCompl ^= Gia_IsComplement(pFan[0]);
            fCompl ^= Gia_IsComplement(pFan[1]);
            pFan[0] = Gia_Regular(pFan[0]);
            pFan[1] = Gia_Regular(pFan[1]);
        }
        fFlip = Gia_ManDecideWhereToAdd( p, vPart, pFan );
        Vec_IntPush( vPart[0], Gia_ObjId(p, pFan[fFlip]) );
        Vec_IntPush( vPart[1], Gia_ObjId(p, pFan[!fFlip]) );
        Gia_ManSetMark0Dfs_rec( p, Gia_ObjId(p, pFan[fFlip]) );
        Gia_ManSetMark1Dfs_rec( p, Gia_ObjId(p, pFan[!fFlip]) );
    }
    //printf( "Detected %d single-output XOR miters and %d other miters.\n", Vec_IntSize(vXors) - Count1, Count1 );
    Vec_IntFree( vXors );
    Gia_ManCleanMark01(p);
    // create new order
    vOrder = Vec_IntAlloc( 100 );
    Vec_IntForEachEntryTwo( vPart[0], vPart[1], iObj, iObj2, i )
        Vec_IntPushTwo( vOrder, iObj, iObj2 );
    Vec_IntFree( vPart[0] );
    Vec_IntFree( vPart[1] );
    Vec_IntReverseOrder( vOrder ); // from LSB to MSB
    //Vec_IntPrint( vOrder );
    return vOrder;
}
Gia_Man_t * Gia_ManDemiterToDual( Gia_Man_t * p )
{
    Gia_Man_t * pNew; Gia_Obj_t * pObj; int i;
    Vec_Int_t * vNodes;
    Vec_Int_t * vOrder = Gia_ManCollectTopXors( p );
    if ( vOrder == NULL )
    {
        printf( "Cannot demiter because the top-most gate is an AND-gate.\n" );
        return NULL;
    }
    assert( Vec_IntSize(vOrder) % 2 == 0 );
    vNodes = Vec_IntAlloc( Gia_ManObjNum(p) );
    Gia_ManIncrementTravId( p );
    Gia_ManCollectAnds( p, Vec_IntArray(vOrder), Vec_IntSize(vOrder), vNodes, NULL );
    pNew = Gia_ManStart( 1 + Gia_ManCiNum(p) + Vec_IntSize(vNodes) + Vec_IntSize(vOrder) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    Gia_ManConst0(p)->Value = 0;
    Gia_ManForEachCi( p, pObj, i )
        pObj->Value = Gia_ManAppendCi( pNew );
    Gia_ManForEachObjVec( vNodes, p, pObj, i )
        pObj->Value = Gia_ManAppendAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
    pObj = Gia_ManCo(p, 0);
    if ( Gia_ObjFanin0(pObj) == Gia_ManConst0(p) )
    {
        Gia_ManAppendCo( pNew, 0 );
        Gia_ManAppendCo( pNew, Gia_ObjFaninC0(pObj) );
    }
    else
    {
        Gia_ManSetPhase( p );
        Gia_ManForEachObjVec( vOrder, p, pObj, i )
            Gia_ManAppendCo( pNew, Abc_LitNotCond(pObj->Value, pObj->fPhase) );
    }
    Vec_IntFree( vNodes );
    Vec_IntFree( vOrder );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Collect nodes reachable from odd/even outputs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManCollectDfs_rec( Gia_Man_t * p, int iObj, Vec_Int_t * vNodes )
{
    Gia_Obj_t * pObj;
    if ( Gia_ObjIsTravIdCurrentId(p, iObj) )
        return;
    Gia_ObjSetTravIdCurrentId(p, iObj);
    pObj = Gia_ManObj( p, iObj );
    if ( !Gia_ObjIsAnd(pObj) )
        return;
    Gia_ManCollectDfs_rec( p, Gia_ObjFaninId0(pObj, iObj), vNodes );
    Gia_ManCollectDfs_rec( p, Gia_ObjFaninId1(pObj, iObj), vNodes );
    Vec_IntPush( vNodes, iObj );
}
Vec_Int_t * Gia_ManCollectReach( Gia_Man_t * p, int fOdd )
{
    int i, iDriver;
    Vec_Int_t * vNodes = Vec_IntAlloc( Gia_ManObjNum(p) );
    Gia_ManIncrementTravId( p );
    Gia_ObjSetTravIdCurrent( p, Gia_ManConst0(p) );
    Gia_ManForEachCoDriverId( p, iDriver, i )
        if ( (i & 1) == fOdd )
            Gia_ManCollectDfs_rec( p, iDriver, vNodes );
    return vNodes;
}
int Gia_ManDemiterDual( Gia_Man_t * p, Gia_Man_t ** pp0, Gia_Man_t ** pp1 )
{
    Gia_Obj_t * pObj;
    int i, fOdd;
    assert( Gia_ManRegNum(p) == 0 );
    assert( Gia_ManCoNum(p) % 2 == 0 );
    *pp0 = *pp1 = NULL;
    for ( fOdd = 0; fOdd < 2; fOdd++ )
    {
        Vec_Int_t * vNodes = Gia_ManCollectReach( p, fOdd );
        Gia_Man_t * pNew = Gia_ManStart( 1 + Gia_ManCiNum(p) + Vec_IntSize(vNodes) + Gia_ManCoNum(p)/2 );
        pNew->pName = Abc_UtilStrsav( p->pName );
        pNew->pSpec = Abc_UtilStrsav( p->pSpec );
        Gia_ManConst0(p)->Value = 0;
        Gia_ManForEachPi( p, pObj, i )
            pObj->Value = Gia_ManAppendCi( pNew );
        Gia_ManForEachObjVec( vNodes, p, pObj, i )
            pObj->Value = Gia_ManAppendAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
        Gia_ManForEachCo( p, pObj, i )
            if ( (i & 1) == fOdd )
                Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
        Vec_IntFree( vNodes );
        if ( fOdd )
            *pp1 = pNew;
        else
            *pp0 = pNew;
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    [Collect nodes reachable from first/second half of outputs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Gia_ManCollectReach2( Gia_Man_t * p, int fSecond )
{
    int i, iDriver;
    Vec_Int_t * vNodes = Vec_IntAlloc( Gia_ManObjNum(p) );
    Gia_ManIncrementTravId( p );
    Gia_ObjSetTravIdCurrent( p, Gia_ManConst0(p) );
    Gia_ManForEachCoDriverId( p, iDriver, i )
        if ( (i < Gia_ManCoNum(p)/2) ^ fSecond )
            Gia_ManCollectDfs_rec( p, iDriver, vNodes );
    return vNodes;
}
int Gia_ManDemiterTwoWords( Gia_Man_t * p, Gia_Man_t ** pp0, Gia_Man_t ** pp1 )
{
    Gia_Obj_t * pObj;
    int i, fSecond;
    assert( Gia_ManRegNum(p) == 0 );
    assert( Gia_ManCoNum(p) % 2 == 0 );
    *pp0 = *pp1 = NULL;
    for ( fSecond = 0; fSecond < 2; fSecond++ )
    {
        Vec_Int_t * vNodes = Gia_ManCollectReach2( p, fSecond );
        Gia_Man_t * pNew = Gia_ManStart( 1 + Gia_ManCiNum(p) + Vec_IntSize(vNodes) + Gia_ManCoNum(p)/2 );
        pNew->pName = Abc_UtilStrsav( p->pName );
        pNew->pSpec = Abc_UtilStrsav( p->pSpec );
        Gia_ManConst0(p)->Value = 0;
        Gia_ManForEachPi( p, pObj, i )
            pObj->Value = Gia_ManAppendCi( pNew );
        Gia_ManForEachObjVec( vNodes, p, pObj, i )
            pObj->Value = Gia_ManAppendAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
        Gia_ManForEachCo( p, pObj, i )
            if ( (i < Gia_ManCoNum(p)/2) ^ fSecond )
                Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
        Vec_IntFree( vNodes );
        if ( fSecond )
            *pp1 = pNew;
        else
            *pp0 = pNew;
    }
    return 1;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

