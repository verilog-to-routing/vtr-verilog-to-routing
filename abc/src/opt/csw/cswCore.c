/**CFile****************************************************************

  FileName    [cswCore.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Cut sweeping.]

  Synopsis    []

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - July 11, 2007.]

  Revision    [$Id: cswCore.c,v 1.00 2007/07/11 00:00:00 alanmi Exp $]

***********************************************************************/

#include "cswInt.h"

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
Aig_Man_t * Csw_Sweep( Aig_Man_t * pAig, int nCutsMax, int nLeafMax, int fVerbose )
{
    Csw_Man_t * p;
    Aig_Man_t * pRes;
    Aig_Obj_t * pObj, * pObjNew, * pObjRes;
    int i;
    abctime clk;
clk = Abc_Clock();
    // start the manager
    p = Csw_ManStart( pAig, nCutsMax, nLeafMax, fVerbose );
    // set elementary cuts at the PIs
    Aig_ManForEachCi( p->pManRes, pObj, i )
    {
        Csw_ObjPrepareCuts( p, pObj, 1 );
        Csw_ObjAddRefs( p, pObj, Aig_ManCi(p->pManAig,i)->nRefs );
    }
    // process the nodes
    Aig_ManForEachNode( pAig, pObj, i )
    {
        // create the new node
        pObjNew = Aig_And( p->pManRes, Csw_ObjChild0Equiv(p, pObj), Csw_ObjChild1Equiv(p, pObj) );
        // check if this node can be represented using another node
//        pObjRes = Csw_ObjSweep( p, Aig_Regular(pObjNew), pObj->nRefs > 1 );
//        pObjRes = Aig_NotCond( pObjRes, Aig_IsComplement(pObjNew) );
        // try recursively if resubsitution is used
        do {
            pObjRes = Csw_ObjSweep( p, Aig_Regular(pObjNew), pObj->nRefs > 1 );
            pObjRes = Aig_NotCond( pObjRes, Aig_IsComplement(pObjNew) );        
            pObjNew = pObjRes;
        } while ( Csw_ObjCuts(p, Aig_Regular(pObjNew)) == NULL && !Aig_ObjIsConst1(Aig_Regular(pObjNew)) );
        // save the resulting node
        Csw_ObjSetEquiv( p, pObj, pObjRes );
        // add to the reference counter
        Csw_ObjAddRefs( p, Aig_Regular(pObjRes), pObj->nRefs );
    }
    // add the POs
    Aig_ManForEachCo( pAig, pObj, i )
        Aig_ObjCreateCo( p->pManRes, Csw_ObjChild0Equiv(p, pObj) );
    // remove dangling nodes 
    Aig_ManCleanup( p->pManRes );
    // return the resulting manager
p->timeTotal = Abc_Clock() - clk;
p->timeOther = p->timeTotal - p->timeCuts - p->timeHash;
    pRes = p->pManRes;
    Csw_ManStop( p );
    return pRes;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

