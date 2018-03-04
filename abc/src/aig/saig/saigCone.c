/**CFile****************************************************************

  FileName    [saigCone.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Sequential AIG package.]

  Synopsis    [Cone of influence computation.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: saigCone.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "saig.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Counts the support size of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Saig_ManSupport_rec( Aig_Man_t * p, Aig_Obj_t * pObj, Vec_Ptr_t * vSupp )
{
    if ( Aig_ObjIsTravIdCurrent(p, pObj) )
        return;
    Aig_ObjSetTravIdCurrent(p, pObj);
    if ( Aig_ObjIsConst1(pObj) )
        return;
    if ( Aig_ObjIsCi(pObj) )
    {
        if ( Saig_ObjIsLo(p,pObj) )
        {
            pObj = Saig_ManLi( p, Aig_ObjCioId(pObj)-Saig_ManPiNum(p) );
            Vec_PtrPush( vSupp, pObj );
        }
        return;
    }
    assert( Aig_ObjIsNode(pObj) );
    Saig_ManSupport_rec( p, Aig_ObjFanin0(pObj), vSupp );
    Saig_ManSupport_rec( p, Aig_ObjFanin1(pObj), vSupp );
}

/**Function*************************************************************

  Synopsis    [Counts the support size of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Saig_ManSupport( Aig_Man_t * p, Vec_Ptr_t * vNodes )
{
    Vec_Ptr_t * vSupp;
    Aig_Obj_t * pObj;
    int i;
    vSupp = Vec_PtrAlloc( 100 );
    Aig_ManIncrementTravId( p );
    Vec_PtrForEachEntry( Aig_Obj_t *, vNodes, pObj, i )
    {
        assert( Aig_ObjIsCo(pObj) );
        Saig_ManSupport_rec( p, Aig_ObjFanin0(pObj), vSupp );
    }
    return vSupp;
}

/**Function*************************************************************

  Synopsis    [Prints information about cones of influence of the POs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Saig_ManPrintConeOne( Aig_Man_t * p, Aig_Obj_t * pObj )
{
    Vec_Ptr_t * vPrev, * vCur, * vTotal;
    int s, i, nCurNew, nCurPrev, nCurOld;
    assert( Saig_ObjIsPo(p, pObj) );
    // start the array
    vPrev = Vec_PtrAlloc( 100 );
    Vec_PtrPush( vPrev, pObj );
    // get the current support
    vCur = Saig_ManSupport( p, vPrev );  
    Vec_PtrClear( vPrev );
    printf( "    PO %3d  ", Aig_ObjCioId(pObj) );
    // continue computing supports as long as there are now nodes
    vTotal = Vec_PtrAlloc( 100 );
    for ( s = 0; ; s++ )
    {
        // classify current into those new, prev, and older 
        nCurNew = nCurPrev = nCurOld = 0;
        Vec_PtrForEachEntry( Aig_Obj_t *, vCur, pObj, i )
        {
            if ( Vec_PtrFind(vTotal, pObj) == -1 )
            {
                Vec_PtrPush( vTotal, pObj );
                nCurNew++;
            }
            else if ( Vec_PtrFind(vPrev, pObj) >= 0 )
                nCurPrev++;
            else
                nCurOld++;
        }
        assert( nCurNew + nCurPrev + nCurOld == Vec_PtrSize(vCur) );
        // print the result
        printf( "%d:%d %d=%d+%d+%d  ", s, Vec_PtrSize(vTotal), Vec_PtrSize(vCur), nCurNew, nCurPrev, nCurOld ); 
        if ( nCurNew == 0 )
            break;
        // compute one more step
        Vec_PtrFree( vPrev );
        vCur = Saig_ManSupport( p, vPrev = vCur );
    }
    printf( "\n" );
    Vec_PtrFree( vPrev );
    Vec_PtrFree( vCur );
    Vec_PtrFree( vTotal );
}

/**Function*************************************************************

  Synopsis    [Prints information about cones of influence of the POs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Saig_ManPrintCones( Aig_Man_t * p )
{
    Aig_Obj_t * pObj;
    int i;
    printf( "The format of this print-out: For each PO, x:a b=c+d+e, where \n" );
    printf( "- x is the time-frame counting back from the PO\n" );
    printf( "- a is the total number of registers in the COI of the PO so far\n" );
    printf( "- b is the number of registers in the COI of the PO in this time-frame\n" );
    printf( "- c is the number of registers in b that are new (appear for the first time)\n" );
    printf( "- d is the number of registers in b in common with the previous time-frame\n" );
    printf( "- e is the number of registers in b in common with other time-frames\n" );
    Aig_ManSetCioIds( p );
    Saig_ManForEachPo( p, pObj, i )
        Saig_ManPrintConeOne( p, pObj );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

