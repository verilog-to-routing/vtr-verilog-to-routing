/**CFile****************************************************************

  FileName    [llb2Map.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [BDD based reachability.]

  Synopsis    [Non-linear quantification scheduling.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: llb2Map.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "llbInt.h"
#include "base/abc/abc.h"
#include "map/if/if.h"

ABC_NAMESPACE_IMPL_START
 

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Returns internal nodes used in the mapping.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Llb_AigMap( Aig_Man_t * pAig, int nLutSize, int nLutMin )
{
    extern Abc_Ntk_t * Abc_NtkFromAigPhase( Aig_Man_t * pMan );
    extern If_Man_t *  Abc_NtkToIf( Abc_Ntk_t * pNtk, If_Par_t * pPars );
    extern void        Gia_ManSetIfParsDefault( void * pPars );
    If_Par_t Pars, * pPars = &Pars;
    If_Man_t * pIfMan;
    If_Obj_t * pAnd;
    Abc_Ntk_t * pNtk;
    Abc_Obj_t * pNode;
    Vec_Int_t * vNodes;
    Aig_Obj_t * pObj;
    int i;

    // create ABC network
    pNtk = Abc_NtkFromAigPhase( pAig );
    assert( Abc_NtkIsStrash(pNtk) );

    // derive mapping parameters
    Gia_ManSetIfParsDefault( pPars );
    pPars->nLutSize = nLutSize;

    // get timing information
    pPars->pTimesArr = Abc_NtkGetCiArrivalFloats(pNtk);
    pPars->pTimesReq = Abc_NtkGetCoRequiredFloats(pNtk);

    // perform LUT mapping
    pIfMan = Abc_NtkToIf( pNtk, pPars );    
    if ( pIfMan == NULL )
    {
        Abc_NtkDelete( pNtk );
        return NULL;
    }
    if ( !If_ManPerformMapping( pIfMan ) )
    {
        Abc_NtkDelete( pNtk );
        If_ManStop( pIfMan );
        return NULL;
    }

    // mark nodes in the AIG used in the mapping
    Aig_ManCleanMarkA( pAig );
    Aig_ManForEachNode( pAig, pObj, i )
    {
        pNode = (Abc_Obj_t *)pObj->pData;
        if ( pNode == NULL )
            continue;
        pAnd = (If_Obj_t *)pNode->pCopy;
        if ( pAnd == NULL )
            continue;
        if ( pAnd->nRefs > 0 && (int)If_ObjCutBest(pAnd)->nLeaves >= nLutMin )
            pObj->fMarkA = 1;
    }
    Abc_NtkDelete( pNtk );
    If_ManStop( pIfMan );

    // unmark flop drivers
    Saig_ManForEachLi( pAig, pObj, i )
        Aig_ObjFanin0(pObj)->fMarkA = 0;

    // collect mapping
    vNodes = Vec_IntAlloc( 100 );
    Aig_ManForEachNode( pAig, pObj, i )
        if ( pObj->fMarkA )
            Vec_IntPush( vNodes, Aig_ObjId(pObj) );
    Aig_ManCleanMarkA( pAig );
    return vNodes;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

