/**CFile****************************************************************

  FileName    [ifSelect.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [FPGA mapping based on priority cuts.]

  Synopsis    [Cut selection procedures.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - November 21, 2006.]

  Revision    [$Id: ifSelect.c,v 1.00 2006/11/21 00:00:00 alanmi Exp $]

***********************************************************************/

#include "if.h"
#include "sat/bsat/satSolver.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Prints the logic cone with choices.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void If_ObjConePrint_rec( If_Man_t * pIfMan, If_Obj_t * pIfObj, Vec_Ptr_t * vVisited )
{
    If_Cut_t * pCut;
    pCut = If_ObjCutBest(pIfObj);
    if ( If_CutDataInt(pCut) )
        return;
    Vec_PtrPush( vVisited, pCut );
    // insert the worst case
    If_CutSetDataInt( pCut, ~0 );
    // skip in case of primary input
    if ( If_ObjIsCi(pIfObj) )
        return;
    // compute the functions of the children
    if ( pIfObj->pEquiv ) 
    If_ObjConePrint_rec( pIfMan, pIfObj->pEquiv, vVisited );
    If_ObjConePrint_rec( pIfMan, pIfObj->pFanin0, vVisited );
    If_ObjConePrint_rec( pIfMan, pIfObj->pFanin1, vVisited );
    printf( "%5d = %5d & %5d | %5d\n", pIfObj->Id, pIfObj->pFanin0->Id, pIfObj->pFanin1->Id, pIfObj->pEquiv ? pIfObj->pEquiv->Id : 0 );
}
void If_ObjConePrint( If_Man_t * pIfMan, If_Obj_t * pIfObj )
{
    If_Cut_t * pCut;
    If_Obj_t * pLeaf;
    int i;
    Vec_PtrClear( pIfMan->vTemp );
    If_ObjConePrint_rec( pIfMan, pIfObj, pIfMan->vTemp );
    Vec_PtrForEachEntry( If_Cut_t *, pIfMan->vTemp, pCut, i )
        If_CutSetDataInt( pCut, 0 );
    // print the leaf variables
    printf( "Cut " );
    pCut = If_ObjCutBest(pIfObj);
    If_CutForEachLeaf( pIfMan, pCut, pLeaf, i )
        printf( "%d ", pLeaf->Id );
    printf( "\n" );
}


/**Function*************************************************************

  Synopsis    [Recursively derives local AIG for the cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int If_ManNodeShapeMap_rec( If_Man_t * pIfMan, If_Obj_t * pIfObj, Vec_Ptr_t * vVisited, Vec_Int_t * vShape )
{
    If_Cut_t * pCut;
    If_Obj_t * pTemp;
    int i, iFunc0, iFunc1;
    // get the best cut
    pCut = If_ObjCutBest(pIfObj);
    // if the cut is visited, return the result
    if ( If_CutDataInt(pCut) )
        return If_CutDataInt(pCut);
    // mark the node as visited
    Vec_PtrPush( vVisited, pCut );
    // insert the worst case
    If_CutSetDataInt( pCut, ~0 );
    // skip in case of primary input
    if ( If_ObjIsCi(pIfObj) )
        return If_CutDataInt(pCut);
    // compute the functions of the children
    for ( i = 0, pTemp = pIfObj; pTemp; pTemp = pTemp->pEquiv, i++ )
    {
        iFunc0 = If_ManNodeShapeMap_rec( pIfMan, pTemp->pFanin0, vVisited, vShape );
        if ( iFunc0 == ~0 )
            continue;
        iFunc1 = If_ManNodeShapeMap_rec( pIfMan, pTemp->pFanin1, vVisited, vShape );
        if ( iFunc1 == ~0 )
            continue;
        // both branches are solved
        Vec_IntPush( vShape, pIfObj->Id );
        Vec_IntPush( vShape, pTemp->Id );
        If_CutSetDataInt( pCut, 1 );
        break;
    }
    return If_CutDataInt(pCut);
}
int If_ManNodeShapeMap( If_Man_t * pIfMan, If_Obj_t * pIfObj, Vec_Int_t * vShape )
{
    If_Cut_t * pCut;
    If_Obj_t * pLeaf;
    int i, iRes;
    // get the best cut
    pCut = If_ObjCutBest(pIfObj);
    assert( pCut->nLeaves > 1 );
    // set the leaf variables
    If_CutForEachLeaf( pIfMan, pCut, pLeaf, i )
    {
        assert( If_CutDataInt( If_ObjCutBest(pLeaf) ) == 0 );
        If_CutSetDataInt( If_ObjCutBest(pLeaf), 1 );
    }
    // recursively compute the function while collecting visited cuts
    Vec_IntClear( vShape );
    Vec_PtrClear( pIfMan->vTemp );
    iRes = If_ManNodeShapeMap_rec( pIfMan, pIfObj, pIfMan->vTemp, vShape ); 
    if ( iRes == ~0 )
    {
        Abc_Print( -1, "If_ManNodeShapeMap(): Computing local AIG has failed.\n" );
        return 0;
    }
    // clean the cuts
    If_CutForEachLeaf( pIfMan, pCut, pLeaf, i )
        If_CutSetDataInt( If_ObjCutBest(pLeaf), 0 );
    Vec_PtrForEachEntry( If_Cut_t *, pIfMan->vTemp, pCut, i )
        If_CutSetDataInt( pCut, 0 );
    return 1;
}


/**Function*************************************************************

  Synopsis    [Recursively derives the local AIG for the cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int  If_WordCountOnes( unsigned uWord )
{
    uWord = (uWord & 0x55555555) + ((uWord>>1) & 0x55555555);
    uWord = (uWord & 0x33333333) + ((uWord>>2) & 0x33333333);
    uWord = (uWord & 0x0F0F0F0F) + ((uWord>>4) & 0x0F0F0F0F);
    uWord = (uWord & 0x00FF00FF) + ((uWord>>8) & 0x00FF00FF);
    return  (uWord & 0x0000FFFF) + (uWord>>16);
}
int If_ManNodeShapeMap2_rec( If_Man_t * pIfMan, If_Obj_t * pIfObj, Vec_Ptr_t * vVisited, Vec_Int_t * vShape )
{
    If_Cut_t * pCut;
    If_Obj_t * pTemp, * pTempBest = NULL;
    int i, iFunc, iFunc0, iFunc1, iBest = 0;
    // get the best cut
    pCut = If_ObjCutBest(pIfObj);
    // if the cut is visited, return the result
    if ( If_CutDataInt(pCut) )
        return If_CutDataInt(pCut);
    // mark the node as visited
    Vec_PtrPush( vVisited, pCut );
    // insert the worst case
    If_CutSetDataInt( pCut, ~0 );
    // skip in case of primary input
    if ( If_ObjIsCi(pIfObj) )
        return If_CutDataInt(pCut);
    // compute the functions of the children
    for ( i = 0, pTemp = pIfObj; pTemp; pTemp = pTemp->pEquiv, i++ )
    {
        iFunc0 = If_ManNodeShapeMap2_rec( pIfMan, pTemp->pFanin0, vVisited, vShape );
        if ( iFunc0 == ~0 )
            continue;
        iFunc1 = If_ManNodeShapeMap2_rec( pIfMan, pTemp->pFanin1, vVisited, vShape );
        if ( iFunc1 == ~0 )
            continue;
        iFunc = iFunc0 | iFunc1;
//        if ( If_WordCountOnes(iBest) <= If_WordCountOnes(iFunc) )
        if ( iBest < iFunc )
        {
            iBest = iFunc;
            pTempBest = pTemp;
        }
    }
    if ( pTempBest )
    {
        Vec_IntPush( vShape, pIfObj->Id );
        Vec_IntPush( vShape, pTempBest->Id );
        If_CutSetDataInt( pCut, iBest );
    }
    return If_CutDataInt(pCut);
}
int If_ManNodeShapeMap2( If_Man_t * pIfMan, If_Obj_t * pIfObj, Vec_Int_t * vShape )
{
    If_Cut_t * pCut;
    If_Obj_t * pLeaf;
    int i, iRes;
    // get the best cut
    pCut = If_ObjCutBest(pIfObj);
    assert( pCut->nLeaves > 1 );
    // set the leaf variables
    If_CutForEachLeaf( pIfMan, pCut, pLeaf, i )
        If_CutSetDataInt( If_ObjCutBest(pLeaf), (1 << i) );
    // recursively compute the function while collecting visited cuts
    Vec_IntClear( vShape );
    Vec_PtrClear( pIfMan->vTemp );
    iRes = If_ManNodeShapeMap2_rec( pIfMan, pIfObj, pIfMan->vTemp, vShape ); 
    if ( iRes == ~0 )
    {
        Abc_Print( -1, "If_ManNodeShapeMap2(): Computing local AIG has failed.\n" );
        return 0;
    }
    // clean the cuts
    If_CutForEachLeaf( pIfMan, pCut, pLeaf, i )
        If_CutSetDataInt( If_ObjCutBest(pLeaf), 0 );
    Vec_PtrForEachEntry( If_Cut_t *, pIfMan->vTemp, pCut, i )
        If_CutSetDataInt( pCut, 0 );
    return 1;
}



/**Function*************************************************************

  Synopsis    [Collects logic cone with choices]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int If_ManConeCollect_rec( If_Man_t * pIfMan, If_Obj_t * pIfObj, Vec_Ptr_t * vVisited, Vec_Ptr_t * vCone )
{
    If_Cut_t * pCut;
    If_Obj_t * pTemp;
    int iFunc0, iFunc1;
    int fRootAdded = 0;
    int fNodeAdded = 0;
    // get the best cut
    pCut = If_ObjCutBest(pIfObj);
    // if the cut is visited, return the result
    if ( If_CutDataInt(pCut) )
        return If_CutDataInt(pCut);
    // mark the node as visited
    Vec_PtrPush( vVisited, pCut );
    // insert the worst case
    If_CutSetDataInt( pCut, ~0 );
    // skip in case of primary input
    if ( If_ObjIsCi(pIfObj) )
        return If_CutDataInt(pCut);
    // compute the functions of the children
    for ( pTemp = pIfObj; pTemp; pTemp = pTemp->pEquiv )
    {
        iFunc0 = If_ManConeCollect_rec( pIfMan, pTemp->pFanin0, vVisited, vCone );
        if ( iFunc0 == ~0 )
            continue;
        iFunc1 = If_ManConeCollect_rec( pIfMan, pTemp->pFanin1, vVisited, vCone );
        if ( iFunc1 == ~0 )
            continue;
        fNodeAdded = 1;
        If_CutSetDataInt( pCut, 1 );
        Vec_PtrPush( vCone, pTemp );
        if ( fRootAdded == 0 && pTemp == pIfObj )
            fRootAdded = 1;
    }
    if ( fNodeAdded && !fRootAdded )
        Vec_PtrPush( vCone, pIfObj );
    return If_CutDataInt(pCut);
}
Vec_Ptr_t * If_ManConeCollect( If_Man_t * pIfMan, If_Obj_t * pIfObj, If_Cut_t * pCut )
{
    Vec_Ptr_t * vCone;
    If_Obj_t * pLeaf;
    int i, RetValue;
    // set the leaf variables
    If_CutForEachLeaf( pIfMan, pCut, pLeaf, i )
    {
        assert( If_CutDataInt( If_ObjCutBest(pLeaf) ) == 0 );
        If_CutSetDataInt( If_ObjCutBest(pLeaf), 1 );
    }
    // recursively compute the function while collecting visited cuts
    vCone = Vec_PtrAlloc( 100 );
    Vec_PtrClear( pIfMan->vTemp );
    RetValue = If_ManConeCollect_rec( pIfMan, pIfObj, pIfMan->vTemp, vCone ); 
    assert( RetValue );
    // clean the cuts
    If_CutForEachLeaf( pIfMan, pCut, pLeaf, i )
        If_CutSetDataInt( If_ObjCutBest(pLeaf), 0 );
    Vec_PtrForEachEntry( If_Cut_t *, pIfMan->vTemp, pCut, i )
        If_CutSetDataInt( pCut, 0 );
    return vCone;
}

/**Function*************************************************************

  Synopsis    [Adding clauses.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void sat_solver_add_choice( sat_solver * pSat, int iVar, Vec_Int_t * vVars )
{
    int * pVars = Vec_IntArray(vVars);
    int nVars = Vec_IntSize(vVars);
    int i, k, Lits[2], Value;
    assert( Vec_IntSize(vVars) < Vec_IntCap(vVars) );
    // create literals
    for ( i = 0; i < nVars; i++ )
        pVars[i] = Abc_Var2Lit( pVars[i], 0 );
    pVars[i] = Abc_Var2Lit( iVar, 1 );
    // add clause
    Value = sat_solver_addclause( pSat, pVars, pVars + nVars + 1 );
    assert( Value );
    // undo literals
    for ( i = 0; i < nVars; i++ )
        pVars[i] = Abc_Lit2Var( pVars[i] );
    // add !out => !in
    Lits[0] = Abc_Var2Lit( iVar, 0 );
    for ( i = 0; i < nVars; i++ )
    {
        Lits[1] = Abc_Var2Lit( pVars[i], 1 );
        Value = sat_solver_addclause( pSat, Lits, Lits + 2 );
        assert( Value );
    }
    // add excluvisity
    for ( i = 0; i < nVars; i++ )
    for ( k = i+1; k < nVars; k++ )
    {
        Lits[0] = Abc_Var2Lit( pVars[i], 1 );
        Lits[1] = Abc_Var2Lit( pVars[k], 1 );
        Value = sat_solver_addclause( pSat, Lits, Lits + 2 );
        assert( Value );
    }
}
static inline int  If_ObjSatVar( If_Obj_t * pIfObj )            { return If_CutDataInt(If_ObjCutBest(pIfObj));  }
static inline void If_ObjSetSatVar( If_Obj_t * pIfObj, int v )  { If_CutSetDataInt( If_ObjCutBest(pIfObj), v ); }


/**Function*************************************************************

  Synopsis    [Recursively derives the local AIG for the cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void If_ManNodeShape2_rec( sat_solver * pSat, If_Man_t * pIfMan, If_Obj_t * pIfObj, Vec_Int_t * vShape )
{
    If_Obj_t * pTemp;
    assert( sat_solver_var_value(pSat, If_ObjSatVar(pIfObj)) == 1 );
    if ( pIfObj->fMark )
        return;
    pIfObj->fMark = 1;
    for ( pTemp = pIfObj; pTemp; pTemp = pTemp->pEquiv )
        if ( sat_solver_var_value(pSat, If_ObjSatVar(pTemp)+1) == 1 )
            break;
    assert( pTemp != NULL );
    If_ManNodeShape2_rec( pSat, pIfMan, pTemp->pFanin0, vShape );
    If_ManNodeShape2_rec( pSat, pIfMan, pTemp->pFanin1, vShape );
    Vec_IntPush( vShape, pIfObj->Id );
    Vec_IntPush( vShape, pTemp->Id );
}

/**Function*************************************************************

  Synopsis    [Solve the problem of selecting choices for the given cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int If_ManNodeShapeSat( If_Man_t * pIfMan, If_Obj_t * pIfObj, Vec_Int_t * vShape )
{
    sat_solver * pSat;
    If_Cut_t * pCut;
    Vec_Ptr_t * vCone;
    Vec_Int_t * vFanins;
    If_Obj_t * pObj, * pTemp;
    int i, Lit, Status;

    // get best cut
    pCut = If_ObjCutBest(pIfObj);
    assert( pCut->nLeaves > 1 );

    // collect the cone
    vCone = If_ManConeCollect( pIfMan, pIfObj, pCut );

    // assign SAT variables
    // EXTERNAL variable is even numbered
    // INTERNAL variable is odd numbered
    If_CutForEachLeaf( pIfMan, pCut, pObj, i )
    {
        assert( If_ObjSatVar(pObj) == 0 );
        If_ObjSetSatVar( pObj, 2*(i+1) );
    }
    Vec_PtrForEachEntry( If_Obj_t *, vCone, pObj, i )
    {
        assert( If_ObjSatVar(pObj) == 0 );
        If_ObjSetSatVar( pObj, 2*(i+1+pCut->nLeaves) );
    }

    // start SAT solver
    pSat = sat_solver_new();
    sat_solver_setnvars( pSat, 2 * (pCut->nLeaves + Vec_PtrSize(vCone) + 1) );

    // add constraints
    vFanins = Vec_IntAlloc( 100 );
    Vec_PtrForEachEntry( If_Obj_t *, vCone, pObj, i )
    {
        assert( If_ObjIsAnd(pObj) );
        Vec_IntClear( vFanins );
        for ( pTemp = pObj; pTemp; pTemp = pTemp->pEquiv )
            if ( If_ObjSatVar(pTemp) )
                Vec_IntPush( vFanins, If_ObjSatVar(pTemp)+1 ); // internal
        assert( Vec_IntSize(vFanins) > 0 );
        sat_solver_add_choice( pSat, If_ObjSatVar(pObj), vFanins ); // external
        assert( If_ObjSatVar(pObj) > 0 );
//        sat_solver_add_and( pSat, If_ObjSatVar(pObj)+1, If_ObjSatVar(pObj->pFanin0), If_ObjSatVar(pObj->pFanin1), 0, 0, 0 );
        if ( If_ObjSatVar(pObj->pFanin0) > 0 && If_ObjSatVar(pObj->pFanin1) > 0 )
        {
            int Lits[2];
            Lits[0] = Abc_Var2Lit( If_ObjSatVar(pObj)+1,        1 );
            Lits[1] = Abc_Var2Lit( If_ObjSatVar(pObj->pFanin0), 0 );
            Status = sat_solver_addclause( pSat, Lits, Lits + 2 );
            assert( Status );

            Lits[0] = Abc_Var2Lit( If_ObjSatVar(pObj)+1,        1 );
            Lits[1] = Abc_Var2Lit( If_ObjSatVar(pObj->pFanin1), 0 );
            Status = sat_solver_addclause( pSat, Lits, Lits + 2 );
            assert( Status );
        }
    }
    Vec_IntFree( vFanins );

    // set cut variables to 1
    pCut = If_ObjCutBest(pIfObj);
    If_CutForEachLeaf( pIfMan, pCut, pObj, i )
    {
        Lit = Abc_Var2Lit( If_ObjSatVar(pObj), 0 ); // external
        Status = sat_solver_addclause( pSat, &Lit, &Lit + 1 );
        assert( Status );
    }
    // set output variable to 1
    Lit = Abc_Var2Lit( If_ObjSatVar(pIfObj), 0 ); // external
    Status = sat_solver_addclause( pSat, &Lit, &Lit + 1 );
    assert( Status );

    // solve the problem
    Status = sat_solver_solve( pSat, NULL, NULL, 0, 0, 0, 0 );
    assert( Status == l_True );

    // mark cut nodes
    If_CutForEachLeaf( pIfMan, pCut, pObj, i )
    {
        assert( pObj->fMark == 0 );
        pObj->fMark = 1;
    }

    // select the node's shape
    Vec_IntClear( vShape );
    assert( pIfObj->fMark == 0 );
    If_ManNodeShape2_rec( pSat, pIfMan, pIfObj, vShape ); 

    // cleanup
    sat_solver_delete( pSat );
    If_CutForEachLeaf( pIfMan, pCut, pObj, i )
    {
        If_ObjSetSatVar( pObj, 0 );
        pObj->fMark = 0;
    }
    Vec_PtrForEachEntry( If_Obj_t *, vCone, pObj, i )
    {
        If_ObjSetSatVar( pObj, 0 );
        pObj->fMark = 0;
    }
    Vec_PtrFree( vCone );
    return 1;
}


/**Function*************************************************************

  Synopsis    [Verify that the shape is correct.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int If_ManCheckShape( If_Man_t * pIfMan, If_Obj_t * pIfObj, Vec_Int_t * vShape )
{
    If_Cut_t * pCut;
    If_Obj_t * pLeaf;
    int i, Entry1, Entry2, RetValue = 1;
    // check that the marks are not set
    pCut = If_ObjCutBest(pIfObj);
    If_CutForEachLeaf( pIfMan, pCut, pLeaf, i )
        assert( pLeaf->fMark == 0 );
    // set the marks of the shape
    Vec_IntForEachEntryDouble( vShape, Entry1, Entry2, i )
    {
        pLeaf = If_ManObj(pIfMan, Entry2);
        pLeaf->pFanin0->fMark = 1;
        pLeaf->pFanin1->fMark = 1;
    }
    // check that the leaves are marked
    If_CutForEachLeaf( pIfMan, pCut, pLeaf, i )
        if ( pLeaf->fMark == 0 )
            RetValue = 0;
        else
            pLeaf->fMark = 0;
    // clean the inner marks
    Vec_IntForEachEntryDouble( vShape, Entry1, Entry2, i )
    {
        pLeaf = If_ManObj(pIfMan, Entry2);
        pLeaf->pFanin0->fMark = 0;
        pLeaf->pFanin1->fMark = 0;
    }
    return RetValue;
}

/**Function*************************************************************

  Synopsis    [Recursively compute the set of nodes supported by the cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int If_ManNodeShape( If_Man_t * pIfMan, If_Obj_t * pIfObj, Vec_Int_t * vShape, int fExact )
{
    int RetValue;
//    if ( pIfMan->nChoices == 0 )
    {
        RetValue = If_ManNodeShapeMap( pIfMan, pIfObj, vShape );
        assert( RetValue );
        if ( !fExact || If_ManCheckShape(pIfMan, pIfObj, vShape) )
            return 1;
    }
//    if ( pIfObj->Id == 1254 && If_ObjCutBest(pIfObj)->nLeaves == 7 )
//        If_ObjConePrint( pIfMan, pIfObj );
    RetValue = If_ManNodeShapeMap2( pIfMan, pIfObj, vShape );
    assert( RetValue );
    RetValue = If_ManCheckShape(pIfMan, pIfObj, vShape);
//    assert( RetValue );
//    printf( "%d", RetValue );
    return 1;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

