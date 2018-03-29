/**CFile****************************************************************

  FileName    [giaRetime.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Scalable AIG package.]

  Synopsis    [Performs most-forward retiming for AIG with flop classes.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: giaRetime.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "gia.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Marks objects reachables from Const0 and PIs/

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManMarkAutonomous_rec( Gia_Man_t * p, Gia_Obj_t * pObj )
{
    if ( Gia_ObjIsTravIdCurrent(p, pObj) )
        return pObj->fMark0;
    Gia_ObjSetTravIdCurrent(p, pObj);
    assert( pObj->fMark0 == 0 );
    if ( Gia_ObjIsPi(p, pObj) || Gia_ObjIsConst0(pObj) )
        return pObj->fMark0 = 1;
    if ( Gia_ObjIsCo(pObj) )
        return pObj->fMark0 = Gia_ManMarkAutonomous_rec( p, Gia_ObjFanin0(pObj) );
    if ( Gia_ObjIsCi(pObj) )
        return pObj->fMark0 = Gia_ManMarkAutonomous_rec( p, Gia_ObjRoToRi(p, pObj) );
    assert( Gia_ObjIsAnd(pObj) );
    if ( Gia_ManMarkAutonomous_rec( p, Gia_ObjFanin0(pObj) ) )
        return pObj->fMark0 = 1;
    return pObj->fMark0 = Gia_ManMarkAutonomous_rec( p, Gia_ObjFanin1(pObj) );
}

/**Function*************************************************************

  Synopsis    [Marks with current trav ROs reachable from Const0 and PIs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManMarkAutonomous( Gia_Man_t * p )
{
    Gia_Obj_t * pObj;
    int i;
    Gia_ManCleanMark0( p );
    Gia_ManIncrementTravId( p );
    Gia_ManForEachRo( p, pObj, i )
        Gia_ManMarkAutonomous_rec( p, pObj );
    Gia_ManIncrementTravId( p );
    Gia_ManForEachRo( p, pObj, i )
        if ( pObj->fMark0 )
            Gia_ObjSetTravIdCurrent( p, pObj );
    Gia_ManCleanMark0( p );
}

/**Function*************************************************************

  Synopsis    [Duplicates the AIG recursively.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManRetimeDup_rec( Gia_Man_t * pNew, Gia_Obj_t * pObj )
{
    if ( ~pObj->Value )
        return;
    assert( Gia_ObjIsAnd(pObj) );
    Gia_ManRetimeDup_rec( pNew, Gia_ObjFanin0(pObj) );
    Gia_ManRetimeDup_rec( pNew, Gia_ObjFanin1(pObj) );
    pObj->Value = Gia_ManHashAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
}

/**Function*************************************************************

  Synopsis    [Duplicates the AIG while retiming the registers to the cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManRetimeDupForward( Gia_Man_t * p, Vec_Ptr_t * vCut )
{
    Gia_Man_t * pNew, * pTemp;
    Gia_Obj_t * pObj, * pObjRi, * pObjRo;
    int i;
    // create the new manager
    pNew = Gia_ManStart( Gia_ManObjNum(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    Gia_ManHashAlloc( pNew );
    // create the true PIs
    Gia_ManFillValue( p );
    Gia_ManSetPhase( p );
    Gia_ManConst0(p)->Value = 0;
    Gia_ManForEachPi( p, pObj, i )
        pObj->Value = Gia_ManAppendCi( pNew );
    // create the registers
    Vec_PtrForEachEntry( Gia_Obj_t *, vCut, pObj, i )
        pObj->Value = Abc_LitNotCond( Gia_ManAppendCi(pNew), pObj->fPhase );
    // duplicate logic above the cut
    Gia_ManForEachCo( p, pObj, i )
        Gia_ManRetimeDup_rec( pNew, Gia_ObjFanin0(pObj) );
    // create the true POs
    Gia_ManForEachPo( p, pObj, i )
        Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
    // remember value in LI
    Gia_ManForEachRi( p, pObj, i )
        pObj->Value = Gia_ObjFanin0Copy(pObj);
    // transfer values from the LIs to the LOs
    Gia_ManForEachRiRo( p, pObjRi, pObjRo, i )
        pObjRo->Value = pObjRi->Value;
    // erase the data values on the internal nodes of the cut
    Vec_PtrForEachEntry( Gia_Obj_t *, vCut, pObj, i )
        if ( Gia_ObjIsAnd(pObj) )
            pObj->Value = ~0;
    // duplicate logic below the cut
    Vec_PtrForEachEntry( Gia_Obj_t *, vCut, pObj, i )
    {
        Gia_ManRetimeDup_rec( pNew, pObj );
        Gia_ManAppendCo( pNew, Abc_LitNotCond( pObj->Value, pObj->fPhase ) );
    }
    Gia_ManHashStop( pNew );
    Gia_ManSetRegNum( pNew, Vec_PtrSize(vCut) );
    pNew = Gia_ManCleanup( pTemp = pNew );
    Gia_ManStop( pTemp );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Derives the cut for forward retiming.]

  Description [Assumes topological ordering of the nodes.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManRetimeForwardOne( Gia_Man_t * p, int * pnRegFixed, int * pnRegMoves )
{
    Vec_Int_t * vFlopClasses = NULL;
    Vec_Int_t * vObjClasses = NULL;
    Gia_Man_t * pNew;
    Vec_Ptr_t * vCut;
    Gia_Obj_t * pObj;
    int i;
    if ( p->vFlopClasses )
    {
//        printf( "Performing retiming with register classes.\n" );
        vObjClasses = Vec_IntAlloc( Gia_ManObjNum(p) );
        for ( i = 0; i < Gia_ManObjNum(p); i++ )
            Vec_IntPush( vObjClasses, -1 );
        Gia_ManForEachRo( p, pObj, i )
            Vec_IntWriteEntry( vObjClasses, Gia_ObjId(p, pObj), Vec_IntEntry(p->vFlopClasses, i) );
        vFlopClasses = Vec_IntAlloc( Gia_ManRegNum(p) );
    }
    // mark the retimable nodes
    Gia_ManIncrementTravId( p );
    Gia_ManMarkAutonomous( p );
    // mark the retimable registers with the fresh trav ID
    Gia_ManIncrementTravId( p );
    *pnRegFixed = 0;
    Gia_ManForEachRo( p, pObj, i )
        if ( Gia_ObjIsTravIdPrevious(p, pObj) )
            Gia_ObjSetTravIdCurrent(p, pObj);
        else
            (*pnRegFixed)++;
    // mark all the nodes that can be retimed forward
    *pnRegMoves = 0;
    Gia_ManForEachAnd( p, pObj, i )
        if ( Gia_ObjIsTravIdCurrent(p, Gia_ObjFanin0(pObj)) && Gia_ObjIsTravIdCurrent(p, Gia_ObjFanin1(pObj)) )
        {
            if ( vObjClasses && Vec_IntEntry(vObjClasses, Gia_ObjFaninId0(pObj, i)) != Vec_IntEntry(vObjClasses, Gia_ObjFaninId1(pObj, i)) )
                continue;
            if ( vObjClasses )
                Vec_IntWriteEntry( vObjClasses, Gia_ObjId(p, pObj), Vec_IntEntry(vObjClasses, Gia_ObjFaninId0(pObj, i)) );
            Gia_ObjSetTravIdCurrent(p, pObj);
            (*pnRegMoves)++;
        }
    // mark the remaining registers
    Gia_ManForEachRo( p, pObj, i )
        Gia_ObjSetTravIdCurrent(p, pObj);
    // find the cut (all such marked objects that fanout into unmarked nodes)
    vCut = Vec_PtrAlloc( 1000 );
    Gia_ManIncrementTravId( p );
    Gia_ManForEachObj( p, pObj, i )
    {
        if ( Gia_ObjIsTravIdPrevious(p, pObj) )
            continue;
        if ( (Gia_ObjIsCo(pObj) || Gia_ObjIsAnd(pObj)) && Gia_ObjIsTravIdPrevious(p, Gia_ObjFanin0(pObj)) )
        {
            if ( vFlopClasses )
                Vec_IntPush( vFlopClasses, Vec_IntEntry(vObjClasses, Gia_ObjFaninId0(pObj, i)) );
            Vec_PtrPush( vCut, Gia_ObjFanin0(pObj) );
            Gia_ObjSetTravIdCurrent( p, Gia_ObjFanin0(pObj) );
        }
        if ( Gia_ObjIsAnd(pObj) && Gia_ObjIsTravIdPrevious(p, Gia_ObjFanin1(pObj)) )
        {
            if ( vFlopClasses )
                Vec_IntPush( vFlopClasses, Vec_IntEntry(vObjClasses, Gia_ObjFaninId1(pObj, i)) );
            Vec_PtrPush( vCut, Gia_ObjFanin1(pObj) );
            Gia_ObjSetTravIdCurrent( p, Gia_ObjFanin1(pObj) );
        }
    }
    assert( vFlopClasses == NULL || Vec_IntSize(vFlopClasses) == Vec_PtrSize(vCut) );
    // finally derive the new manager
    pNew = Gia_ManRetimeDupForward( p, vCut );
    Vec_PtrFree( vCut );
    if ( vObjClasses )
    Vec_IntFree( vObjClasses );
    pNew->vFlopClasses = vFlopClasses;
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Derives the cut for forward retiming.]

  Description [Assumes topological ordering of the nodes.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManRetimeForward( Gia_Man_t * p, int nMaxIters, int fVerbose )
{
    Gia_Man_t * pNew, * pTemp;
    int i, nRegFixed, nRegMoves = 1;
    abctime clk;
    pNew = p;
    for ( i = 0; i < nMaxIters && nRegMoves > 0; i++ )
    {
        clk = Abc_Clock();
        pNew = Gia_ManRetimeForwardOne( pTemp = pNew, &nRegFixed, &nRegMoves );
        if ( fVerbose )
        {
            printf( "%2d : And = %6d. Reg = %5d. Unret = %5d. Move = %6d. ", 
                i + 1, Gia_ManAndNum(pTemp), Gia_ManRegNum(pTemp), nRegFixed, nRegMoves );
            ABC_PRT( "Time", Abc_Clock() - clk );
        }
        if ( pTemp != p )
            Gia_ManStop( pTemp );
    }
/*
    clk = Abc_Clock();
    pNew = Gia_ManReduceLaches( pNew, fVerbose );
    if ( fVerbose )
    {
        ABC_PRT( "Register sharing time", Abc_Clock() - clk );
    }
*/
    return pNew;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

