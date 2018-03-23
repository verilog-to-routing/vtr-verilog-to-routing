/**CFile****************************************************************

  FileName    [mpmMig.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Configurable technology mapper.]

  Synopsis    [Subject graph data structure.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 1, 2013.]

  Revision    [$Id: mpmMig.c,v 1.00 2013/06/01 00:00:00 alanmi Exp $]

***********************************************************************/

#include "mpmInt.h"

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
Mig_Man_t * Mig_ManStart()
{
    Mig_Man_t * p;
    assert( sizeof(Mig_Obj_t) >= 16 );
    assert( (1 << MIG_BASE) == MIG_MASK + 1 );
    p = ABC_CALLOC( Mig_Man_t, 1 );
    Vec_IntGrow( &p->vCis, 1024 );
    Vec_IntGrow( &p->vCos, 1024 );
    Mig_ManAppendObj( p ); // const0
    return p;
}
void Mig_ManStop( Mig_Man_t * p )
{
    if ( 0 )
    printf( "Subject graph uses %d pages of %d objects with %d entries. Total memory = %.2f MB.\n", 
        Vec_PtrSize(&p->vPages), MIG_MASK + 1, p->nObjs,
        1.0 * Vec_PtrSize(&p->vPages) * (MIG_MASK + 1) * 16 / (1 << 20) );
    // attributes
    ABC_FREE( p->vTravIds.pArray );
    ABC_FREE( p->vCopies.pArray );
    ABC_FREE( p->vLevels.pArray );
    ABC_FREE( p->vRefs.pArray );
    ABC_FREE( p->vSibls.pArray );
    // pages
    Vec_PtrForEachEntry( Mig_Obj_t *, &p->vPages, p->pPage, p->iPage )
        --p->pPage, ABC_FREE( p->pPage );
    // objects
    ABC_FREE( p->vPages.pArray );
    ABC_FREE( p->vCis.pArray );
    ABC_FREE( p->vCos.pArray );
    ABC_FREE( p->pName );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Mig_ManTypeNum( Mig_Man_t * p, int Type )
{
    Mig_Obj_t * pObj;
    int Counter = 0;
    Mig_ManForEachNode( p, pObj )
        Counter += (Mig_ObjNodeType(pObj) == Type);
    return Counter;
}
int Mig_ManAndNum( Mig_Man_t * p )
{
    return Mig_ManTypeNum(p, 1);
}
int Mig_ManXorNum( Mig_Man_t * p )
{
    return Mig_ManTypeNum(p, 2);
}
int Mig_ManMuxNum( Mig_Man_t * p )
{
    return Mig_ManTypeNum(p, 3);
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mig_ManSetRefs( Mig_Man_t * p )
{
    Mig_Obj_t * pObj;
    int i, iFanin;
    // increment references
    Vec_IntFill( &p->vRefs, Mig_ManObjNum(p), 0 );
    Mig_ManForEachObj( p, pObj )
    {
        Mig_ObjForEachFaninId( pObj, iFanin, i )
            Vec_IntAddToEntry( &p->vRefs, iFanin, 1 );
        if ( Mig_ObjSiblId(pObj) )
            Vec_IntAddToEntry( &p->vRefs, Mig_ObjSiblId(pObj), 1 );
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Mig_ManSuppSize_rec( Mig_Obj_t * pObj )
{
    if ( pObj == NULL )
        return 0;
    if ( Mig_ObjIsTravIdCurrent(pObj) )
        return 0;
    Mig_ObjSetTravIdCurrent(pObj);
    if ( Mig_ObjIsCi(pObj) )
        return 1;
    assert( Mig_ObjIsNode(pObj) );
    return Mig_ManSuppSize_rec( Mig_ObjFanin0(pObj) ) +
           Mig_ManSuppSize_rec( Mig_ObjFanin1(pObj) ) +
           Mig_ManSuppSize_rec( Mig_ObjFanin2(pObj) );
}
int Mig_ManSuppSize2_rec( Mig_Man_t * p, int iObj )
{
    Mig_Obj_t * pObj;
    if ( iObj == MIG_NONE )
        return 0;
    if ( Mig_ObjIsTravIdCurrentId(p, iObj) )
        return 0;
    Mig_ObjSetTravIdCurrentId(p, iObj);
    pObj = Mig_ManObj( p, iObj );
    if ( Mig_ObjIsCi(pObj) )
        return 1;
    assert( Mig_ObjIsNode(pObj) );
    return Mig_ManSuppSize2_rec( p, Mig_ObjFaninId0(pObj) ) +
           Mig_ManSuppSize2_rec( p, Mig_ObjFaninId1(pObj) ) +
           Mig_ManSuppSize2_rec( p, Mig_ObjFaninId2(pObj) );
}
int Mig_ManSuppSizeOne( Mig_Obj_t * pObj )
{
    Mig_ObjIncrementTravId( pObj );
//    return Mig_ManSuppSize_rec( pObj );
    return Mig_ManSuppSize2_rec( Mig_ObjMan(pObj), Mig_ObjId(pObj) );
}
int Mig_ManSuppSizeTest( Mig_Man_t * p )
{
    Mig_Obj_t * pObj;
    int Counter = 0;
    abctime clk = Abc_Clock();
    Mig_ManForEachObj( p, pObj )
        if ( Mig_ObjIsNode(pObj) )
            Counter += (Mig_ManSuppSizeOne(pObj) <= 16);
    printf( "Nodes with small support %d (out of %d)\n", Counter, Mig_ManNodeNum(p) );
    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
    return Counter;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

