/**CFile****************************************************************

  FileName    [dchSweep.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Choice computation for tech-mapping.]

  Synopsis    [One round of SAT sweeping.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 29, 2008.]

  Revision    [$Id: dchSweep.c,v 1.00 2008/07/29 00:00:00 alanmi Exp $]

***********************************************************************/

#include "dchInt.h"
#include "misc/bar/bar.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static inline Aig_Obj_t * Dch_ObjChild0Fra( Aig_Obj_t * pObj ) { assert( !Aig_IsComplement(pObj) ); return Aig_ObjFanin0(pObj)? Aig_NotCond(Dch_ObjFraig(Aig_ObjFanin0(pObj)), Aig_ObjFaninC0(pObj)) : NULL;  }
static inline Aig_Obj_t * Dch_ObjChild1Fra( Aig_Obj_t * pObj ) { assert( !Aig_IsComplement(pObj) ); return Aig_ObjFanin1(pObj)? Aig_NotCond(Dch_ObjFraig(Aig_ObjFanin1(pObj)), Aig_ObjFaninC1(pObj)) : NULL;  }

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Performs fraiging for one node.]

  Description [Returns the fraiged node.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Dch_ManSweepNode( Dch_Man_t * p, Aig_Obj_t * pObj )
{ 
    Aig_Obj_t * pObjRepr, * pObjFraig, * pObjFraig2, * pObjReprFraig;
    int RetValue;
    // get representative of this class
    pObjRepr = Aig_ObjRepr( p->pAigTotal, pObj );
    if ( pObjRepr == NULL )
        return;
    // get the fraiged node
    pObjFraig = Dch_ObjFraig( pObj );
    if ( pObjFraig == NULL )
        return;
    // get the fraiged representative
    pObjReprFraig = Dch_ObjFraig( pObjRepr );
    if ( pObjReprFraig == NULL )
        return;
    // if the fraiged nodes are the same, return
    if ( Aig_Regular(pObjFraig) == Aig_Regular(pObjReprFraig) )
    {
        // remember the proved equivalence
        p->pReprsProved[ pObj->Id ] = pObjRepr;
        return;
    }
    assert( Aig_Regular(pObjFraig) != Aig_ManConst1(p->pAigFraig) );
    RetValue = Dch_NodesAreEquiv( p, Aig_Regular(pObjReprFraig), Aig_Regular(pObjFraig) );
    if ( RetValue == -1 ) // timed out
    {
        Dch_ObjSetFraig( pObj, NULL );
        return;
    }
    if ( RetValue == 1 )  // proved equivalent
    {
        pObjFraig2 = Aig_NotCond( pObjReprFraig, pObj->fPhase ^ pObjRepr->fPhase );
        Dch_ObjSetFraig( pObj, pObjFraig2 );
        // remember the proved equivalence
        p->pReprsProved[ pObj->Id ] = pObjRepr;
        return;
    }
    // disproved the equivalence
    if ( p->pPars->fSimulateTfo )
        Dch_ManResimulateCex( p, pObj, pObjRepr );
    else
        Dch_ManResimulateCex2( p, pObj, pObjRepr );
    assert( Aig_ObjRepr( p->pAigTotal, pObj ) != pObjRepr );
}

/**Function*************************************************************

  Synopsis    [Performs fraiging for the internal nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Dch_ManSweep( Dch_Man_t * p )
{
    Bar_Progress_t * pProgress = NULL;
    Aig_Obj_t * pObj, * pObjNew;
    int i;
    // map constants and PIs
    p->pAigFraig = Aig_ManStart( Aig_ManObjNumMax(p->pAigTotal) );
    Aig_ManCleanData( p->pAigTotal );
    Aig_ManConst1(p->pAigTotal)->pData = Aig_ManConst1(p->pAigFraig);
    Aig_ManForEachCi( p->pAigTotal, pObj, i )
        pObj->pData = Aig_ObjCreateCi( p->pAigFraig );
    // sweep internal nodes
    pProgress = Bar_ProgressStart( stdout, Aig_ManObjNumMax(p->pAigTotal) );
    Aig_ManForEachNode( p->pAigTotal, pObj, i )
    {
        Bar_ProgressUpdate( pProgress, i, NULL );
        if ( Dch_ObjFraig(Aig_ObjFanin0(pObj)) == NULL || 
             Dch_ObjFraig(Aig_ObjFanin1(pObj)) == NULL )
            continue;
        pObjNew = Aig_And( p->pAigFraig, Dch_ObjChild0Fra(pObj), Dch_ObjChild1Fra(pObj) );
        if ( pObjNew == NULL )
            continue;
        Dch_ObjSetFraig( pObj, pObjNew );
        Dch_ManSweepNode( p, pObj );
    }
    Bar_ProgressStop( pProgress );
    // update the representatives of the nodes (makes classes invalid)
    ABC_FREE( p->pAigTotal->pReprs );
    p->pAigTotal->pReprs = p->pReprsProved;
    p->pReprsProved = NULL;
    // clean the mark
    Aig_ManCleanMarkB( p->pAigTotal );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

