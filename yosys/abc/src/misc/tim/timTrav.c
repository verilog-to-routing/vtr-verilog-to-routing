/**CFile****************************************************************

  FileName    [timTrav.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Hierarchy/timing manager.]

  Synopsis    [Manipulation of traversal IDs.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - April 28, 2007.]

  Revision    [$Id: timTrav.c,v 1.00 2007/04/28 00:00:00 alanmi Exp $]

***********************************************************************/

#include "timInt.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Increments the trav ID of the manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Tim_ManIncrementTravId( Tim_Man_t * p )
{
    int i;
    if ( p->nTravIds >= (1<<30)-1 )
    {
        p->nTravIds = 0;
        for ( i = 0; i < p->nCis; i++ )
            p->pCis[i].TravId = 0;
        for ( i = 0; i < p->nCos; i++ )
            p->pCos[i].TravId = 0;
    }
    assert( p->nTravIds < (1<<30)-1 );
    p->nTravIds++;
}

/**Function*************************************************************

  Synopsis    [Label box inputs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Tim_ManSetCurrentTravIdBoxInputs( Tim_Man_t * p, int iBox )
{
    Tim_Box_t * pBox;
    Tim_Obj_t * pObj;
    int i;
    pBox = Tim_ManBox( p, iBox );
    Tim_ManBoxForEachInput( p, pBox, pObj, i )
        pObj->TravId = p->nTravIds;
}

/**Function*************************************************************

  Synopsis    [Label box outputs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Tim_ManSetCurrentTravIdBoxOutputs( Tim_Man_t * p, int iBox )
{
    Tim_Box_t * pBox;
    Tim_Obj_t * pObj;
    int i;
    pBox = Tim_ManBox( p, iBox );
    Tim_ManBoxForEachOutput( p, pBox, pObj, i )
        pObj->TravId = p->nTravIds;
}

/**Function*************************************************************

  Synopsis    [Label box inputs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Tim_ManSetPreviousTravIdBoxInputs( Tim_Man_t * p, int iBox )
{
    Tim_Box_t * pBox;
    Tim_Obj_t * pObj;
    int i;
    pBox = Tim_ManBox( p, iBox );
    Tim_ManBoxForEachInput( p, pBox, pObj, i )
        pObj->TravId = p->nTravIds - 1;
}

/**Function*************************************************************

  Synopsis    [Label box outputs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Tim_ManSetPreviousTravIdBoxOutputs( Tim_Man_t * p, int iBox )
{
    Tim_Box_t * pBox;
    Tim_Obj_t * pObj;
    int i;
    pBox = Tim_ManBox( p, iBox );
    Tim_ManBoxForEachOutput( p, pBox, pObj, i )
        pObj->TravId = p->nTravIds - 1;
}

/**Function*************************************************************

  Synopsis    [Updates required time of the CI.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Tim_ManIsCiTravIdCurrent( Tim_Man_t * p, int iCi )
{
    assert( iCi < p->nCis );
    assert( p->fUseTravId );
    return p->pCis[iCi].TravId == p->nTravIds;
}

/**Function*************************************************************

  Synopsis    [Updates required time of the CO.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Tim_ManIsCoTravIdCurrent( Tim_Man_t * p, int iCo )
{
    assert( iCo < p->nCos );
    assert( p->fUseTravId );
    return p->pCos[iCo].TravId == p->nTravIds;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

