/**CFile****************************************************************
 
  FileName    [aigJust.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [AIG package.]

  Synopsis    [Justification of node values.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - April 28, 2007.]

  Revision    [$Id: aigJust.c,v 1.00 2007/04/28 00:00:00 alanmi Exp $]

***********************************************************************/

#include "aig.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#define AIG_VAL0  1
#define AIG_VAL1  2
#define AIG_VALX  3

// storing ternary values
static inline int Aig_ObjSetTerValue( Aig_Obj_t * pNode, int Value )
{
    assert( Value );
    pNode->fMarkA = (Value & 1);
    pNode->fMarkB = ((Value >> 1) & 1);
    return Value;
}
static inline int Aig_ObjGetTerValue( Aig_Obj_t * pNode )
{
    return (pNode->fMarkB << 1) | pNode->fMarkA;
}

// working with ternary values
static inline int Aig_ObjNotTerValue( int Value )
{
    if ( Value == AIG_VAL1 )
        return AIG_VAL0;
    if ( Value == AIG_VAL0 )
        return AIG_VAL1;
    return AIG_VALX;
}
static inline int Aig_ObjAndTerValue( int Value0, int Value1 )
{
    if ( Value0 == AIG_VAL0 || Value1 == AIG_VAL0 )
        return AIG_VAL0;
    if ( Value0 == AIG_VAL1 && Value1 == AIG_VAL1 )
        return AIG_VAL1;
    return AIG_VALX;
}
static inline int Aig_ObjNotCondTerValue( int Value, int Cond )
{
    return Cond ? Aig_ObjNotTerValue( Value ) : Value;
}

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Returns value (0 or 1) or X if unassigned.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Aig_ObjSatValue( Aig_Man_t * pAig, Aig_Obj_t * pNode, int fCompl )
{
    if ( Aig_ObjIsTravIdCurrent(pAig, pNode) )
        return (pNode->fMarkA ^ fCompl) ? AIG_VAL1 : AIG_VAL0;
    return AIG_VALX;
}

/**Function*************************************************************

  Synopsis    [Recursively searched for a satisfying assignment.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aig_NtkFindSatAssign_rec( Aig_Man_t * pAig, Aig_Obj_t * pNode, int Value, Vec_Int_t * vSuppLits, int Heur )
{
    int Value0, Value1;
//    ++Heur;
    if ( Aig_ObjIsConst1(pNode) )
        return 1;
    if ( Aig_ObjIsTravIdCurrent(pAig, pNode) )
        return ((int)pNode->fMarkA == Value);
    Aig_ObjSetTravIdCurrent(pAig, pNode);
    pNode->fMarkA = Value;
    if ( Aig_ObjIsCi(pNode) )
    {
//        if ( Aig_ObjId(pNode) % 1000 == 0 )
//            Value ^= 1;
        if ( vSuppLits )
            Vec_IntPush( vSuppLits, Abc_Var2Lit( Aig_ObjCioId(pNode), !Value ) );
        return 1;
    }
    assert( Aig_ObjIsNode(pNode) );
    // propagation
    if ( Value ) 
    {
        if ( !Aig_NtkFindSatAssign_rec(pAig, Aig_ObjFanin0(pNode), !Aig_ObjFaninC0(pNode), vSuppLits, Heur) )
            return 0;
        return Aig_NtkFindSatAssign_rec(pAig, Aig_ObjFanin1(pNode), !Aig_ObjFaninC1(pNode), vSuppLits, Heur);
    }
    // justification
    Value0 = Aig_ObjSatValue( pAig, Aig_ObjFanin0(pNode), Aig_ObjFaninC0(pNode) );
    if ( Value0 == AIG_VAL0 )
        return 1;
    Value1 = Aig_ObjSatValue( pAig, Aig_ObjFanin1(pNode), Aig_ObjFaninC1(pNode) );
    if ( Value1 == AIG_VAL0 )
        return 1;
    if ( Value0 == AIG_VAL1 && Value1 == AIG_VAL1 )
        return 0;
    if ( Value0 == AIG_VAL1 )
        return Aig_NtkFindSatAssign_rec(pAig, Aig_ObjFanin1(pNode), Aig_ObjFaninC1(pNode), vSuppLits, Heur);
    if ( Value1 == AIG_VAL1 )
        return Aig_NtkFindSatAssign_rec(pAig, Aig_ObjFanin0(pNode), Aig_ObjFaninC0(pNode), vSuppLits, Heur);
    assert( Value0 == AIG_VALX && Value1 == AIG_VALX );
    // decision making
//    if ( rand() % 10 == Heur )
//    if ( Aig_ObjId(pNode) % 8 == Heur )
    if ( ++Heur % 8 == 0 )
        return Aig_NtkFindSatAssign_rec(pAig, Aig_ObjFanin1(pNode), Aig_ObjFaninC1(pNode), vSuppLits, Heur);
    else
        return Aig_NtkFindSatAssign_rec(pAig, Aig_ObjFanin0(pNode), Aig_ObjFaninC0(pNode), vSuppLits, Heur);
}

/**Function*************************************************************

  Synopsis    [Returns 1 if SAT assignment is found; 0 otherwise.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aig_ObjFindSatAssign( Aig_Man_t * pAig, Aig_Obj_t * pNode, int Value, Vec_Int_t * vSuppLits )
{
    int i;
    if ( Aig_ObjIsCo(pNode) )
        return Aig_ObjFindSatAssign( pAig, Aig_ObjFanin0(pNode), Value ^ Aig_ObjFaninC0(pNode), vSuppLits );
    for ( i = 0; i < 8; i++ )
    {
        Vec_IntClear( vSuppLits );
        Aig_ManIncrementTravId( pAig );
        if ( Aig_NtkFindSatAssign_rec( pAig, pNode, Value, vSuppLits, i ) )
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
int Aig_ObjTerSimulate_rec( Aig_Man_t * pAig, Aig_Obj_t * pNode )
{
    int Value0, Value1;
    if ( Aig_ObjIsConst1(pNode) )
        return AIG_VAL1;
    if ( Aig_ObjIsTravIdCurrent(pAig, pNode) )
        return Aig_ObjGetTerValue( pNode );
    Aig_ObjSetTravIdCurrent( pAig, pNode );
    if ( Aig_ObjIsCi(pNode) )
        return Aig_ObjSetTerValue( pNode, AIG_VALX );
    Value0 = Aig_ObjNotCondTerValue( Aig_ObjTerSimulate_rec(pAig, Aig_ObjFanin0(pNode)), Aig_ObjFaninC0(pNode) );
    if ( Aig_ObjIsCo(pNode) || Value0 == AIG_VAL0 )
        return Aig_ObjSetTerValue( pNode, Value0 );
    assert( Aig_ObjIsNode(pNode) );
    Value1 = Aig_ObjNotCondTerValue( Aig_ObjTerSimulate_rec(pAig, Aig_ObjFanin1(pNode)), Aig_ObjFaninC1(pNode) );
    return Aig_ObjSetTerValue( pNode, Aig_ObjAndTerValue(Value0, Value1) );
}

/**Function*************************************************************

  Synopsis    [Returns value of the object under input assignment.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aig_ObjTerSimulate( Aig_Man_t * pAig, Aig_Obj_t * pNode, Vec_Int_t * vSuppLits )
{
    Aig_Obj_t * pObj;
    int i, Entry;
    Aig_ManIncrementTravId( pAig );
    Vec_IntForEachEntry( vSuppLits, Entry, i )
    {
        pObj = Aig_ManCi( pAig, Abc_Lit2Var(Entry) );
        Aig_ObjSetTerValue( pObj, Abc_LitIsCompl(Entry) ? AIG_VAL0 : AIG_VAL1 );
        Aig_ObjSetTravIdCurrent( pAig, pObj );
//printf( "%d ", Entry );
    }
//printf( "\n" );
    return Aig_ObjTerSimulate_rec( pAig, pNode );
}


typedef struct Aig_ManPack_t_ Aig_ManPack_t;
extern Aig_ManPack_t *        Aig_ManPackStart( Aig_Man_t * pAig );
extern void                   Aig_ManPackStop( Aig_ManPack_t * p );
extern void                   Aig_ManPackAddPattern( Aig_ManPack_t * p, Vec_Int_t * vLits );
extern Vec_Int_t *            Aig_ManPackConstNodes( Aig_ManPack_t * p );

/**Function*************************************************************

  Synopsis    [Testing of the framework.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ManJustExperiment( Aig_Man_t * pAig )
{
    Aig_ManPack_t * pPack;
    Vec_Int_t * vSuppLits, * vNodes;
    Aig_Obj_t * pObj;
    int i;
    abctime clk = Abc_Clock();
    int Count0 = 0, Count0f = 0, Count1 = 0, Count1f = 0;
    int nTotalLits = 0;
    vSuppLits = Vec_IntAlloc( 100 );
    pPack = Aig_ManPackStart( pAig );
    vNodes = Aig_ManPackConstNodes( pPack );
//    Aig_ManForEachCo( pAig, pObj, i )
    Aig_ManForEachObjVec( vNodes, pAig, pObj, i )
    {
        if ( pObj->fPhase ) // const 1
        {
            if ( Aig_ObjFindSatAssign(pAig, pObj, 0, vSuppLits) )
            {
//                assert( Aig_ObjTerSimulate(pAig, pObj, vSuppLits) == AIG_VAL0 );
//                if ( Aig_ObjTerSimulate(pAig, pObj, vSuppLits) != AIG_VAL0 )
//                    printf( "Justification error!\n" );
                Count0++;
                nTotalLits += Vec_IntSize(vSuppLits);
                Aig_ManPackAddPattern( pPack, vSuppLits );
            }
            else
                Count0f++;
        }
        else
        {
            if ( Aig_ObjFindSatAssign(pAig, pObj, 1, vSuppLits) )
            {
//                assert( Aig_ObjTerSimulate(pAig, pObj, vSuppLits) == AIG_VAL1 );
//                if ( Aig_ObjTerSimulate(pAig, pObj, vSuppLits) != AIG_VAL1 )
//                    printf( "Justification error!\n" );
                Count1++;
                nTotalLits += Vec_IntSize(vSuppLits);
                Aig_ManPackAddPattern( pPack, vSuppLits );
            }
            else
                Count1f++;
        }
    }
    Vec_IntFree( vSuppLits );
    printf( "PO =%6d. C0 =%6d. C0f =%6d. C1 =%6d. C1f =%6d. (%6.2f %%) Ave =%4.1f ", 
        Aig_ManCoNum(pAig), Count0, Count0f, Count1, Count1f, 100.0*(Count0+Count1)/Aig_ManCoNum(pAig), 1.0*nTotalLits/(Count0+Count1) );
    Abc_PrintTime( 1, "T", Abc_Clock() - clk );
    Aig_ManCleanMarkAB( pAig );
    Aig_ManPackStop( pPack );
    Vec_IntFree( vNodes );
}







////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

