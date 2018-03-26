/**CFile****************************************************************

  FileName    [giaIso3.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Scalable AIG package.]

  Synopsis    []

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: giaIso3.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "gia.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static unsigned Iso_Nodes[6] = { 0x04892ed6, 0xc2523d7d, 0xdc36cd2e, 0xf2db64f8, 0xde3126bb, 0xdebbdff0 }; // ab, a!b, !a!b, pi, po, const0
static unsigned Iso_Fanio[2] = { 0x855ee0cf, 0x946e1b5f }; // fanin, fanout
static unsigned Iso_Compl[2] = { 0x8ba63e50, 0x14d87f02 }; // non-compl, compl

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline unsigned Gia_Iso3Node( Gia_Obj_t * pObj )
{
    if ( Gia_ObjIsAnd(pObj) )
        return Iso_Nodes[Gia_ObjFaninC0(pObj) + Gia_ObjFaninC1(pObj)];
    if ( Gia_ObjIsCi(pObj) )
        return Iso_Nodes[3];
    if ( Gia_ObjIsCo(pObj) )
        return Iso_Nodes[4];
    return Iso_Nodes[5];
}
void Gia_Iso3Init( Gia_Man_t * p )
{
    Gia_Obj_t * pObj;
    int i;
    Gia_ManForEachObj( p, pObj, i )
        pObj->Value = Gia_Iso3Node( pObj );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Gia_Iso3ComputeEdge( Gia_Man_t * p, Gia_Obj_t * pObj, Gia_Obj_t * pFanin, int fCompl, Vec_Int_t * vSign )
{
    pObj->Value   += Vec_IntEntry(vSign, Gia_ObjId(p, pFanin)) + Iso_Compl[fCompl] + Iso_Fanio[0];
    pFanin->Value += Vec_IntEntry(vSign, Gia_ObjId(p, pObj))   + Iso_Compl[fCompl] + Iso_Fanio[1];
}
void Gia_Iso3Compute( Gia_Man_t * p, Vec_Int_t * vSign )
{
    Gia_Obj_t * pObj;
    int i;
    Gia_ManForEachObj( p, pObj, i )
    {
        if ( Gia_ObjIsAnd(pObj) || Gia_ObjIsCo(pObj) )
            Gia_Iso3ComputeEdge( p, pObj, Gia_ObjFanin0(pObj), Gia_ObjFaninC0(pObj), vSign );
        if ( Gia_ObjIsAnd(pObj) )
            Gia_Iso3ComputeEdge( p, pObj, Gia_ObjFanin1(pObj), Gia_ObjFaninC1(pObj), vSign );
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Vec_Int_t * Gia_Iso3Save( Gia_Man_t * p )
{
    Vec_Int_t * vSign;
    Gia_Obj_t * pObj;
    int i;
    vSign = Vec_IntAlloc( Gia_ManObjNum(p) );
    Gia_ManForEachObj( p, pObj, i )
        Vec_IntPush( vSign, pObj->Value );
    return vSign;
}
int Gia_Iso3Unique( Vec_Int_t * vSign )
{
    int nUnique;
    Vec_Int_t * vCopy = Vec_IntDup( vSign );
    Vec_IntUniqify( vCopy );
    nUnique = Vec_IntSize(vCopy);
    Vec_IntFree( vCopy );
    return nUnique;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_Iso3Test( Gia_Man_t * p )
{
    int nIterMax = 500;
    int i, Prev = -1, This; 
    abctime clk = Abc_Clock();
    Vec_Int_t * vSign = NULL;
    Gia_Iso3Init( p );
    for ( i = 0; i < nIterMax; i++ )
    {
        vSign = Gia_Iso3Save( p );
//        This = Gia_Iso3Unique( vSign );
        This = Vec_IntUniqueCount( vSign, 1, NULL );
        printf( "Iter %3d : %6d  out of %6d  ", i, This, Vec_IntSize(vSign) );
        Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
        if ( This == Prev )
            break;
        Prev = This;
        Gia_Iso3Compute( p, vSign );
        Vec_IntFreeP( &vSign );
    }
    Vec_IntFreeP( &vSign );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

