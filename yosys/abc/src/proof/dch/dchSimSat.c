/**CFile****************************************************************

  FileName    [dchSimSat.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Choice computation for tech-mapping.]

  Synopsis    [Performs resimulation using counter-examples.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 29, 2008.]

  Revision    [$Id: dchSimSat.c,v 1.00 2008/07/29 00:00:00 alanmi Exp $]

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

  Synopsis    [Collects internal nodes in the reverse DFS order.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Dch_ManCollectTfoCands_rec( Dch_Man_t * p, Aig_Obj_t * pObj )
{
    Aig_Obj_t * pFanout, * pRepr;
    int iFanout = -1, i;
    assert( !Aig_IsComplement(pObj) );
    if ( Aig_ObjIsTravIdCurrent(p->pAigTotal, pObj) )
        return;
    Aig_ObjSetTravIdCurrent(p->pAigTotal, pObj);
    // traverse the fanouts
    Aig_ObjForEachFanout( p->pAigTotal, pObj, pFanout, iFanout, i )
        Dch_ManCollectTfoCands_rec( p, pFanout );
    // check if the given node has a representative
    pRepr = Aig_ObjRepr( p->pAigTotal, pObj );
    if ( pRepr == NULL )
        return;
    // pRepr is the constant 1 node
    if ( pRepr == Aig_ManConst1(p->pAigTotal) )
    {
        Vec_PtrPush( p->vSimRoots, pObj );
        return;
    }
    // pRepr is the representative of an equivalence class
    if ( pRepr->fMarkA )
        return;
    pRepr->fMarkA = 1;
    Vec_PtrPush( p->vSimClasses, pRepr );
}

/**Function*************************************************************

  Synopsis    [Collect equivalence classes and const1 cands in the TFO.]

  Description []
               
  SideEffects []

  SeeAlso     []
 
***********************************************************************/
void Dch_ManCollectTfoCands( Dch_Man_t * p, Aig_Obj_t * pObj1, Aig_Obj_t * pObj2 )
{
    Aig_Obj_t * pObj;
    int i;
    Vec_PtrClear( p->vSimRoots );
    Vec_PtrClear( p->vSimClasses );
    Aig_ManIncrementTravId( p->pAigTotal );
    Aig_ObjSetTravIdCurrent( p->pAigTotal, Aig_ManConst1(p->pAigTotal) );    
    Dch_ManCollectTfoCands_rec( p, pObj1 );
    Dch_ManCollectTfoCands_rec( p, pObj2 );
    Vec_PtrSort( p->vSimRoots, (int (*)(const void *, const void *))Aig_ObjCompareIdIncrease );
    Vec_PtrSort( p->vSimClasses, (int (*)(const void *, const void *))Aig_ObjCompareIdIncrease );
    Vec_PtrForEachEntry( Aig_Obj_t *, p->vSimClasses, pObj, i )
        pObj->fMarkA = 0;
}

/**Function*************************************************************

  Synopsis    [Resimulates the cone of influence of the solved nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []
 
***********************************************************************/
void Dch_ManResimulateSolved_rec( Dch_Man_t * p, Aig_Obj_t * pObj )
{
    if ( Aig_ObjIsTravIdCurrent(p->pAigTotal, pObj) )
        return;
    Aig_ObjSetTravIdCurrent(p->pAigTotal, pObj);
    if ( Aig_ObjIsCi(pObj) )
    {
        Aig_Obj_t * pObjFraig;
        int nVarNum;
        pObjFraig = Dch_ObjFraig( pObj );
        assert( !Aig_IsComplement(pObjFraig) );
        nVarNum = Dch_ObjSatNum( p, pObjFraig );
        // get the value from the SAT solver
        // (account for the fact that some vars may be minimized away)
        pObj->fMarkB = !nVarNum? 0 : sat_solver_var_value( p->pSat, nVarNum );
//        pObj->fMarkB = !nVarNum? Aig_ManRandom(0) & 1 : sat_solver_var_value( p->pSat, nVarNum );
        return;
    }
    Dch_ManResimulateSolved_rec( p, Aig_ObjFanin0(pObj) );
    Dch_ManResimulateSolved_rec( p, Aig_ObjFanin1(pObj) );
    pObj->fMarkB = ( Aig_ObjFanin0(pObj)->fMarkB ^ Aig_ObjFaninC0(pObj) )
                 & ( Aig_ObjFanin1(pObj)->fMarkB ^ Aig_ObjFaninC1(pObj) );
    // count the cone size
    if ( Dch_ObjSatNum( p, Aig_Regular(Dch_ObjFraig(pObj)) ) > 0 )
        p->nConeThis++;
}

/**Function*************************************************************

  Synopsis    [Resimulates the cone of influence of the other nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Dch_ManResimulateOther_rec( Dch_Man_t * p, Aig_Obj_t * pObj )
{
    if ( Aig_ObjIsTravIdCurrent(p->pAigTotal, pObj) )
        return;
    Aig_ObjSetTravIdCurrent(p->pAigTotal, pObj);
    if ( Aig_ObjIsCi(pObj) )
    {
        // set random value
        pObj->fMarkB = Aig_ManRandom(0) & 1;
        return;
    }
    Dch_ManResimulateOther_rec( p, Aig_ObjFanin0(pObj) );
    Dch_ManResimulateOther_rec( p, Aig_ObjFanin1(pObj) );
    pObj->fMarkB = ( Aig_ObjFanin0(pObj)->fMarkB ^ Aig_ObjFaninC0(pObj) )
                 & ( Aig_ObjFanin1(pObj)->fMarkB ^ Aig_ObjFaninC1(pObj) );
}

/**Function*************************************************************

  Synopsis    [Handle the counter-example.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Dch_ManResimulateCex( Dch_Man_t * p, Aig_Obj_t * pObj, Aig_Obj_t * pRepr )
{
    Aig_Obj_t * pRoot, ** ppClass;
    int i, k, nSize, RetValue1, RetValue2;
    abctime clk = Abc_Clock();
    // get the equivalence classes
    Dch_ManCollectTfoCands( p, pObj, pRepr );
    // resimulate the cone of influence of the solved nodes
    p->nConeThis = 0;
    Aig_ManIncrementTravId( p->pAigTotal );
    Aig_ObjSetTravIdCurrent( p->pAigTotal, Aig_ManConst1(p->pAigTotal) );
    Dch_ManResimulateSolved_rec( p, pObj );
    Dch_ManResimulateSolved_rec( p, pRepr );
    p->nConeMax = Abc_MaxInt( p->nConeMax, p->nConeThis );
    // resimulate the cone of influence of the other nodes
    Vec_PtrForEachEntry( Aig_Obj_t *, p->vSimRoots, pRoot, i )
        Dch_ManResimulateOther_rec( p, pRoot );
    // refine these nodes
    RetValue1 = Dch_ClassesRefineConst1Group( p->ppClasses, p->vSimRoots, 0 );
    // resimulate the cone of influence of the cand classes
    RetValue2 = 0;
    Vec_PtrForEachEntry( Aig_Obj_t *, p->vSimClasses, pRoot, i )
    {
        ppClass = Dch_ClassesReadClass( p->ppClasses, pRoot, &nSize );
        for ( k = 0; k < nSize; k++ )
            Dch_ManResimulateOther_rec( p, ppClass[k] );
        // refine this class
        RetValue2 += Dch_ClassesRefineOneClass( p->ppClasses, pRoot, 0 );
    }
    // make sure refinement happened
    if ( Aig_ObjIsConst1(pRepr) )
        assert( RetValue1 );
    else
        assert( RetValue2 );
p->timeSimSat += Abc_Clock() - clk;
}

/**Function*************************************************************

  Synopsis    [Handle the counter-example.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Dch_ManResimulateCex2( Dch_Man_t * p, Aig_Obj_t * pObj, Aig_Obj_t * pRepr )
{
    Aig_Obj_t * pRoot;
    int i, RetValue;
    abctime clk = Abc_Clock();
    // get the equivalence class
    if ( Dch_ObjIsConst1Cand(p->pAigTotal, pObj) )
        Dch_ClassesCollectConst1Group( p->ppClasses, pObj, 500, p->vSimRoots );
    else
        Dch_ClassesCollectOneClass( p->ppClasses, pRepr, p->vSimRoots );
    // resimulate the cone of influence of the solved nodes
    p->nConeThis = 0;
    Aig_ManIncrementTravId( p->pAigTotal );
    Aig_ObjSetTravIdCurrent( p->pAigTotal, Aig_ManConst1(p->pAigTotal) );
    Dch_ManResimulateSolved_rec( p, pObj );
    Dch_ManResimulateSolved_rec( p, pRepr );
    p->nConeMax = Abc_MaxInt( p->nConeMax, p->nConeThis );
    // resimulate the cone of influence of the other nodes
    Vec_PtrForEachEntry( Aig_Obj_t *, p->vSimRoots, pRoot, i )
        Dch_ManResimulateOther_rec( p, pRoot );
    // refine this class
    if ( Dch_ObjIsConst1Cand(p->pAigTotal, pObj) )
        RetValue = Dch_ClassesRefineConst1Group( p->ppClasses, p->vSimRoots, 0 );
    else
        RetValue = Dch_ClassesRefineOneClass( p->ppClasses, pRepr, 0 );
    assert( RetValue );
p->timeSimSat += Abc_Clock() - clk;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

