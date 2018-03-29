/**CFile****************************************************************

  FileName    [cecSimBack.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Combinational equivalence checking.]

  Synopsis    [Backward simulation.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: cecSimBack.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "cecInt.h"
#include "aig/gia/giaAig.h"

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
int Cec_ObjSatVerify( Gia_Man_t * p, Gia_Obj_t * pRoot, int Value )
{
    Gia_Obj_t * pObj;
    int i, RetValue = 1;
    printf( "Obj = %4d  Value = %d  ", Gia_ObjId(p, pRoot), Value );
    Gia_ObjTerSimSet0( Gia_ManConst0(p) );
    Gia_ManForEachCi( p, pObj, i )
        if ( !Gia_ObjIsTravIdCurrent(p, pObj) )
            Gia_ObjTerSimSetX( pObj ), printf( "x" );
        else if ( pObj->fMark0 )
            Gia_ObjTerSimSet1( pObj ), printf( "1" );
        else
            Gia_ObjTerSimSet0( pObj ), printf( "0" );
    printf( " " );
    Gia_ManForEachAnd( p, pObj, i )
        Gia_ObjTerSimAnd( pObj );
    if ( Value ? Gia_ObjTerSimGet1(pRoot) : Gia_ObjTerSimGet0(pRoot) )
        printf( "Verification successful.\n" );
    else
        printf( "Verification failed.\n" ), RetValue = 0;
    return RetValue;
}

/**Function*************************************************************

  Synopsis    [Return 1 if pObj can have Value.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
word Cec_ManCheckSat2_rec( Gia_Man_t * p, Gia_Obj_t * pObj, word Value, Vec_Wrd_t * vValues )
{
    Gia_Obj_t * pFan0, * pFan1;
    if ( Gia_ObjIsTravIdCurrent(p, pObj) )
        return Value == (int)pObj->fMark0;
    Gia_ObjSetTravIdCurrent(p, pObj);
    pObj->fMark0 = Value;
    if ( !Gia_ObjIsAnd(pObj) )
        return 1;
    pFan0 = Gia_ObjFanin0(pObj);
    pFan1 = Gia_ObjFanin1(pObj);
    if ( Value )
    {
        return Cec_ManCheckSat2_rec( p, pFan0, !Gia_ObjFaninC0(pObj), vValues ) &&
               Cec_ManCheckSat2_rec( p, pFan1, !Gia_ObjFaninC1(pObj), vValues );
    }
    if ( Gia_ObjIsTravIdCurrent(p, pFan0) ) // already assigned
    {
        if ( Gia_ObjFaninC0(pObj) == (int)pFan0->fMark0 ) // justified
            return 1;
        return Cec_ManCheckSat2_rec( p, pFan1, Gia_ObjFaninC1(pObj), vValues );        
    }
    if ( Gia_ObjIsTravIdCurrent(p, pFan1) ) // already assigned
    {
        if ( Gia_ObjFaninC1(pObj) == (int)pFan1->fMark0 ) // justified
            return 1;
        return Cec_ManCheckSat2_rec( p, pFan0, Gia_ObjFaninC0(pObj), vValues );        
    }
    return Cec_ManCheckSat2_rec( p, pFan0, Gia_ObjFaninC0(pObj), vValues );
}
word Cec_ManCheckSat2( Gia_Man_t * p, Gia_Obj_t * pObj, int Value, Vec_Wrd_t * vValues )
{
    return 0;
}

/**Function*************************************************************

  Synopsis    [Return 1 if pObj can have Value.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Cec_ManCheckSat_rec( Gia_Man_t * p, Gia_Obj_t * pObj, int Value )
{
    Gia_Obj_t * pFan0, * pFan1;
    if ( Gia_ObjIsTravIdCurrent(p, pObj) )
        return Value == (int)pObj->fMark0;
    Gia_ObjSetTravIdCurrent(p, pObj);
    pObj->fMark0 = Value;
    if ( !Gia_ObjIsAnd(pObj) )
        return 1;
    pFan0 = Gia_ObjFanin0(pObj);
    pFan1 = Gia_ObjFanin1(pObj);
    if ( Value )
    {
        return Cec_ManCheckSat_rec( p, pFan0, !Gia_ObjFaninC0(pObj) ) &&
               Cec_ManCheckSat_rec( p, pFan1, !Gia_ObjFaninC1(pObj) );
    }
    if ( Gia_ObjIsTravIdCurrent(p, pFan0) ) // already assigned
    {
        if ( Gia_ObjFaninC0(pObj) == (int)pFan0->fMark0 ) // justified
            return 1;
        return Cec_ManCheckSat_rec( p, pFan1, Gia_ObjFaninC1(pObj) );        
    }
    if ( Gia_ObjIsTravIdCurrent(p, pFan1) ) // already assigned
    {
        if ( Gia_ObjFaninC1(pObj) == (int)pFan1->fMark0 ) // justified
            return 1;
        return Cec_ManCheckSat_rec( p, pFan0, Gia_ObjFaninC0(pObj) );        
    }
    return Cec_ManCheckSat_rec( p, pFan0, Gia_ObjFaninC0(pObj) );
}
void Cec_ManSimBack( Gia_Man_t * p )
{
    abctime clk = Abc_Clock();
    Vec_Wrd_t * vValues = Vec_WrdStart( Gia_ManObjNum(p) );
    Gia_Obj_t * pObj; 
    int i, Count = 0;
    word Res;
    Gia_ManSetPhase( p );
    //Gia_ManForEachAnd( p, pObj, i )
    //    printf( "%d", pObj->fPhase );
    //printf( "\n" );
    //return;

    Gia_ManForEachAnd( p, pObj, i )
    {
        Gia_ManIncrementTravId(p);
        //Gia_ManCleanMark0( p );
        Res = Cec_ManCheckSat_rec( p, pObj, !pObj->fPhase );
        //if ( Res )
        //    Cec_ObjSatVerify( p, pObj, !pObj->fPhase );

        //Res = Cec_ManCheckSat2_rec( p, pObj, !pObj->fPhase ? ~(word)0 : 0, vValues );
        //if ( Res )
        //    Cec_ObjSatVerify2( p, pObj, !pObj->fPhase, Res );

        Count += (int)(Res > 0);
    }
    Vec_WrdFree( vValues );
    printf( "Obj = %6d.  SAT = %6d.  ", Gia_ManAndNum(p), Count );
    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

