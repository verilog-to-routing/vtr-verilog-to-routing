/**CFile****************************************************************

  FileName    [acecNorm.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [CEC for arithmetic circuits.]

  Synopsis    [Adder tree normalization.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: acecNorm.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "acecInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Acec_InsertHadd( Gia_Man_t * pNew, int In[2], int Out[2] )
{
    int And, Or;
    Out[1] = Gia_ManAppendAnd2( pNew, In[0], In[1] );
    And    = Gia_ManAppendAnd2( pNew, Abc_LitNot(In[0]), Abc_LitNot(In[1]) );
    Or     = Gia_ManAppendOr2( pNew, Out[1], And );
    Out[0] = Abc_LitNot( Or );
}
void Acec_InsertFadd( Gia_Man_t * pNew, int In[3], int Out[2] )
{
    int In2[2], Out1[2], Out2[2];
    Acec_InsertHadd( pNew, In, Out1 );
    In2[0] = Out1[0];
    In2[1] = In[2];
    Acec_InsertHadd( pNew, In2, Out2 );
    Out[0] = Out2[0];
    Out[1] = Gia_ManAppendOr2( pNew, Out1[1], Out2[1] );
}
Vec_Int_t * Acec_InsertTree( Gia_Man_t * pNew, Vec_Wec_t * vLeafMap )
{
    Vec_Int_t * vRootRanks = Vec_IntAlloc( Vec_WecSize(vLeafMap) + 5 );
    Vec_Int_t * vLevel;
    int i, In[3], Out[2];
    Vec_WecForEachLevel( vLeafMap, vLevel, i )
    {
        if ( Vec_IntSize(vLevel) == 0 )
        {
            Vec_IntPush( vRootRanks, 0 );
            continue;
        }
        while ( Vec_IntSize(vLevel) > 1 )
        {
            if ( Vec_IntSize(vLevel) == 2 )
                Vec_IntPush( vLevel, 0 );
            //In[2] = Vec_IntPop( vLevel );
            //In[1] = Vec_IntPop( vLevel );
            //In[0] = Vec_IntPop( vLevel );

            In[0] = Vec_IntEntry( vLevel, 0 );
            Vec_IntDrop( vLevel, 0 );

            In[1] = Vec_IntEntry( vLevel, 0 );
            Vec_IntDrop( vLevel, 0 );

            In[2] = Vec_IntEntry( vLevel, 0 );
            Vec_IntDrop( vLevel, 0 );

            Acec_InsertFadd( pNew, In, Out );
            Vec_IntPush( vLevel, Out[0] );
            if ( i+1 < Vec_WecSize(vLeafMap) )
                vLevel = Vec_WecEntry(vLeafMap, i+1);
            else
                vLevel = Vec_WecPushLevel(vLeafMap);
            Vec_IntPush( vLevel, Out[1] );
            vLevel = Vec_WecEntry(vLeafMap, i);
        }
        assert( Vec_IntSize(vLevel) == 1 );
        Vec_IntPush( vRootRanks, Vec_IntEntry(vLevel, 0) );
    }
    return vRootRanks;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Acec_InsertBox_rec( Gia_Man_t * pNew, Gia_Man_t * p, Gia_Obj_t * pObj )
{
    if ( ~pObj->Value )
        return pObj->Value;
    assert( Gia_ObjIsAnd(pObj) );
    Acec_InsertBox_rec( pNew, p, Gia_ObjFanin0(pObj) );
    Acec_InsertBox_rec( pNew, p, Gia_ObjFanin1(pObj) );
    return (pObj->Value = Gia_ManAppendAnd2( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) ));
}
Vec_Int_t * Acec_BuildTree( Gia_Man_t * pNew, Gia_Man_t * p, Vec_Wec_t * vLeafLits, Vec_Int_t * vRootLits )
{
    Vec_Wec_t * vLeafMap = Vec_WecStart( Vec_WecSize(vLeafLits) );
    Vec_Int_t * vLevel, * vRootRanks;  
    int i, k, iLit, iLitNew;
    // add roo literals
    if ( vRootLits )
        Vec_IntForEachEntry( vRootLits, iLit, i )
        {
            if ( i < Vec_WecSize(vLeafMap) )
                vLevel = Vec_WecEntry(vLeafMap, i);
            else
                vLevel = Vec_WecPushLevel(vLeafMap);
            Vec_IntPush( vLevel, iLit );
        }
    // add other literals
    Vec_WecForEachLevel( vLeafLits, vLevel, i )
        Vec_IntForEachEntry( vLevel, iLit, k )
        {
            Gia_Obj_t * pObj = Gia_ManObj( p, Abc_Lit2Var(iLit) );
            iLitNew = Acec_InsertBox_rec( pNew, p, pObj );
            iLitNew = Abc_LitNotCond( iLitNew, Abc_LitIsCompl(iLit) );
            Vec_WecPush( vLeafMap, i, iLitNew );
        }
    // construct map of root literals
    vRootRanks = Acec_InsertTree( pNew, vLeafMap );
    Vec_WecFree( vLeafMap );
    return vRootRanks;
}
Gia_Man_t * Acec_InsertBox( Acec_Box_t * pBox, int fAll )
{
    Gia_Man_t * p = pBox->pGia;
    Gia_Man_t * pNew;
    Gia_Obj_t * pObj;
    Vec_Int_t * vRootRanks, * vLevel, * vTemp;
    int i, k, iLit, iLitNew;
    pNew = Gia_ManStart( Gia_ManObjNum(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    Gia_ManFillValue(p);
    Gia_ManConst0(p)->Value = 0;
    Gia_ManForEachCi( p, pObj, i )
        pObj->Value = Gia_ManAppendCi( pNew );
    // implement tree
    if ( fAll )
        vRootRanks = Acec_BuildTree( pNew, p, pBox->vLeafLits, NULL );
    else
    {
        assert( pBox->vShared != NULL );
        assert( pBox->vUnique != NULL );
        vRootRanks = Acec_BuildTree( pNew, p, pBox->vShared, NULL );
        vRootRanks = Acec_BuildTree( pNew, p, pBox->vUnique, vTemp = vRootRanks );
        Vec_IntFree( vTemp );
    }
    // update polarity of literals
    Vec_WecForEachLevel( pBox->vRootLits, vLevel, i )
        Vec_IntForEachEntry( vLevel, iLit, k )
        {
            pObj = Gia_ManObj( p, Abc_Lit2Var(iLit) );
            iLitNew = k ? 0 : Vec_IntEntry( vRootRanks, i );
            pObj->Value = Abc_LitNotCond( iLitNew, Abc_LitIsCompl(iLit) );
        }
    Vec_IntFree( vRootRanks );
    // construct the outputs
    Gia_ManForEachCo( p, pObj, i )
        Acec_InsertBox_rec( pNew, p, Gia_ObjFanin0(pObj) );
    Gia_ManForEachCo( p, pObj, i )
        pObj->Value = Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
    Gia_ManSetRegNum( pNew, Gia_ManRegNum(p) );
    return pNew;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Acec_Normalize( Gia_Man_t * pGia, int fBooth, int fVerbose )
{
    Vec_Bit_t * vIgnore = fBooth ? Acec_BoothFindPPG( pGia ) : NULL;
    Acec_Box_t * pBox = Acec_DeriveBox( pGia, vIgnore, 0, 0, fVerbose );
    Gia_Man_t * pNew  = Acec_InsertBox( pBox, 1 );
    Acec_BoxFreeP( &pBox );
    Vec_BitFreeP( &vIgnore );
    return pNew;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

