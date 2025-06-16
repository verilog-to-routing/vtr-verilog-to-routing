/**CFile****************************************************************

  FileName    [saigScl.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Sequential AIG package.]

  Synopsis    [Sequential cleanup.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: saigScl.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

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

  Synopsis    [Report registers useless for SEC.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Saig_ManReportUselessRegisters( Aig_Man_t * pAig )
{
    Aig_Obj_t * pObj, * pDriver;
    int i, Counter1, Counter2;
    // set PIO numbers
    Aig_ManSetCioIds( pAig );
    // check how many POs are driven by a register whose ref count is 1
    Counter1 = 0;
    Saig_ManForEachPo( pAig, pObj, i )
    {
        pDriver = Aig_ObjFanin0(pObj);
        if ( Saig_ObjIsLo(pAig, pDriver) && Aig_ObjRefs(pDriver) == 1 )
            Counter1++;
    }
    // check how many PIs have ref count 1 and drive a register
    Counter2 = 0;
    Saig_ManForEachLi( pAig, pObj, i )
    {
        pDriver = Aig_ObjFanin0(pObj);
        if ( Saig_ObjIsPi(pAig, pDriver) && Aig_ObjRefs(pDriver) == 1 )
            Counter2++;
    }
    if ( Counter1 )
        printf( "Network has %d (out of %d) registers driving POs.\n", Counter1, Saig_ManRegNum(pAig) );
    if ( Counter2 )
        printf( "Network has %d (out of %d) registers driven by PIs.\n", Counter2, Saig_ManRegNum(pAig) );
}


/**Function*************************************************************

  Synopsis    [Report the number of pairs of complemented registers.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Saig_ManReportComplements( Aig_Man_t * p )
{
    Aig_Obj_t * pObj, * pFanin;
    int i, Counter = 0;
    assert( Aig_ManRegNum(p) > 0 );
    Aig_ManForEachObj( p, pObj, i )
        assert( !pObj->fMarkA );
    Aig_ManForEachLiSeq( p, pObj, i )
    {
        pFanin = Aig_ObjFanin0(pObj);
        if ( pFanin->fMarkA )
            Counter++;
        else
            pFanin->fMarkA = 1;
    }
    // count fanins that have both attributes
    Aig_ManForEachLiSeq( p, pObj, i )
    {
        pFanin = Aig_ObjFanin0(pObj);
        pFanin->fMarkA = 0;
    }
    return Counter;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

