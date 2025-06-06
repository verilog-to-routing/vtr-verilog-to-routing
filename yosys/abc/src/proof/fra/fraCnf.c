/**CFile****************************************************************

  FileName    [fraCnf.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [New FRAIG package.]

  Synopsis    []

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 30, 2007.]

  Revision    [$Id: fraCnf.c,v 1.00 2007/06/30 00:00:00 alanmi Exp $]

***********************************************************************/

#include "fra.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////


/**Function*************************************************************

  Synopsis    [Addes clauses to the solver.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fra_AddClausesMux( Fra_Man_t * p, Aig_Obj_t * pNode )
{
    Aig_Obj_t * pNodeI, * pNodeT, * pNodeE;
    int pLits[4], RetValue, VarF, VarI, VarT, VarE, fCompT, fCompE;

    assert( !Aig_IsComplement( pNode ) );
    assert( Aig_ObjIsMuxType( pNode ) );
    // get nodes (I = if, T = then, E = else)
    pNodeI = Aig_ObjRecognizeMux( pNode, &pNodeT, &pNodeE );
    // get the variable numbers
    VarF = Fra_ObjSatNum(pNode);
    VarI = Fra_ObjSatNum(pNodeI);
    VarT = Fra_ObjSatNum(Aig_Regular(pNodeT));
    VarE = Fra_ObjSatNum(Aig_Regular(pNodeE));
    // get the complementation flags
    fCompT = Aig_IsComplement(pNodeT);
    fCompE = Aig_IsComplement(pNodeE);

    // f = ITE(i, t, e)

    // i' + t' + f
    // i' + t  + f'
    // i  + e' + f
    // i  + e  + f'

    // create four clauses
    pLits[0] = toLitCond(VarI, 1);
    pLits[1] = toLitCond(VarT, 1^fCompT);
    pLits[2] = toLitCond(VarF, 0);
    RetValue = sat_solver_addclause( p->pSat, pLits, pLits + 3 );
    assert( RetValue );
    pLits[0] = toLitCond(VarI, 1);
    pLits[1] = toLitCond(VarT, 0^fCompT);
    pLits[2] = toLitCond(VarF, 1);
    RetValue = sat_solver_addclause( p->pSat, pLits, pLits + 3 );
    assert( RetValue );
    pLits[0] = toLitCond(VarI, 0);
    pLits[1] = toLitCond(VarE, 1^fCompE);
    pLits[2] = toLitCond(VarF, 0);
    RetValue = sat_solver_addclause( p->pSat, pLits, pLits + 3 );
    assert( RetValue );
    pLits[0] = toLitCond(VarI, 0);
    pLits[1] = toLitCond(VarE, 0^fCompE);
    pLits[2] = toLitCond(VarF, 1);
    RetValue = sat_solver_addclause( p->pSat, pLits, pLits + 3 );
    assert( RetValue );

    // two additional clauses
    // t' & e' -> f'
    // t  & e  -> f 

    // t  + e   + f'
    // t' + e'  + f 

    if ( VarT == VarE )
    {
//        assert( fCompT == !fCompE );
        return;
    }

    pLits[0] = toLitCond(VarT, 0^fCompT);
    pLits[1] = toLitCond(VarE, 0^fCompE);
    pLits[2] = toLitCond(VarF, 1);
    RetValue = sat_solver_addclause( p->pSat, pLits, pLits + 3 );
    assert( RetValue );
    pLits[0] = toLitCond(VarT, 1^fCompT);
    pLits[1] = toLitCond(VarE, 1^fCompE);
    pLits[2] = toLitCond(VarF, 0);
    RetValue = sat_solver_addclause( p->pSat, pLits, pLits + 3 );
    assert( RetValue );
}

/**Function*************************************************************

  Synopsis    [Addes clauses to the solver.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fra_AddClausesSuper( Fra_Man_t * p, Aig_Obj_t * pNode, Vec_Ptr_t * vSuper )
{
    Aig_Obj_t * pFanin;
    int * pLits, nLits, RetValue, i;
    assert( !Aig_IsComplement(pNode) );
    assert( Aig_ObjIsNode( pNode ) );
    // create storage for literals
    nLits = Vec_PtrSize(vSuper) + 1;
    pLits = ABC_ALLOC( int, nLits );
    // suppose AND-gate is A & B = C
    // add !A => !C   or   A + !C
    Vec_PtrForEachEntry( Aig_Obj_t *, vSuper, pFanin, i )
    {
        pLits[0] = toLitCond(Fra_ObjSatNum(Aig_Regular(pFanin)), Aig_IsComplement(pFanin));
        pLits[1] = toLitCond(Fra_ObjSatNum(pNode), 1);
        RetValue = sat_solver_addclause( p->pSat, pLits, pLits + 2 );
        assert( RetValue );
    }
    // add A & B => C   or   !A + !B + C
    Vec_PtrForEachEntry( Aig_Obj_t *, vSuper, pFanin, i )
        pLits[i] = toLitCond(Fra_ObjSatNum(Aig_Regular(pFanin)), !Aig_IsComplement(pFanin));
    pLits[nLits-1] = toLitCond(Fra_ObjSatNum(pNode), 0);
    RetValue = sat_solver_addclause( p->pSat, pLits, pLits + nLits );
    assert( RetValue );
    ABC_FREE( pLits );
}

/**Function*************************************************************

  Synopsis    [Collects the supergate.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fra_CollectSuper_rec( Aig_Obj_t * pObj, Vec_Ptr_t * vSuper, int fFirst, int fUseMuxes )
{
    // if the new node is complemented or a PI, another gate begins
    if ( Aig_IsComplement(pObj) || Aig_ObjIsCi(pObj) || (!fFirst && Aig_ObjRefs(pObj) > 1) || 
         (fUseMuxes && Aig_ObjIsMuxType(pObj)) )
    {
        Vec_PtrPushUnique( vSuper, pObj );
        return;
    }
    // go through the branches
    Fra_CollectSuper_rec( Aig_ObjChild0(pObj), vSuper, 0, fUseMuxes );
    Fra_CollectSuper_rec( Aig_ObjChild1(pObj), vSuper, 0, fUseMuxes );
}

/**Function*************************************************************

  Synopsis    [Collects the supergate.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Fra_CollectSuper( Aig_Obj_t * pObj, int fUseMuxes )
{
    Vec_Ptr_t * vSuper;
    assert( !Aig_IsComplement(pObj) );
    assert( !Aig_ObjIsCi(pObj) );
    vSuper = Vec_PtrAlloc( 4 );
    Fra_CollectSuper_rec( pObj, vSuper, 1, fUseMuxes );
    return vSuper;
}

/**Function*************************************************************

  Synopsis    [Updates the solver clause database.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fra_ObjAddToFrontier( Fra_Man_t * p, Aig_Obj_t * pObj, Vec_Ptr_t * vFrontier )
{
    assert( !Aig_IsComplement(pObj) );
    if ( Fra_ObjSatNum(pObj) )
        return;
    assert( Fra_ObjSatNum(pObj) == 0 );
    assert( Fra_ObjFaninVec(pObj) == NULL );
    if ( Aig_ObjIsConst1(pObj) )
        return;
    Fra_ObjSetSatNum( pObj, p->nSatVars++ );
    if ( Aig_ObjIsNode(pObj) )
        Vec_PtrPush( vFrontier, pObj );
}

/**Function*************************************************************

  Synopsis    [Updates the solver clause database.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fra_CnfNodeAddToSolver( Fra_Man_t * p, Aig_Obj_t * pOld, Aig_Obj_t * pNew )
{ 
    Vec_Ptr_t * vFrontier, * vFanins;
    Aig_Obj_t * pNode, * pFanin;
    int i, k, fUseMuxes = 1;
    assert( pOld || pNew );
    // quit if CNF is ready
    if ( (!pOld || Fra_ObjFaninVec(pOld)) && (!pNew || Fra_ObjFaninVec(pNew)) )
        return;
    // start the frontier
    vFrontier = Vec_PtrAlloc( 100 );
    if ( pOld ) Fra_ObjAddToFrontier( p, pOld, vFrontier );
    if ( pNew ) Fra_ObjAddToFrontier( p, pNew, vFrontier );
    // explore nodes in the frontier
    Vec_PtrForEachEntry( Aig_Obj_t *, vFrontier, pNode, i )
    {
        // create the supergate
        assert( Fra_ObjSatNum(pNode) );
        assert( Fra_ObjFaninVec(pNode) == NULL );
        if ( fUseMuxes && Aig_ObjIsMuxType(pNode) )
        {
            vFanins = Vec_PtrAlloc( 4 );
            Vec_PtrPushUnique( vFanins, Aig_ObjFanin0( Aig_ObjFanin0(pNode) ) );
            Vec_PtrPushUnique( vFanins, Aig_ObjFanin0( Aig_ObjFanin1(pNode) ) );
            Vec_PtrPushUnique( vFanins, Aig_ObjFanin1( Aig_ObjFanin0(pNode) ) );
            Vec_PtrPushUnique( vFanins, Aig_ObjFanin1( Aig_ObjFanin1(pNode) ) );
            Vec_PtrForEachEntry( Aig_Obj_t *, vFanins, pFanin, k )
                Fra_ObjAddToFrontier( p, Aig_Regular(pFanin), vFrontier );
            Fra_AddClausesMux( p, pNode );
        }
        else
        {
            vFanins = Fra_CollectSuper( pNode, fUseMuxes );
            Vec_PtrForEachEntry( Aig_Obj_t *, vFanins, pFanin, k )
                Fra_ObjAddToFrontier( p, Aig_Regular(pFanin), vFrontier );
            Fra_AddClausesSuper( p, pNode, vFanins );
        }
        assert( Vec_PtrSize(vFanins) > 1 );
        Fra_ObjSetFaninVec( pNode, vFanins );
    }
    Vec_PtrFree( vFrontier );
}



////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

