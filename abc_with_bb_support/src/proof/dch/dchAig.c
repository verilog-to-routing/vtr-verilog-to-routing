/**CFile****************************************************************

  FileName    [dchAig.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Choice computation for tech-mapping.]

  Synopsis    [AIG manipulation.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 29, 2008.]

  Revision    [$Id: dchAig.c,v 1.00 2008/07/29 00:00:00 alanmi Exp $]

***********************************************************************/

#include "dchInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Derives the cumulative AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Dch_DeriveTotalAig_rec( Aig_Man_t * p, Aig_Obj_t * pObj )
{
    if ( pObj->pData )
        return;
    Dch_DeriveTotalAig_rec( p, Aig_ObjFanin0(pObj) );
    Dch_DeriveTotalAig_rec( p, Aig_ObjFanin1(pObj) );
    pObj->pData = Aig_And( p, Aig_ObjChild0Copy(pObj), Aig_ObjChild1Copy(pObj) );
}

/**Function*************************************************************

  Synopsis    [Derives the cumulative AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Dch_DeriveTotalAig( Vec_Ptr_t * vAigs )
{
    Aig_Man_t * pAig, * pAig2, * pAigTotal;
    Aig_Obj_t * pObj, * pObjPi, * pObjPo;
    int i, k, nNodes;
    assert( Vec_PtrSize(vAigs) > 0 );
    // make sure they have the same number of PIs/POs
    nNodes = 0;
    pAig = (Aig_Man_t *)Vec_PtrEntry( vAigs, 0 );
    Vec_PtrForEachEntry( Aig_Man_t *, vAigs, pAig2, i )
    {
        assert( Aig_ManCiNum(pAig) == Aig_ManCiNum(pAig2) );
        assert( Aig_ManCoNum(pAig) == Aig_ManCoNum(pAig2) );
        nNodes += Aig_ManNodeNum(pAig2);
        Aig_ManCleanData( pAig2 );
    }
    // map constant nodes
    pAigTotal = Aig_ManStart( nNodes );
    Vec_PtrForEachEntry( Aig_Man_t *, vAigs, pAig2, k )
        Aig_ManConst1(pAig2)->pData = Aig_ManConst1(pAigTotal);
    // map primary inputs
    Aig_ManForEachCi( pAig, pObj, i )
    {
        pObjPi = Aig_ObjCreateCi( pAigTotal );
        Vec_PtrForEachEntry( Aig_Man_t *, vAigs, pAig2, k )
            Aig_ManCi( pAig2, i )->pData = pObjPi;
    }
    // construct the AIG in the order of POs
    Aig_ManForEachCo( pAig, pObj, i )
    {
        Vec_PtrForEachEntry( Aig_Man_t *, vAigs, pAig2, k )
        {
            pObjPo = Aig_ManCo( pAig2, i );
            Dch_DeriveTotalAig_rec( pAigTotal, Aig_ObjFanin0(pObjPo) );
        }
        Aig_ObjCreateCo( pAigTotal, Aig_ObjChild0Copy(pObj) );
    }
/*
    // mark the cone of the first AIG
    Aig_ManIncrementTravId( pAigTotal );
    Aig_ManForEachObj( pAig, pObj, i )
        if ( pObj->pData ) 
            Aig_ObjSetTravIdCurrent( pAigTotal, pObj->pData );
*/
    // cleanup should not be done
    return pAigTotal;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

