/**CFile****************************************************************

  FileName    [amapMan.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Technology mapper for standard cells.]

  Synopsis    [Mapping manager.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: amapMan.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "amapInt.h"

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
Amap_Man_t * Amap_ManStart( int nNodes )
{
    Amap_Man_t * p;
    // start the manager
    p = ABC_ALLOC( Amap_Man_t, 1 );
    memset( p, 0, sizeof(Amap_Man_t) );
    p->fEpsilonInternal = (float)0.01;
    // allocate arrays for nodes
    p->vPis    = Vec_PtrAlloc( 100 );
    p->vPos    = Vec_PtrAlloc( 100 );
    p->vObjs   = Vec_PtrAlloc( 100 );
    p->vTemp   = Vec_IntAlloc( 100 );
    p->vCuts0  = Vec_PtrAlloc( 100 );
    p->vCuts1  = Vec_PtrAlloc( 100 );
    p->vCuts2  = Vec_PtrAlloc( 100 );
    p->vTempP  = Vec_PtrAlloc( 100 );
    // prepare the memory manager
    p->pMemObj = Aig_MmFixedStart( sizeof(Amap_Obj_t), nNodes );
    p->pMemCuts = Aig_MmFlexStart();
    p->pMemCutBest = Aig_MmFlexStart();
    p->pMemTemp = Aig_MmFlexStart();
    return p;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Amap_ManStop( Amap_Man_t * p )
{
    Vec_PtrFree( p->vPis );
    Vec_PtrFree( p->vPos );
    Vec_PtrFree( p->vObjs );
    Vec_PtrFree( p->vCuts0 );
    Vec_PtrFree( p->vCuts1 );
    Vec_PtrFree( p->vCuts2 );
    Vec_PtrFree( p->vTempP );
    Vec_IntFree( p->vTemp );
    Aig_MmFixedStop( p->pMemObj, 0 );
    Aig_MmFlexStop( p->pMemCuts, 0 );
    Aig_MmFlexStop( p->pMemCutBest, 0 );
    Aig_MmFlexStop( p->pMemTemp, 0 );
    ABC_FREE( p->pMatsTemp );
    ABC_FREE( p->ppCutsTemp );
    ABC_FREE( p->pCutsPi );
    ABC_FREE( p );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

