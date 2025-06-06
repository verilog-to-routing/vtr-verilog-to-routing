/**CFile****************************************************************

  FileName    [hopMan.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Minimalistic And-Inverter Graph package.]

  Synopsis    [AIG manager.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - May 11, 2006.]

  Revision    [$Id: hopMan.c,v 1.00 2006/05/11 00:00:00 alanmi Exp $]

***********************************************************************/

#include "hop.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Starts the AIG manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Hop_Man_t * Hop_ManStart()
{
    Hop_Man_t * p;
    // start the manager
    p = ABC_ALLOC( Hop_Man_t, 1 );
    memset( p, 0, sizeof(Hop_Man_t) );
    // perform initializations
    p->nTravIds = 1;
    p->fRefCount = 1;
    p->fCatchExor = 0;
    // allocate arrays for nodes
    p->vPis = Vec_PtrAlloc( 100 );
    p->vPos = Vec_PtrAlloc( 100 );
    // prepare the internal memory manager
    Hop_ManStartMemory( p );
    // create the constant node
    p->pConst1 = Hop_ManFetchMemory( p );
    p->pConst1->Type = AIG_CONST1;
    p->pConst1->fPhase = 1;
    p->nCreated = 1;
    // start the table
//    p->nTableSize = 107;
    p->nTableSize = 10007;
    p->pTable = ABC_ALLOC( Hop_Obj_t *, p->nTableSize );
    memset( p->pTable, 0, sizeof(Hop_Obj_t *) * p->nTableSize );
    return p;
}

/**Function*************************************************************

  Synopsis    [Stops the AIG manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Hop_ManStop( Hop_Man_t * p )
{
    Hop_Obj_t * pObj;
    int i;
    // make sure the nodes have clean marks
    pObj = Hop_ManConst1(p);
    assert( !pObj->fMarkA && !pObj->fMarkB );
    Hop_ManForEachPi( p, pObj, i )
        assert( !pObj->fMarkA && !pObj->fMarkB );
    Hop_ManForEachPo( p, pObj, i )
        assert( !pObj->fMarkA && !pObj->fMarkB );
    Hop_ManForEachNode( p, pObj, i )
        assert( !pObj->fMarkA && !pObj->fMarkB );
    // print time
    if ( p->time1 ) { ABC_PRT( "time1", p->time1 ); }
    if ( p->time2 ) { ABC_PRT( "time2", p->time2 ); }
//    Hop_TableProfile( p );
    if ( p->vChunks )  Hop_ManStopMemory( p );
    if ( p->vPis )     Vec_PtrFree( p->vPis );
    if ( p->vPos )     Vec_PtrFree( p->vPos );
    if ( p->vObjs )    Vec_PtrFree( p->vObjs );
    ABC_FREE( p->pTable );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    [Returns the number of dangling nodes removed.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Hop_ManCleanup( Hop_Man_t * p )
{
    Vec_Ptr_t * vObjs;
    Hop_Obj_t * pNode;
    int i, nNodesOld;
    assert( p->fRefCount );
    nNodesOld = Hop_ManNodeNum(p);
    // collect roots of dangling nodes
    vObjs = Vec_PtrAlloc( 100 );
    Hop_ManForEachNode( p, pNode, i )
        if ( Hop_ObjRefs(pNode) == 0 )
            Vec_PtrPush( vObjs, pNode );
    // recursively remove dangling nodes
    Vec_PtrForEachEntry( Hop_Obj_t *, vObjs, pNode, i )
        Hop_ObjDelete_rec( p, pNode );
    Vec_PtrFree( vObjs );
    return nNodesOld - Hop_ManNodeNum(p);
}

/**Function*************************************************************

  Synopsis    [Stops the AIG manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Hop_ManPrintStats( Hop_Man_t * p )
{
    printf( "PI/PO = %d/%d. ", Hop_ManPiNum(p), Hop_ManPoNum(p) );
    printf( "A = %7d. ",       Hop_ManAndNum(p) );
    printf( "X = %5d. ",       Hop_ManExorNum(p) );
    printf( "Cre = %7d. ",     p->nCreated );
    printf( "Del = %7d. ",     p->nDeleted );
    printf( "Lev = %3d. ",     Hop_ManCountLevels(p) );
    printf( "\n" );
}



////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

