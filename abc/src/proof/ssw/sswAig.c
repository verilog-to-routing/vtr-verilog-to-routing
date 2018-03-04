/**CFile****************************************************************

  FileName    [sswAig.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Inductive prover with constraints.]

  Synopsis    [AIG manipulation.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 1, 2008.]

  Revision    [$Id: sswAig.c,v 1.00 2008/09/01 00:00:00 alanmi Exp $]

***********************************************************************/

#include "sswInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Starts the SAT manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ssw_Frm_t * Ssw_FrmStart( Aig_Man_t * pAig )
{
    Ssw_Frm_t * p;
    p = ABC_ALLOC( Ssw_Frm_t, 1 );
    memset( p, 0, sizeof(Ssw_Frm_t) );
    p->pAig          = pAig;
    p->nObjs         = Aig_ManObjNumMax( pAig );
    p->nFrames       = 0;
    p->pFrames       = NULL;
    p->vAig2Frm      = Vec_PtrAlloc( 0 );
    Vec_PtrFill( p->vAig2Frm, 2 * p->nObjs, NULL );
    return p;
}

/**Function*************************************************************

  Synopsis    [Starts the SAT manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ssw_FrmStop( Ssw_Frm_t * p )
{
    if ( p->pFrames )
        Aig_ManStop( p->pFrames );
    Vec_PtrFree( p->vAig2Frm );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    [Performs speculative reduction for one node.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Ssw_FramesConstrainNode( Ssw_Man_t * p, Aig_Man_t * pFrames, Aig_Man_t * pAig, Aig_Obj_t * pObj, int iFrame, int fTwoPos )
{
    Aig_Obj_t * pObjNew, * pObjNew2, * pObjRepr, * pObjReprNew, * pMiter;
    // skip nodes without representative
    pObjRepr = Aig_ObjRepr(pAig, pObj);
    if ( pObjRepr == NULL )
        return;
    p->nConstrTotal++;
    assert( pObjRepr->Id < pObj->Id );
    // get the new node
    pObjNew = Ssw_ObjFrame( p, pObj, iFrame );
    // get the new node of the representative
    pObjReprNew = Ssw_ObjFrame( p, pObjRepr, iFrame );
    // if this is the same node, no need to add constraints
    if ( pObj->fPhase == pObjRepr->fPhase )
    {
        assert( pObjNew != Aig_Not(pObjReprNew) );
        if ( pObjNew == pObjReprNew )
            return;
    }
    else
    {
        assert( pObjNew != pObjReprNew );
        if ( pObjNew == Aig_Not(pObjReprNew) )
            return;
    }
    p->nConstrReduced++;
    // these are different nodes - perform speculative reduction
    pObjNew2 = Aig_NotCond( pObjReprNew, pObj->fPhase ^ pObjRepr->fPhase );
    // set the new node
    Ssw_ObjSetFrame( p, pObj, iFrame, pObjNew2 );
    // add the constraint
    if ( fTwoPos )
    {
        Aig_ObjCreateCo( pFrames, pObjNew2 );
        Aig_ObjCreateCo( pFrames, pObjNew );
    }
    else
    {
        pMiter = Aig_Exor( pFrames, pObjNew, pObjNew2 );
        Aig_ObjCreateCo( pFrames, Aig_NotCond(pMiter, Aig_ObjPhaseReal(pMiter)) );
    }
}

/**Function*************************************************************

  Synopsis    [Prepares the inductive case with speculative reduction.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Ssw_FramesWithClasses( Ssw_Man_t * p )
{
    Aig_Man_t * pFrames;
    Aig_Obj_t * pObj, * pObjLi, * pObjLo, * pObjNew;
    int i, f, iLits;
    assert( p->pFrames == NULL );
    assert( Aig_ManRegNum(p->pAig) > 0 );
    assert( Aig_ManRegNum(p->pAig) < Aig_ManCiNum(p->pAig) );
    p->nConstrTotal = p->nConstrReduced = 0;

    // start the fraig package
    pFrames = Aig_ManStart( Aig_ManObjNumMax(p->pAig) * p->nFrames );
    // create latches for the first frame
    Saig_ManForEachLo( p->pAig, pObj, i )
        Ssw_ObjSetFrame( p, pObj, 0, Aig_ObjCreateCi(pFrames) );
    // add timeframes
    iLits = 0;
    for ( f = 0; f < p->pPars->nFramesK; f++ )
    {
        // map constants and PIs
        Ssw_ObjSetFrame( p, Aig_ManConst1(p->pAig), f, Aig_ManConst1(pFrames) );
        Saig_ManForEachPi( p->pAig, pObj, i )
        {
            pObjNew = Aig_ObjCreateCi(pFrames);
            pObjNew->fPhase = (p->vInits != NULL) && Vec_IntEntry(p->vInits, iLits++);
            Ssw_ObjSetFrame( p, pObj, f, pObjNew );
        }
        // set the constraints on the latch outputs
        Saig_ManForEachLo( p->pAig, pObj, i )
            Ssw_FramesConstrainNode( p, pFrames, p->pAig, pObj, f, 1 );
        // add internal nodes of this frame
        Aig_ManForEachNode( p->pAig, pObj, i )
        {
            pObjNew = Aig_And( pFrames, Ssw_ObjChild0Fra(p, pObj, f), Ssw_ObjChild1Fra(p, pObj, f) );
            Ssw_ObjSetFrame( p, pObj, f, pObjNew );
            Ssw_FramesConstrainNode( p, pFrames, p->pAig, pObj, f, 1 );
        }
        // transfer to the primary outputs
        Aig_ManForEachCo( p->pAig, pObj, i )
            Ssw_ObjSetFrame( p, pObj, f, Ssw_ObjChild0Fra(p, pObj,f) );
        // transfer latch input to the latch outputs
        Saig_ManForEachLiLo( p->pAig, pObjLi, pObjLo, i )
            Ssw_ObjSetFrame( p, pObjLo, f+1, Ssw_ObjFrame(p, pObjLi,f) );
    }
    assert( p->vInits == NULL || Vec_IntSize(p->vInits) == iLits + Saig_ManPiNum(p->pAig) );
    // add the POs for the latch outputs of the last frame
    Saig_ManForEachLo( p->pAig, pObj, i )
        Aig_ObjCreateCo( pFrames, Ssw_ObjFrame( p, pObj, p->pPars->nFramesK ) );

    // remove dangling nodes
    Aig_ManCleanup( pFrames );
    // make sure the satisfying assignment is node assigned
    assert( pFrames->pData == NULL );
//Aig_ManShow( pFrames, 0, NULL );
    return pFrames;
}

/**Function*************************************************************

  Synopsis    [Prepares the inductive case with speculative reduction.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Ssw_SpeculativeReduction( Ssw_Man_t * p )
{
    Aig_Man_t * pFrames;
    Aig_Obj_t * pObj, * pObjNew;
    int i;
    assert( p->pFrames == NULL );
    assert( Aig_ManRegNum(p->pAig) > 0 );
    assert( Aig_ManRegNum(p->pAig) < Aig_ManCiNum(p->pAig) );
    p->nConstrTotal = p->nConstrReduced = 0;

    // start the fraig package
    pFrames = Aig_ManStart( Aig_ManObjNumMax(p->pAig) * p->nFrames );
    pFrames->pName = Abc_UtilStrsav( p->pAig->pName );
    // map constants and PIs
    Ssw_ObjSetFrame( p, Aig_ManConst1(p->pAig), 0, Aig_ManConst1(pFrames) );
    Saig_ManForEachPi( p->pAig, pObj, i )
        Ssw_ObjSetFrame( p, pObj, 0, Aig_ObjCreateCi(pFrames) );
    // create latches for the first frame
    Saig_ManForEachLo( p->pAig, pObj, i )
        Ssw_ObjSetFrame( p, pObj, 0, Aig_ObjCreateCi(pFrames) );
    // set the constraints on the latch outputs
    Saig_ManForEachLo( p->pAig, pObj, i )
        Ssw_FramesConstrainNode( p, pFrames, p->pAig, pObj, 0, 0 );
    // add internal nodes of this frame
    Aig_ManForEachNode( p->pAig, pObj, i )
    {
        pObjNew = Aig_And( pFrames, Ssw_ObjChild0Fra(p, pObj, 0), Ssw_ObjChild1Fra(p, pObj, 0) );
        Ssw_ObjSetFrame( p, pObj, 0, pObjNew );
        Ssw_FramesConstrainNode( p, pFrames, p->pAig, pObj, 0, 0 );
    }
    // add the POs for the latch outputs of the last frame
    Saig_ManForEachLi( p->pAig, pObj, i )
        Aig_ObjCreateCo( pFrames, Ssw_ObjChild0Fra(p, pObj,0) );
    // remove dangling nodes
    Aig_ManCleanup( pFrames );
    Aig_ManSetRegNum( pFrames, Aig_ManRegNum(p->pAig) );
//    Abc_Print( 1, "SpecRed: Total constraints = %d. Reduced constraints = %d.\n", 
//        p->nConstrTotal, p->nConstrReduced );
    return pFrames;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END
