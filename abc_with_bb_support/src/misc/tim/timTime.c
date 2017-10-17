/**CFile****************************************************************

  FileName    [timTime.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Hierarchy/timing manager.]

  Synopsis    [Setting and resetting timing information of the boxes.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - April 28, 2007.]

  Revision    [$Id: timTime.c,v 1.00 2007/04/28 00:00:00 alanmi Exp $]

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

  Synopsis    [Initializes arrival time of the PI.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Tim_ManInitPiArrival( Tim_Man_t * p, int iPi, float Delay )
{
    assert( iPi < p->nCis );
    p->pCis[iPi].timeArr = Delay;
}

/**Function*************************************************************

  Synopsis    [Initializes required time of the PO.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Tim_ManInitPoRequired( Tim_Man_t * p, int iPo, float Delay )
{
    assert( iPo < p->nCos );
    p->pCos[iPo].timeReq = Delay;
}

/**Function*************************************************************

  Synopsis    [Sets arrival times of all PIs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Tim_ManInitPiArrivalAll( Tim_Man_t * p, float Delay )
{
    Tim_Obj_t * pObj;
    int i;
    Tim_ManForEachPi( p, pObj, i )
        Tim_ManInitPiArrival( p, i, Delay );
}

/**Function*************************************************************

  Synopsis    [Sets required times of all POs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Tim_ManInitPoRequiredAll( Tim_Man_t * p, float Delay )
{
    Tim_Obj_t * pObj;
    int i;
    Tim_ManForEachPo( p, pObj, i )
        Tim_ManSetCoRequired( p, i, Delay );
}

/**Function*************************************************************

  Synopsis    [Updates arrival time of the CO.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Tim_ManSetCoArrival( Tim_Man_t * p, int iCo, float Delay )
{
    assert( iCo < p->nCos );
    assert( !p->fUseTravId || p->pCos[iCo].TravId != p->nTravIds );
    p->pCos[iCo].timeArr = Delay;
    p->pCos[iCo].TravId = p->nTravIds;
}

/**Function*************************************************************

  Synopsis    [Updates required time of the CI.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Tim_ManSetCiRequired( Tim_Man_t * p, int iCi, float Delay )
{
    assert( iCi < p->nCis );
    assert( !p->fUseTravId || p->pCis[iCi].TravId != p->nTravIds );
    p->pCis[iCi].timeReq = Delay;
    p->pCis[iCi].TravId = p->nTravIds;
}

/**Function*************************************************************

  Synopsis    [Updates required time of the CO.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Tim_ManSetCoRequired( Tim_Man_t * p, int iCo, float Delay )
{
    assert( iCo < p->nCos );
    assert( !p->fUseTravId || !p->nTravIds || p->pCos[iCo].TravId != p->nTravIds );
    p->pCos[iCo].timeReq = Delay;
    p->pCos[iCo].TravId = p->nTravIds;
}


/**Function*************************************************************

  Synopsis    [Returns CO arrival time.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
float Tim_ManGetCiArrival( Tim_Man_t * p, int iCi )
{
    Tim_Box_t * pBox;
    Tim_Obj_t * pObjThis, * pObj, * pObjRes;
    float * pTable, * pDelays, DelayBest;
    int i, k;
    // consider the already processed PI
    pObjThis = Tim_ManCi( p, iCi );
    if ( p->fUseTravId && pObjThis->TravId == p->nTravIds )
        return pObjThis->timeArr;
    pObjThis->TravId = p->nTravIds;
    // consider the main PI
    pBox = Tim_ManCiBox( p, iCi );
    if ( pBox == NULL )
        return pObjThis->timeArr;
    // update box timing
    pBox->TravId = p->nTravIds;
    // get the arrival times of the inputs of the box (POs)
    if ( p->fUseTravId )
    Tim_ManBoxForEachInput( p, pBox, pObj, i )
        if ( pObj->TravId != p->nTravIds )
            printf( "Tim_ManGetCiArrival(): Input arrival times of the box are not up to date!\n" );
    // compute the arrival times for each output of the box (PIs)
    pTable = Tim_ManBoxDelayTable( p, pBox->iBox );
    Tim_ManBoxForEachOutput( p, pBox, pObjRes, i )
    {
        pDelays = pTable + 3 + i * pBox->nInputs;
        DelayBest = -TIM_ETERNITY;
        Tim_ManBoxForEachInput( p, pBox, pObj, k )
			if ( pDelays[k] != -ABC_INFINITY )
				DelayBest = Abc_MaxInt( DelayBest, pObj->timeArr + pDelays[k] );
        pObjRes->timeArr = DelayBest;
        pObjRes->TravId = p->nTravIds;
    }
    return pObjThis->timeArr;
}

/**Function*************************************************************

  Synopsis    [Returns CO required time.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
float Tim_ManGetCoRequired( Tim_Man_t * p, int iCo )
{
    Tim_Box_t * pBox;
    Tim_Obj_t * pObjThis, * pObj, * pObjRes;
    float * pTable, * pDelays, DelayBest;
    int i, k;
    // consider the already processed PO
    pObjThis = Tim_ManCo( p, iCo );
    if ( p->fUseTravId && pObjThis->TravId == p->nTravIds )
        return pObjThis->timeReq;
    pObjThis->TravId = p->nTravIds;
    // consider the main PO
    pBox = Tim_ManCoBox( p, iCo );
    if ( pBox == NULL )
        return pObjThis->timeReq;
    // update box timing
    pBox->TravId = p->nTravIds;
    // get the required times of the outputs of the box (PIs)
    if ( p->fUseTravId )
    Tim_ManBoxForEachOutput( p, pBox, pObj, i )
        if ( pObj->TravId != p->nTravIds )
            printf( "Tim_ManGetCoRequired(): Output required times of output %d the box %d are not up to date!\n", i, pBox->iBox );
    // compute the required times for each input of the box (POs)
    pTable = Tim_ManBoxDelayTable( p, pBox->iBox );
    Tim_ManBoxForEachInput( p, pBox, pObjRes, i )
    {
        DelayBest = TIM_ETERNITY;
        Tim_ManBoxForEachOutput( p, pBox, pObj, k )
        {
            pDelays = pTable + 3 + k * pBox->nInputs;
			if ( pDelays[k] != -ABC_INFINITY )
				DelayBest = Abc_MinFloat( DelayBest, pObj->timeReq - pDelays[i] );
        }
        pObjRes->timeReq = DelayBest;
        pObjRes->TravId = p->nTravIds;
    }
    return pObjThis->timeReq;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

