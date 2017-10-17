/**CFile****************************************************************

  FileName    [saigSimExt2.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Sequential AIG package.]

  Synopsis    [Extending simulation trace to contain ternary values.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: saigSimExt2.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "abs.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#define SAIG_ZER 1
#define SAIG_ONE 2
#define SAIG_UND 3

static inline int Saig_ManSimInfoNot( int Value )
{
    if ( Value == SAIG_ZER )
        return SAIG_ONE;
    if ( Value == SAIG_ONE )
        return SAIG_ZER;
    return SAIG_UND;
}

static inline int Saig_ManSimInfoAnd( int Value0, int Value1 )
{
    if ( Value0 == SAIG_ZER || Value1 == SAIG_ZER )
        return SAIG_ZER;
    if ( Value0 == SAIG_ONE && Value1 == SAIG_ONE )
        return SAIG_ONE;
    return SAIG_UND;
}

static inline int Saig_ManSimInfoGet( Vec_Ptr_t * vSimInfo, Aig_Obj_t * pObj, int iFrame )
{
    unsigned * pInfo = (unsigned *)Vec_PtrEntry( vSimInfo, Aig_ObjId(pObj) );
    return 3 & (pInfo[iFrame >> 4] >> ((iFrame & 15) << 1));
}

static inline void Saig_ManSimInfoSet( Vec_Ptr_t * vSimInfo, Aig_Obj_t * pObj, int iFrame, int Value )
{
    unsigned * pInfo = (unsigned *)Vec_PtrEntry( vSimInfo, Aig_ObjId(pObj) );
    assert( Value >= SAIG_ZER && Value <= SAIG_UND );
    Value ^= Saig_ManSimInfoGet( vSimInfo, pObj, iFrame );
    pInfo[iFrame >> 4] ^= (Value << ((iFrame & 15) << 1));
}



#define SAIG_ZER_NEW 0   // 0 not visited
#define SAIG_ONE_NEW 1   // 1 not visited
#define SAIG_ZER_OLD 2   // 0 visited
#define SAIG_ONE_OLD 3   // 1 visited

static inline int Saig_ManSimInfo2IsOld( int Value )
{
    return Value == SAIG_ZER_OLD || Value == SAIG_ONE_OLD;
}

static inline int Saig_ManSimInfo2SetOld( int Value )
{
    if ( Value == SAIG_ZER_NEW )
        return SAIG_ZER_OLD;
    if ( Value == SAIG_ONE_NEW )
        return SAIG_ONE_OLD;
    assert( 0 );
    return 0;
}

static inline int Saig_ManSimInfo2Not( int Value )
{
    if ( Value == SAIG_ZER_NEW )
        return SAIG_ONE_NEW;
    if ( Value == SAIG_ONE_NEW )
        return SAIG_ZER_NEW;
    if ( Value == SAIG_ZER_OLD )
        return SAIG_ONE_OLD;
    if ( Value == SAIG_ONE_OLD )
        return SAIG_ZER_OLD;
    assert( 0 );
    return 0;
}

static inline int Saig_ManSimInfo2And( int Value0, int Value1 )
{
    if ( Value0 == SAIG_ZER_NEW || Value1 == SAIG_ZER_NEW )
        return SAIG_ZER_NEW;
    if ( Value0 == SAIG_ONE_NEW && Value1 == SAIG_ONE_NEW )
        return SAIG_ONE_NEW;
    assert( 0 );
    return 0;
}

static inline int Saig_ManSimInfo2Get( Vec_Ptr_t * vSimInfo, Aig_Obj_t * pObj, int iFrame )
{
    unsigned * pInfo = (unsigned *)Vec_PtrEntry( vSimInfo, Aig_ObjId(pObj) );
    return 3 & (pInfo[iFrame >> 4] >> ((iFrame & 15) << 1));
}

static inline void Saig_ManSimInfo2Set( Vec_Ptr_t * vSimInfo, Aig_Obj_t * pObj, int iFrame, int Value )
{
    unsigned * pInfo = (unsigned *)Vec_PtrEntry( vSimInfo, Aig_ObjId(pObj) );
    Value ^= Saig_ManSimInfo2Get( vSimInfo, pObj, iFrame );
    pInfo[iFrame >> 4] ^= (Value << ((iFrame & 15) << 1));
}

// performs ternary simulation
//extern int Saig_ManSimDataInit( Aig_Man_t * p, Abc_Cex_t * pCex, Vec_Ptr_t * vSimInfo, Vec_Int_t * vRes );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Performs ternary simulation for one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Saig_ManExtendOneEval( Vec_Ptr_t * vSimInfo, Aig_Obj_t * pObj, int iFrame )
{
    int Value0, Value1, Value;
    Value0 = Saig_ManSimInfoGet( vSimInfo, Aig_ObjFanin0(pObj), iFrame );
    if ( Aig_ObjFaninC0(pObj) )
        Value0 = Saig_ManSimInfoNot( Value0 );
    if ( Aig_ObjIsCo(pObj) )
    {
        Saig_ManSimInfoSet( vSimInfo, pObj, iFrame, Value0 );
        return Value0;
    }
    assert( Aig_ObjIsNode(pObj) );
    Value1 = Saig_ManSimInfoGet( vSimInfo, Aig_ObjFanin1(pObj), iFrame );
    if ( Aig_ObjFaninC1(pObj) )
        Value1 = Saig_ManSimInfoNot( Value1 );
    Value = Saig_ManSimInfoAnd( Value0, Value1 );
    Saig_ManSimInfoSet( vSimInfo, pObj, iFrame, Value );
    return Value;
}

/**Function*************************************************************

  Synopsis    [Performs ternary simulation for one design.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Saig_ManSimDataInit( Aig_Man_t * p, Abc_Cex_t * pCex, Vec_Ptr_t * vSimInfo, Vec_Int_t * vRes )
{
    Aig_Obj_t * pObj, * pObjLi, * pObjLo;
    int i, f, Entry, iBit = 0;
    Saig_ManForEachLo( p, pObj, i )
        Saig_ManSimInfoSet( vSimInfo, pObj, 0, Abc_InfoHasBit(pCex->pData, iBit++)?SAIG_ONE:SAIG_ZER );
    for ( f = 0; f <= pCex->iFrame; f++ )
    {
        Saig_ManSimInfoSet( vSimInfo, Aig_ManConst1(p), f, SAIG_ONE );
        Saig_ManForEachPi( p, pObj, i )
            Saig_ManSimInfoSet( vSimInfo, pObj, f, Abc_InfoHasBit(pCex->pData, iBit++)?SAIG_ONE:SAIG_ZER );
        if ( vRes )
        Vec_IntForEachEntry( vRes, Entry, i )
            Saig_ManSimInfoSet( vSimInfo, Aig_ManCi(p, Entry), f, SAIG_UND );
        Aig_ManForEachNode( p, pObj, i )
            Saig_ManExtendOneEval( vSimInfo, pObj, f );
        Aig_ManForEachCo( p, pObj, i )
            Saig_ManExtendOneEval( vSimInfo, pObj, f );
        if ( f == pCex->iFrame )
            break;
        Saig_ManForEachLiLo( p, pObjLi, pObjLo, i )
            Saig_ManSimInfoSet( vSimInfo, pObjLo, f+1, Saig_ManSimInfoGet(vSimInfo, pObjLi, f) );
    }
    // make sure the output of the property failed
    pObj = Aig_ManCo( p, pCex->iPo );
    return Saig_ManSimInfoGet( vSimInfo, pObj, pCex->iFrame );
}

/**Function*************************************************************

  Synopsis    [Performs ternary simulation for one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Saig_ManExtendOneEval2( Vec_Ptr_t * vSimInfo, Aig_Obj_t * pObj, int iFrame )
{
    int Value0, Value1, Value;
    Value0 = Saig_ManSimInfo2Get( vSimInfo, Aig_ObjFanin0(pObj), iFrame );
    if ( Aig_ObjFaninC0(pObj) )
        Value0 = Saig_ManSimInfo2Not( Value0 );
    if ( Aig_ObjIsCo(pObj) )
    {
        Saig_ManSimInfo2Set( vSimInfo, pObj, iFrame, Value0 );
        return Value0;
    }
    assert( Aig_ObjIsNode(pObj) );
    Value1 = Saig_ManSimInfo2Get( vSimInfo, Aig_ObjFanin1(pObj), iFrame );
    if ( Aig_ObjFaninC1(pObj) )
        Value1 = Saig_ManSimInfo2Not( Value1 );
    Value = Saig_ManSimInfo2And( Value0, Value1 );
    Saig_ManSimInfo2Set( vSimInfo, pObj, iFrame, Value );
    return Value;
}

/**Function*************************************************************

  Synopsis    [Performs sensitization analysis for one design.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Saig_ManSimDataInit2( Aig_Man_t * p, Abc_Cex_t * pCex, Vec_Ptr_t * vSimInfo )
{
    Aig_Obj_t * pObj, * pObjLi, * pObjLo;
    int i, f, iBit = 0;
    Saig_ManForEachLo( p, pObj, i )
        Saig_ManSimInfo2Set( vSimInfo, pObj, 0, Abc_InfoHasBit(pCex->pData, iBit++)?SAIG_ONE_NEW:SAIG_ZER_NEW );
    for ( f = 0; f <= pCex->iFrame; f++ )
    {
        Saig_ManSimInfo2Set( vSimInfo, Aig_ManConst1(p), f, SAIG_ONE_NEW );
        Saig_ManForEachPi( p, pObj, i )
            Saig_ManSimInfo2Set( vSimInfo, pObj, f, Abc_InfoHasBit(pCex->pData, iBit++)?SAIG_ONE_NEW:SAIG_ZER_NEW );
        Aig_ManForEachNode( p, pObj, i )
            Saig_ManExtendOneEval2( vSimInfo, pObj, f );
        Aig_ManForEachCo( p, pObj, i )
            Saig_ManExtendOneEval2( vSimInfo, pObj, f );
        if ( f == pCex->iFrame )
            break;
        Saig_ManForEachLiLo( p, pObjLi, pObjLo, i )
            Saig_ManSimInfo2Set( vSimInfo, pObjLo, f+1, Saig_ManSimInfo2Get(vSimInfo, pObjLi, f) );
    }
    // make sure the output of the property failed
    pObj = Aig_ManCo( p, pCex->iPo );
    return Saig_ManSimInfo2Get( vSimInfo, pObj, pCex->iFrame );
}

/**Function*************************************************************

  Synopsis    [Drive implications of the given node towards primary outputs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Saig_ManSetAndDriveImplications_rec( Aig_Man_t * p, Aig_Obj_t * pObj, int f, int fMax, Vec_Ptr_t * vSimInfo )
{
    Aig_Obj_t * pFanout;
    int k, iFanout = -1, Value0, Value1;
    int Value = Saig_ManSimInfo2Get( vSimInfo, pObj, f );
    assert( !Saig_ManSimInfo2IsOld( Value ) );
    Saig_ManSimInfo2Set( vSimInfo, pObj, f, Saig_ManSimInfo2SetOld(Value) );
    if ( (Aig_ObjIsCo(pObj) && f == fMax) || Saig_ObjIsPo(p, pObj) )
        return;
    if ( Saig_ObjIsLi( p, pObj ) )
    {
        assert( f < fMax );
        pFanout = Saig_ObjLiToLo(p, pObj);
        Value = Saig_ManSimInfo2Get( vSimInfo, pFanout, f+1 );
        if ( !Saig_ManSimInfo2IsOld( Value ) )
            Saig_ManSetAndDriveImplications_rec( p, pFanout, f+1, fMax, vSimInfo );
        return;
    }
    assert( Aig_ObjIsCi(pObj) || Aig_ObjIsNode(pObj) || Aig_ObjIsConst1(pObj) );
    Aig_ObjForEachFanout( p, pObj, pFanout, iFanout, k )
    {
        Value = Saig_ManSimInfo2Get( vSimInfo, pFanout, f );
        if ( Saig_ManSimInfo2IsOld( Value ) )
            continue;
        if ( Aig_ObjIsCo(pFanout) )
        {
            Saig_ManSetAndDriveImplications_rec( p, pFanout, f, fMax, vSimInfo );
            continue;
        }
        assert( Aig_ObjIsNode(pFanout) );
        Value0 = Saig_ManSimInfo2Get( vSimInfo, Aig_ObjFanin0(pFanout), f );
        Value1 = Saig_ManSimInfo2Get( vSimInfo, Aig_ObjFanin1(pFanout), f );
        if ( Aig_ObjFaninC0(pFanout) )
            Value0 = Saig_ManSimInfo2Not( Value0 );
        if ( Aig_ObjFaninC1(pFanout) )
            Value1 = Saig_ManSimInfo2Not( Value1 );
        if ( Value0 == SAIG_ZER_OLD || Value1 == SAIG_ZER_OLD || 
            (Value0 == SAIG_ONE_OLD && Value1 == SAIG_ONE_OLD) )
            Saig_ManSetAndDriveImplications_rec( p, pFanout, f, fMax, vSimInfo );
    }
}

/**Function*************************************************************

  Synopsis    [Performs recursive sensetization analysis.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Saig_ManExplorePaths_rec( Aig_Man_t * p, Aig_Obj_t * pObj, int f, int fMax, Vec_Ptr_t * vSimInfo )
{
    int Value = Saig_ManSimInfo2Get( vSimInfo, pObj, f );
    if ( Saig_ManSimInfo2IsOld( Value ) )
        return;
    Saig_ManSetAndDriveImplications_rec( p, pObj, f, fMax, vSimInfo );
    assert( !Aig_ObjIsConst1(pObj) );
    if ( Saig_ObjIsLo(p, pObj) && f == 0 )
        return;
    if ( Saig_ObjIsPi(p, pObj) )
    {
        // propagate implications of this assignment
        int i, iPiNum = Aig_ObjCioId(pObj);
        for ( i = fMax; i >= 0; i-- )
            if ( i != f )
                Saig_ManSetAndDriveImplications_rec( p, Aig_ManCi(p, iPiNum), i, fMax, vSimInfo );
        return;
    }
    if ( Saig_ObjIsLo( p, pObj ) )
    {
        assert( f > 0 );
        Saig_ManExplorePaths_rec( p, Saig_ObjLoToLi(p, pObj), f-1, fMax, vSimInfo );
        return;
    }
    if ( Aig_ObjIsCo(pObj) )
    {
        Saig_ManExplorePaths_rec( p, Aig_ObjFanin0(pObj), f, fMax, vSimInfo );
        return;
    }
    assert( Aig_ObjIsNode(pObj) );
    if ( Value == SAIG_ZER_OLD )
    {
//        if ( (Aig_ObjId(pObj) & 1) == 0 )
            Saig_ManExplorePaths_rec( p, Aig_ObjFanin0(pObj), f, fMax, vSimInfo );
//        else
//            Saig_ManExplorePaths_rec( p, Aig_ObjFanin1(pObj), f, fMax, vSimInfo );
    }
    else
    {
        Saig_ManExplorePaths_rec( p, Aig_ObjFanin0(pObj), f, fMax, vSimInfo );
        Saig_ManExplorePaths_rec( p, Aig_ObjFanin1(pObj), f, fMax, vSimInfo );
    }
}

/**Function*************************************************************

  Synopsis    [Returns the array of PIs for flops that should not be absracted.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Saig_ManProcessCex( Aig_Man_t * p, int iFirstFlopPi, Abc_Cex_t * pCex, Vec_Ptr_t * vSimInfo, int fVerbose )
{
    Aig_Obj_t * pObj;
    Vec_Int_t * vRes, * vResInv;
    int i, f, Value;
//    assert( Aig_ManRegNum(p) > 0 );
    assert( (unsigned *)Vec_PtrEntry(vSimInfo,1) - (unsigned *)Vec_PtrEntry(vSimInfo,0) >= Abc_BitWordNum(2*(pCex->iFrame+1)) );
    // start simulation data
    Value = Saig_ManSimDataInit2( p, pCex, vSimInfo );
    assert( Value == SAIG_ONE_NEW );
    // derive implications of constants and primary inputs
    Saig_ManForEachLo( p, pObj, i )
        Saig_ManSetAndDriveImplications_rec( p, pObj, 0, pCex->iFrame, vSimInfo );
    for ( f = pCex->iFrame; f >= 0; f-- )
    {
        Saig_ManSetAndDriveImplications_rec( p, Aig_ManConst1(p), f, pCex->iFrame, vSimInfo );
        for ( i = 0; i < iFirstFlopPi; i++ )
            Saig_ManSetAndDriveImplications_rec( p, Aig_ManCi(p, i), f, pCex->iFrame, vSimInfo );
    }
    // recursively compute justification
    Saig_ManExplorePaths_rec( p, Aig_ManCo(p, pCex->iPo), pCex->iFrame, pCex->iFrame, vSimInfo );
    // select the result
    vRes = Vec_IntAlloc( 1000 );
    vResInv = Vec_IntAlloc( 1000 );
    for ( i = iFirstFlopPi; i < Saig_ManPiNum(p); i++ )
    {
        for ( f = pCex->iFrame; f >= 0; f-- )
        {
            Value = Saig_ManSimInfo2Get( vSimInfo, Aig_ManCi(p, i), f );
            if ( Saig_ManSimInfo2IsOld( Value ) )
                break;
        }
        if ( f >= 0 )
            Vec_IntPush( vRes, i );
        else
            Vec_IntPush( vResInv, i );
    }
    // resimulate to make sure it is valid
    Value = Saig_ManSimDataInit( p, pCex, vSimInfo, vResInv );
    assert( Value == SAIG_ONE );
    Vec_IntFree( vResInv );
    return vRes;
}

/**Function*************************************************************

  Synopsis    [Returns the array of PIs for flops that should not be absracted.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Saig_ManExtendCounterExampleTest2( Aig_Man_t * p, int iFirstFlopPi, Abc_Cex_t * pCex, int fVerbose )
{
    Vec_Int_t * vRes;
    Vec_Ptr_t * vSimInfo;
    abctime clk;
    if ( Saig_ManPiNum(p) != pCex->nPis )
    {
        printf( "Saig_ManExtendCounterExampleTest2(): The PI count of AIG (%d) does not match that of cex (%d).\n", 
            Aig_ManCiNum(p), pCex->nPis );
        return NULL;
    }
    Aig_ManFanoutStart( p );
    vSimInfo = Vec_PtrAllocSimInfo( Aig_ManObjNumMax(p), Abc_BitWordNum(2*(pCex->iFrame+1)) );
    Vec_PtrCleanSimInfo( vSimInfo, 0, Abc_BitWordNum(2*(pCex->iFrame+1)) );

clk = Abc_Clock();
    vRes = Saig_ManProcessCex( p, iFirstFlopPi, pCex, vSimInfo, fVerbose );
    if ( fVerbose )
    {
        printf( "Total new PIs = %3d. Non-removable PIs = %3d.  ", Saig_ManPiNum(p)-iFirstFlopPi, Vec_IntSize(vRes) );
ABC_PRT( "Time", Abc_Clock() - clk );
    }
    Vec_PtrFree( vSimInfo );
    Aig_ManFanoutStop( p );
    return vRes;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

