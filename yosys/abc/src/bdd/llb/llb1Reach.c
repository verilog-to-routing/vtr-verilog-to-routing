/**CFile****************************************************************

  FileName    [llb1Reach.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [BDD based reachability.]

  Synopsis    [Reachability analysis.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: llb1Reach.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "llbInt.h"

ABC_NAMESPACE_IMPL_START

 
////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Derives global BDD for the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Llb_ManConstructOutBdd( Aig_Man_t * pAig, Aig_Obj_t * pNode, DdManager * dd )
{
    DdNode * bBdd0, * bBdd1, * bFunc;
    Vec_Ptr_t * vNodes;
    Aig_Obj_t * pObj = NULL;
    int i;
    abctime TimeStop;
    if ( Aig_ObjFanin0(pNode) == Aig_ManConst1(pAig) )
        return Cudd_NotCond( Cudd_ReadOne(dd), Aig_ObjFaninC0(pNode) );
    TimeStop = dd->TimeStop;  dd->TimeStop = 0;
    vNodes = Aig_ManDfsNodes( pAig, &pNode, 1 );
    assert( Vec_PtrSize(vNodes) > 0 );
    Vec_PtrForEachEntry( Aig_Obj_t *, vNodes, pObj, i )
    {
        if ( !Aig_ObjIsNode(pObj) )
            continue;
        bBdd0 = Cudd_NotCond( Aig_ObjFanin0(pObj)->pData, Aig_ObjFaninC0(pObj) );
        bBdd1 = Cudd_NotCond( Aig_ObjFanin1(pObj)->pData, Aig_ObjFaninC1(pObj) );
        pObj->pData = Cudd_bddAnd( dd, bBdd0, bBdd1 );  Cudd_Ref( (DdNode *)pObj->pData );
    }
    bFunc = (DdNode *)pObj->pData; Cudd_Ref( bFunc );
    Vec_PtrForEachEntry( Aig_Obj_t *, vNodes, pObj, i )
    {
        if ( !Aig_ObjIsNode(pObj) )
            continue;
        Cudd_RecursiveDeref( dd, (DdNode *)pObj->pData );
    }
    Vec_PtrFree( vNodes );
    if ( Aig_ObjIsCo(pNode) )
        bFunc = Cudd_NotCond( bFunc, Aig_ObjFaninC0(pNode) );
    Cudd_Deref( bFunc );
    dd->TimeStop = TimeStop;
    return bFunc;
}

/**Function*************************************************************

  Synopsis    [Derives BDD for the group.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Llb_ManConstructGroupBdd( Llb_Man_t * p, Llb_Grp_t * pGroup )
{
    Aig_Obj_t * pObj;
    DdNode * bBdd0, * bBdd1, * bRes, * bXor, * bTemp;
    int i, k;
    Aig_ManConst1(p->pAig)->pData = Cudd_ReadOne( p->dd );
    Vec_PtrForEachEntry( Aig_Obj_t *, pGroup->vIns, pObj, i )
        pObj->pData = Cudd_bddIthVar( p->dd, Vec_IntEntry(p->vObj2Var, Aig_ObjId(pObj)) );
    Vec_PtrForEachEntry( Aig_Obj_t *, pGroup->vNodes, pObj, i )
    {
        bBdd0 = Cudd_NotCond( Aig_ObjFanin0(pObj)->pData, Aig_ObjFaninC0(pObj) );
        bBdd1 = Cudd_NotCond( Aig_ObjFanin1(pObj)->pData, Aig_ObjFaninC1(pObj) );
//        pObj->pData = Extra_bddAndTime( p->dd, bBdd0, bBdd1, p->pPars->TimeTarget );  
        pObj->pData = Cudd_bddAnd( p->dd, bBdd0, bBdd1 );  
        if ( pObj->pData == NULL )
        {
            Vec_PtrForEachEntryStop( Aig_Obj_t *, pGroup->vNodes, pObj, k, i )
                if ( pObj->pData )
                    Cudd_RecursiveDeref( p->dd, (DdNode *)pObj->pData );
            return NULL;
        }
        Cudd_Ref( (DdNode *)pObj->pData );
    }
    bRes = Cudd_ReadOne( p->dd );   Cudd_Ref( bRes );
    Vec_PtrForEachEntry( Aig_Obj_t *, pGroup->vOuts, pObj, i )
    {
        if ( Aig_ObjIsCo(pObj) )
            bBdd0 = Cudd_NotCond( Aig_ObjFanin0(pObj)->pData, Aig_ObjFaninC0(pObj) );
        else
            bBdd0 = (DdNode *)pObj->pData;
        bBdd1 = Cudd_bddIthVar( p->dd, Vec_IntEntry(p->vObj2Var, Aig_ObjId(pObj)) );
        bXor  = Cudd_bddXor( p->dd, bBdd0, bBdd1 );                  Cudd_Ref( bXor );
//        bRes  = Extra_bddAndTime( p->dd, bTemp = bRes, Cudd_Not(bXor), p->pPars->TimeTarget );  
        bRes  = Cudd_bddAnd( p->dd, bTemp = bRes, Cudd_Not(bXor) );  
        if ( bRes == NULL )
        {
            Cudd_RecursiveDeref( p->dd, bTemp );
            Cudd_RecursiveDeref( p->dd, bXor );
            Vec_PtrForEachEntryStop( Aig_Obj_t *, pGroup->vNodes, pObj, k, i )
                if ( pObj->pData )
                    Cudd_RecursiveDeref( p->dd, (DdNode *)pObj->pData );
            return NULL;
        }        
        Cudd_Ref( bRes );
        Cudd_RecursiveDeref( p->dd, bTemp );
        Cudd_RecursiveDeref( p->dd, bXor );
    }
    Vec_PtrForEachEntry( Aig_Obj_t *, pGroup->vNodes, pObj, i )
        Cudd_RecursiveDeref( p->dd, (DdNode *)pObj->pData );
    Cudd_Deref( bRes );
    return bRes;
}

/**Function*************************************************************

  Synopsis    [Derives quantification cube.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Llb_ManConstructQuantCubeIntern( Llb_Man_t * p, Llb_Grp_t * pGroup, int iGrpPlace, int fBackward )
{
    Aig_Obj_t * pObj;
    DdNode * bRes, * bTemp, * bVar;
    int i, iGroupFirst, iGroupLast;
    abctime TimeStop;
    TimeStop = p->dd->TimeStop; p->dd->TimeStop = 0;
    bRes = Cudd_ReadOne( p->dd );   Cudd_Ref( bRes );
    Vec_PtrForEachEntry( Aig_Obj_t *, pGroup->vIns, pObj, i )
    {
        if ( fBackward && Saig_ObjIsPi(p->pAig, pObj) )
            continue;
        iGroupFirst = Vec_IntEntry(p->vVarBegs, Aig_ObjId(pObj));
        iGroupLast  = Vec_IntEntry(p->vVarEnds, Aig_ObjId(pObj));
        assert( iGroupFirst <= iGroupLast );
        if ( iGroupFirst < iGroupLast )
            continue;
        bVar  = Cudd_bddIthVar( p->dd, Vec_IntEntry(p->vObj2Var, Aig_ObjId(pObj)) );
        bRes  = Cudd_bddAnd( p->dd, bTemp = bRes, bVar );  Cudd_Ref( bRes );
        Cudd_RecursiveDeref( p->dd, bTemp );
    }
    Vec_PtrForEachEntry( Aig_Obj_t *, pGroup->vOuts, pObj, i )
    {
        if ( fBackward && Saig_ObjIsPi(p->pAig, pObj) )
            continue;
        iGroupFirst = Vec_IntEntry(p->vVarBegs, Aig_ObjId(pObj));
        iGroupLast  = Vec_IntEntry(p->vVarEnds, Aig_ObjId(pObj));
        assert( iGroupFirst <= iGroupLast );
        if ( iGroupFirst < iGroupLast )
            continue;
        bVar  = Cudd_bddIthVar( p->dd, Vec_IntEntry(p->vObj2Var, Aig_ObjId(pObj)) );
        bRes  = Cudd_bddAnd( p->dd, bTemp = bRes, bVar );  Cudd_Ref( bRes );
        Cudd_RecursiveDeref( p->dd, bTemp );
    }
    Cudd_Deref( bRes );
    p->dd->TimeStop = TimeStop;
    return bRes;
}

/**Function*************************************************************

  Synopsis    [Derives quantification cube.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Llb_ManConstructQuantCubeFwd( Llb_Man_t * p, Llb_Grp_t * pGroup, int iGrpPlace )
{
    Aig_Obj_t * pObj;
    DdNode * bRes, * bTemp, * bVar;
    int i, iGroupLast;
    abctime TimeStop;
    TimeStop = p->dd->TimeStop; p->dd->TimeStop = 0;
    bRes = Cudd_ReadOne( p->dd );   Cudd_Ref( bRes );
    Vec_PtrForEachEntry( Aig_Obj_t *, pGroup->vIns, pObj, i )
    {
        iGroupLast = Vec_IntEntry(p->vVarEnds, Aig_ObjId(pObj));
        assert( iGroupLast >= iGrpPlace );
        if ( iGroupLast > iGrpPlace )
            continue;
        bVar  = Cudd_bddIthVar( p->dd, Vec_IntEntry(p->vObj2Var, Aig_ObjId(pObj)) );
        bRes  = Cudd_bddAnd( p->dd, bTemp = bRes, bVar );  Cudd_Ref( bRes );
        Cudd_RecursiveDeref( p->dd, bTemp );
    }
    Vec_PtrForEachEntry( Aig_Obj_t *, pGroup->vOuts, pObj, i )
    {
        iGroupLast = Vec_IntEntry(p->vVarEnds, Aig_ObjId(pObj));
        assert( iGroupLast >= iGrpPlace );
        if ( iGroupLast > iGrpPlace )
            continue;
        bVar  = Cudd_bddIthVar( p->dd, Vec_IntEntry(p->vObj2Var, Aig_ObjId(pObj)) );
        bRes  = Cudd_bddAnd( p->dd, bTemp = bRes, bVar );  Cudd_Ref( bRes );
        Cudd_RecursiveDeref( p->dd, bTemp );
    }
    Cudd_Deref( bRes );
    p->dd->TimeStop = TimeStop;
    return bRes;
}

/**Function*************************************************************

  Synopsis    [Derives quantification cube.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Llb_ManConstructQuantCubeBwd( Llb_Man_t * p, Llb_Grp_t * pGroup, int iGrpPlace )
{
    Aig_Obj_t * pObj;
    DdNode * bRes, * bTemp, * bVar;
    int i, iGroupFirst;
    abctime TimeStop;
    TimeStop = p->dd->TimeStop; p->dd->TimeStop = 0;
    bRes = Cudd_ReadOne( p->dd );   Cudd_Ref( bRes );
    Vec_PtrForEachEntry( Aig_Obj_t *, pGroup->vIns, pObj, i )
    {
        if ( Saig_ObjIsPi(p->pAig, pObj) )
            continue;
        iGroupFirst = Vec_IntEntry(p->vVarBegs, Aig_ObjId(pObj));
        assert( iGroupFirst <= iGrpPlace );
        if ( iGroupFirst < iGrpPlace )
            continue;
        bVar  = Cudd_bddIthVar( p->dd, Vec_IntEntry(p->vObj2Var, Aig_ObjId(pObj)) );
        bRes  = Cudd_bddAnd( p->dd, bTemp = bRes, bVar );  Cudd_Ref( bRes );
        Cudd_RecursiveDeref( p->dd, bTemp );
    }
    Vec_PtrForEachEntry( Aig_Obj_t *, pGroup->vOuts, pObj, i )
    {
        if ( Saig_ObjIsPi(p->pAig, pObj) )
            continue;
        iGroupFirst = Vec_IntEntry(p->vVarBegs, Aig_ObjId(pObj));
        assert( iGroupFirst <= iGrpPlace );
        if ( iGroupFirst < iGrpPlace )
            continue;
        bVar  = Cudd_bddIthVar( p->dd, Vec_IntEntry(p->vObj2Var, Aig_ObjId(pObj)) );
        bRes  = Cudd_bddAnd( p->dd, bTemp = bRes, bVar );  Cudd_Ref( bRes );
        Cudd_RecursiveDeref( p->dd, bTemp );
    }
    Cudd_Deref( bRes );
    p->dd->TimeStop = TimeStop;
    return bRes;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Llb_ManComputeInitState( Llb_Man_t * p, DdManager * dd )
{
    Aig_Obj_t * pObj;
    DdNode * bRes, * bVar, * bTemp;
    int i, iVar;
    abctime TimeStop;
    TimeStop = dd->TimeStop; dd->TimeStop = 0;
    bRes = Cudd_ReadOne( dd );   Cudd_Ref( bRes );
    Saig_ManForEachLo( p->pAig, pObj, i )
    {
        iVar  = (dd == p->ddG) ? i : Vec_IntEntry(p->vObj2Var, Aig_ObjId(pObj));
        bVar  = Cudd_bddIthVar( dd, iVar );
        bRes  = Cudd_bddAnd( dd, bTemp = bRes, Cudd_Not(bVar) );  Cudd_Ref( bRes );
        Cudd_RecursiveDeref( dd, bTemp );
    }
    Cudd_Deref( bRes );
    dd->TimeStop = TimeStop;
    return bRes;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Llb_ManComputeImage( Llb_Man_t * p, DdNode * bInit, int fBackward )
{
    int fCheckSupport = 0;
    Llb_Grp_t * pGroup;
    DdNode * bImage, * bGroup, * bCube, * bTemp;
    int k, Index;
    bImage = bInit;  Cudd_Ref( bImage );
    for ( k = 1; k < p->pMatrix->nCols-1; k++ )
    {
        if ( fBackward )
            Index = p->pMatrix->nCols - 1 - k;
        else
            Index = k;

        // compute group BDD
        pGroup = p->pMatrix->pColGrps[Index];
        bGroup = Llb_ManConstructGroupBdd( p, pGroup );
        if ( bGroup == NULL )
        {
            Cudd_RecursiveDeref( p->dd, bImage );
            return NULL;
        }
        Cudd_Ref( bGroup );
        // quantify variables appearing only in this group
        bCube  = Llb_ManConstructQuantCubeIntern( p, pGroup, Index, fBackward );       Cudd_Ref( bCube );
        bGroup = Cudd_bddExistAbstract( p->dd, bTemp = bGroup, bCube );         
        if ( bGroup == NULL )
        {
            Cudd_RecursiveDeref( p->dd, bTemp );
            Cudd_RecursiveDeref( p->dd, bCube );
            return NULL;
        }
        Cudd_Ref( bGroup );
        Cudd_RecursiveDeref( p->dd, bTemp );
        Cudd_RecursiveDeref( p->dd, bCube );
        // perform partial product
        if ( fBackward )
            bCube  = Llb_ManConstructQuantCubeBwd( p, pGroup, Index );
        else
            bCube  = Llb_ManConstructQuantCubeFwd( p, pGroup, Index );
        Cudd_Ref( bCube );
//        bImage = Extra_bddAndAbstractTime( p->dd, bTemp = bImage, bGroup, bCube, p->pPars->TimeTarget );   
        bImage = Cudd_bddAndAbstract( p->dd, bTemp = bImage, bGroup, bCube );   
        if ( bImage == NULL )
        {
            Cudd_RecursiveDeref( p->dd, bTemp );
            Cudd_RecursiveDeref( p->dd, bGroup );
            Cudd_RecursiveDeref( p->dd, bCube );
            return NULL;
        }
        Cudd_Ref( bImage );
        Cudd_RecursiveDeref( p->dd, bTemp );
        Cudd_RecursiveDeref( p->dd, bGroup );
        Cudd_RecursiveDeref( p->dd, bCube );
    }

    // make sure image depends on next state vars
    if ( fCheckSupport )
    {
        bCube = Cudd_Support( p->dd, bImage );    Cudd_Ref( bCube );
        for ( bTemp = bCube; bTemp != p->dd->one; bTemp = cuddT(bTemp) )
        {
            int ObjId = Vec_IntEntry( p->vVar2Obj, bTemp->index );
            Aig_Obj_t * pObj = Aig_ManObj( p->pAig, ObjId );
            if ( !Saig_ObjIsLi(p->pAig, pObj) )
                printf( "Var %d assigned to obj %d that is not LI\n", bTemp->index, ObjId );
        }
        Cudd_RecursiveDeref( p->dd, bCube );
    }
    Cudd_Deref( bImage );
    return bImage;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Llb_ManCreateConstraints( Llb_Man_t * p, Vec_Int_t * vHints, int fUseNsVars )
{
    DdNode * bConstr, * bFunc, * bTemp;
    Aig_Obj_t * pObj;
    int i, Entry;
    abctime TimeStop;
    if ( vHints == NULL )
        return Cudd_ReadOne( p->dd );
    TimeStop = p->dd->TimeStop; p->dd->TimeStop = 0;
    assert( Aig_ManCiNum(p->pAig) == Aig_ManCiNum(p->pAigGlo) );
    // assign const and PI nodes to the original AIG
    Aig_ManCleanData( p->pAig );
    Aig_ManConst1( p->pAig )->pData = Cudd_ReadOne( p->dd );
    Saig_ManForEachPi( p->pAig, pObj, i )
        pObj->pData = Cudd_bddIthVar( p->dd, Vec_IntEntry(p->vObj2Var,Aig_ObjId(pObj)) );
    Saig_ManForEachLo( p->pAig, pObj, i )
    {
        if ( fUseNsVars )
            Entry = Vec_IntEntry( p->vObj2Var, Aig_ObjId(Saig_ObjLoToLi(p->pAig, pObj)) );
        else
            Entry = Vec_IntEntry( p->vObj2Var, Aig_ObjId(pObj) );
        pObj->pData = Cudd_bddIthVar( p->dd, Entry );
    }
    // transfer them to the global AIG
    Aig_ManCleanData( p->pAigGlo );
    Aig_ManConst1( p->pAigGlo )->pData = Cudd_ReadOne( p->dd );
    Aig_ManForEachCi( p->pAigGlo, pObj, i )
        pObj->pData = Aig_ManCi(p->pAig, i)->pData;
    // derive consraints
    bConstr = Cudd_ReadOne( p->dd );   Cudd_Ref( bConstr );
    Vec_IntForEachEntry( vHints, Entry, i )
    {
        if ( Entry != 0 && Entry != 1 )
            continue;
        bFunc = Llb_ManConstructOutBdd( p->pAigGlo, Aig_ManObj(p->pAigGlo, i), p->dd );  Cudd_Ref( bFunc );
        bFunc = Cudd_NotCond( bFunc, Entry ); // restrict to not constraint
        // make the product
        bConstr = Cudd_bddAnd( p->dd, bTemp = bConstr, bFunc );                          Cudd_Ref( bConstr );
        Cudd_RecursiveDeref( p->dd, bTemp );
        Cudd_RecursiveDeref( p->dd, bFunc );
    }
    Cudd_Deref( bConstr );
    p->dd->TimeStop = TimeStop;
    return bConstr;
}

/**Function*************************************************************

  Synopsis    [Perform reachability with hints and returns reached states in ppGlo.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Cex_t * Llb_ManReachDeriveCex( Llb_Man_t * p )
{
    Abc_Cex_t * pCex;
    Aig_Obj_t * pObj;
    DdNode * bState = NULL, * bImage, * bOneCube, * bTemp, * bRing;
    int i, v, RetValue, nPiOffset;
    char * pValues = ABC_ALLOC( char, Cudd_ReadSize(p->ddR) );
    assert( Vec_PtrSize(p->vRings) > 0 );

    p->dd->TimeStop  = 0;
    p->ddR->TimeStop = 0;

/*
    Saig_ManForEachLo( p->pAig, pObj, i )
        printf( "%d ", pObj->Id );
    printf( "\n" );
    Saig_ManForEachLi( p->pAig, pObj, i )
        printf( "%d(%d) ", pObj->Id, Aig_ObjFaninId0(pObj) );
    printf( "\n" );
*/
    // allocate room for the counter-example
    pCex = Abc_CexAlloc( Saig_ManRegNum(p->pAig), Saig_ManPiNum(p->pAig), Vec_PtrSize(p->vRings) );
    pCex->iFrame = Vec_PtrSize(p->vRings) - 1;
    pCex->iPo = -1;

    // get the last cube
    bOneCube = Cudd_bddIntersect( p->ddR, (DdNode *)Vec_PtrEntryLast(p->vRings), p->ddR->bFunc );  Cudd_Ref( bOneCube );
    RetValue = Cudd_bddPickOneCube( p->ddR, bOneCube, pValues );
    Cudd_RecursiveDeref( p->ddR, bOneCube );
    assert( RetValue );

    // write PIs of counter-example
    nPiOffset = Saig_ManRegNum(p->pAig) + Saig_ManPiNum(p->pAig) * (Vec_PtrSize(p->vRings) - 1);
    Saig_ManForEachPi( p->pAig, pObj, i )
        if ( pValues[Saig_ManRegNum(p->pAig)+i] == 1 )
            Abc_InfoSetBit( pCex->pData, nPiOffset + i );

    // write state in terms of NS variables
    if ( Vec_PtrSize(p->vRings) > 1 )
    {
        bState = Llb_CoreComputeCube( p->dd, p->vGlo2Ns, 1, pValues );   Cudd_Ref( bState );
    }
    // perform backward analysis
    Vec_PtrForEachEntryReverse( DdNode *, p->vRings, bRing, v )
    { 
        if ( v == Vec_PtrSize(p->vRings) - 1 )
            continue;
//Extra_bddPrintSupport( p->dd, bState );  printf( "\n" );
//Extra_bddPrintSupport( p->dd, bRing );   printf( "\n" );
        // compute the next states
        bImage = Llb_ManComputeImage( p, bState, 1 ); 
        assert( bImage != NULL );
        Cudd_Ref( bImage );
        Cudd_RecursiveDeref( p->dd, bState );
//Extra_bddPrintSupport( p->dd, bImage );  printf( "\n" );

        // move reached states into ring manager
        bImage = Extra_TransferPermute( p->dd, p->ddR, bTemp = bImage, Vec_IntArray(p->vCs2Glo) );    Cudd_Ref( bImage );
        Cudd_RecursiveDeref( p->dd, bTemp );
//Extra_bddPrintSupport( p->ddR, bImage );  printf( "\n" );

        // intersect with the previous set
        bOneCube = Cudd_bddIntersect( p->ddR, bImage, bRing );                Cudd_Ref( bOneCube );
        Cudd_RecursiveDeref( p->ddR, bImage );

        // find any assignment of the BDD
        RetValue = Cudd_bddPickOneCube( p->ddR, bOneCube, pValues );
        Cudd_RecursiveDeref( p->ddR, bOneCube );
        assert( RetValue );
/*
        for ( i = 0; i < p->ddR->size; i++ )
            printf( "%d ", pValues[i] );
        printf( "\n" );
*/
        // write PIs of counter-example
        nPiOffset -= Saig_ManPiNum(p->pAig);
        Saig_ManForEachPi( p->pAig, pObj, i )
            if ( pValues[Saig_ManRegNum(p->pAig)+i] == 1 )
                Abc_InfoSetBit( pCex->pData, nPiOffset + i );

        // check that we get the init state
        if ( v == 0 )
        {
            Saig_ManForEachLo( p->pAig, pObj, i )
                assert( pValues[i] == 0 );
            break;
        }

        // write state in terms of NS variables
        bState = Llb_CoreComputeCube( p->dd, p->vGlo2Ns, 1, pValues );   Cudd_Ref( bState );
    }
    assert( nPiOffset == Saig_ManRegNum(p->pAig) );
    // update the output number
//Abc_CexPrint( pCex );
    RetValue = Saig_ManFindFailedPoCex( p->pAigGlo, pCex );
    assert( RetValue >= 0 && RetValue < Saig_ManPoNum(p->pAigGlo) ); // invalid CEX!!!
    pCex->iPo = RetValue;
    // cleanup
    ABC_FREE( pValues );
    return pCex;
}

/**Function*************************************************************

  Synopsis    [Perform reachability with hints and returns reached states in ppGlo.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Llb_ManReachability( Llb_Man_t * p, Vec_Int_t * vHints, DdManager ** pddGlo )
{
    int * pNs2Glo = Vec_IntArray( p->vNs2Glo );
    int * pCs2Glo = Vec_IntArray( p->vCs2Glo );
    int * pGlo2Cs = Vec_IntArray( p->vGlo2Cs );
    DdNode * bCurrent, * bReached, * bNext, * bTemp, * bCube;
    DdNode * bConstrCs, * bConstrNs;
    abctime clk2, clk = Abc_Clock();
    int nIters, nBddSize = 0;
//    int nThreshold = 10000;

    // compute time to stop
    p->pPars->TimeTarget = p->pPars->TimeLimit ? p->pPars->TimeLimit * CLOCKS_PER_SEC + Abc_Clock(): 0;

    // define variable limits
    Llb_ManPrepareVarLimits( p );

    // start the managers
    assert( p->dd == NULL );
    assert( p->ddG == NULL );
    assert( p->ddR == NULL );
    p->dd  = Cudd_Init( Vec_IntSize(p->vVar2Obj), 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0 );
    p->ddR = Cudd_Init( Aig_ManCiNum(p->pAig),    0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0 );
    if ( pddGlo && *pddGlo )
        p->ddG = *pddGlo, *pddGlo = NULL; 
    else
        p->ddG = Cudd_Init( Aig_ManRegNum(p->pAig),   0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0 );

    if ( p->pPars->fReorder )
    {
        Cudd_AutodynEnable( p->dd,  CUDD_REORDER_SYMM_SIFT );
        Cudd_AutodynEnable( p->ddG, CUDD_REORDER_SYMM_SIFT );
        Cudd_AutodynEnable( p->ddR, CUDD_REORDER_SYMM_SIFT );
    }
    else
    {
        Cudd_AutodynDisable( p->dd );
        Cudd_AutodynDisable( p->ddG );
        Cudd_AutodynDisable( p->ddR );
    }

    // set the stop time parameter
    p->dd->TimeStop  = p->pPars->TimeTarget;
    p->ddG->TimeStop = p->pPars->TimeTarget;
    p->ddR->TimeStop = p->pPars->TimeTarget;

    // create bad state in the ring manager
    p->ddR->bFunc  = Llb_BddComputeBad( p->pAigGlo, p->ddR, p->pPars->TimeTarget );          
    if ( p->ddR->bFunc == NULL )
    {
        if ( !p->pPars->fSilent )
            printf( "Reached timeout (%d seconds) during constructing the bad states.\n", p->pPars->TimeLimit );
        p->pPars->iFrame = -1;
        return -1;
    }
    Cudd_Ref( p->ddR->bFunc );

    // derive constraints
    bConstrCs = Llb_ManCreateConstraints( p, vHints, 0 );   Cudd_Ref( bConstrCs );
    bConstrNs = Llb_ManCreateConstraints( p, vHints, 1 );   Cudd_Ref( bConstrNs );
//Extra_bddPrint( p->dd, bConstrCs ); printf( "\n" );
//Extra_bddPrint( p->dd, bConstrNs ); printf( "\n" );

    // perform reachability analysis
    // compute the starting set of states
    if ( p->ddG->bFunc )
    {
        bReached = p->ddG->bFunc; p->ddG->bFunc = NULL;
        bCurrent = Extra_TransferPermute( p->ddG, p->dd, bReached, pGlo2Cs );    Cudd_Ref( bCurrent );
    }
    else
    {
        bReached = Llb_ManComputeInitState( p, p->ddG );                         Cudd_Ref( bReached );
        bCurrent = Llb_ManComputeInitState( p, p->dd  );                         Cudd_Ref( bCurrent );
    }
//Extra_bddPrintSupport( p->ddG, bReached ); printf( "\n" );
//Extra_bddPrintSupport( p->dd,  bCurrent ); printf( "\n" );

//Extra_bddPrintSupport( p->dd, bCurrent ); printf( "\n" );
    for ( nIters = 0; nIters < p->pPars->nIterMax; nIters++ )
    { 
        clk2 = Abc_Clock();
        // check the runtime limit
        if ( p->pPars->TimeLimit && Abc_Clock() > p->pPars->TimeTarget )
        {
            if ( !p->pPars->fSilent )
                printf( "Reached timeout during image computation (%d seconds).\n",  p->pPars->TimeLimit );
            p->pPars->iFrame = nIters - 1;
            Cudd_RecursiveDeref( p->dd,  bCurrent );  bCurrent = NULL;
            Cudd_RecursiveDeref( p->dd,  bConstrCs ); bConstrCs = NULL;
            Cudd_RecursiveDeref( p->dd,  bConstrNs ); bConstrNs = NULL;
            Cudd_RecursiveDeref( p->ddG, bReached );  bReached = NULL;
            return -1;
        }

        // save the onion ring
        bTemp = Extra_TransferPermute( p->dd, p->ddR, bCurrent, pCs2Glo );
        if ( bTemp == NULL )
        {
            if ( !p->pPars->fSilent )
                printf( "Reached timeout (%d seconds) during ring transfer.\n",  p->pPars->TimeLimit );
            p->pPars->iFrame = nIters - 1;
            Cudd_RecursiveDeref( p->dd,  bCurrent );  bCurrent = NULL;
            Cudd_RecursiveDeref( p->dd,  bConstrCs ); bConstrCs = NULL;
            Cudd_RecursiveDeref( p->dd,  bConstrNs ); bConstrNs = NULL;
            Cudd_RecursiveDeref( p->ddG, bReached );  bReached = NULL;
            return -1;
        }
        Cudd_Ref( bTemp );
        Vec_PtrPush( p->vRings, bTemp );

        // check it for bad states
        if ( !p->pPars->fSkipOutCheck && !Cudd_bddLeq( p->ddR, bTemp, Cudd_Not(p->ddR->bFunc) ) ) 
        {
            assert( p->pAigGlo->pSeqModel == NULL );
            if ( !p->pPars->fBackward )
                p->pAigGlo->pSeqModel = Llb_ManReachDeriveCex( p ); 
            if ( !p->pPars->fSilent )
            {
                if ( !p->pPars->fBackward )
                    Abc_Print( 1, "Output %d of miter \"%s\" was asserted in frame %d.  ", p->pAigGlo->pSeqModel->iPo, p->pAigGlo->pName, p->pAigGlo->pName, nIters );
                else
                    Abc_Print( 1, "Output ??? of miter \"%s\" was asserted in frame %d (counter-example is not produced).  ", p->pAigGlo->pName, nIters );
                Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
            }
            p->pPars->iFrame = nIters - 1;
            Cudd_RecursiveDeref( p->dd,  bCurrent );  bCurrent = NULL;
            Cudd_RecursiveDeref( p->dd,  bConstrCs ); bConstrCs = NULL;
            Cudd_RecursiveDeref( p->dd,  bConstrNs ); bConstrNs = NULL;
            Cudd_RecursiveDeref( p->ddG, bReached );  bReached = NULL;
            return 0; 
        }

        // restrict reachable states using constraints
        if ( vHints )
        {
            bCurrent = Cudd_bddAnd( p->dd, bTemp = bCurrent, bConstrCs );                                Cudd_Ref( bCurrent );
            Cudd_RecursiveDeref( p->dd, bTemp );
        }

        // quantify variables appearing only in the init state
        bCube    = Llb_ManConstructQuantCubeIntern( p, (Llb_Grp_t *)Vec_PtrEntry(p->vGroups,0), 0, 0 );  Cudd_Ref( bCube );
        bCurrent = Cudd_bddExistAbstract( p->dd, bTemp = bCurrent, bCube );                              Cudd_Ref( bCurrent );
        Cudd_RecursiveDeref( p->dd, bTemp );
        Cudd_RecursiveDeref( p->dd, bCube );

        // compute the next states
        bNext = Llb_ManComputeImage( p, bCurrent, 0 );
        if ( bNext == NULL )
        {
            if ( !p->pPars->fSilent )
                printf( "Reached timeout (%d seconds) during image computation.\n",  p->pPars->TimeLimit );
            p->pPars->iFrame = nIters - 1;
            Cudd_RecursiveDeref( p->dd,  bCurrent );   bCurrent = NULL;
            Cudd_RecursiveDeref( p->dd,  bConstrCs );  bConstrCs = NULL;
            Cudd_RecursiveDeref( p->dd,  bConstrNs );  bConstrNs = NULL;
            Cudd_RecursiveDeref( p->ddG, bReached );   bReached = NULL;
            return -1;
        }
        Cudd_Ref( bNext );
        Cudd_RecursiveDeref( p->dd, bCurrent ); bCurrent = NULL;

        // restrict reachable states using constraints
        if ( vHints )
        {
            bNext = Cudd_bddAnd( p->dd, bTemp = bNext, bConstrNs );                Cudd_Ref( bNext );
            Cudd_RecursiveDeref( p->dd, bTemp );
        }
//Extra_bddPrintSupport( p->dd, bNext ); printf( "\n" );

        // remap these states into the current state vars
//        bNext = Extra_TransferPermute( p->dd, p->ddG, bTemp = bNext, pNs2Glo );    Cudd_Ref( bNext );
//        Cudd_RecursiveDeref( p->dd, bTemp );
//        bNext = Extra_TransferPermuteTime( p->dd, p->ddG, bTemp = bNext, pNs2Glo, p->pPars->TimeTarget );    
        bNext = Extra_TransferPermute( p->dd, p->ddG, bTemp = bNext, pNs2Glo );    
        if ( bNext == NULL )
        {
            if ( !p->pPars->fSilent )
                printf( "Reached timeout (%d seconds) during image computation in transfer 1.\n",  p->pPars->TimeLimit );
            p->pPars->iFrame = nIters - 1;
            Cudd_RecursiveDeref( p->dd,  bTemp );  
            Cudd_RecursiveDeref( p->dd,  bConstrCs );  bConstrCs = NULL;
            Cudd_RecursiveDeref( p->dd,  bConstrNs );  bConstrNs = NULL;
            Cudd_RecursiveDeref( p->ddG, bReached );   bReached = NULL;
            return -1;
        }
        Cudd_Ref( bNext );
        Cudd_RecursiveDeref( p->dd, bTemp );  


        // check if there are any new states
        if ( Cudd_bddLeq( p->ddG, bNext, bReached ) ) // implication = no new states
        {
            Cudd_RecursiveDeref( p->ddG,  bNext );     bNext = NULL;
            break;
        }

        // check the BDD size
        nBddSize = Cudd_DagSize(bNext);
        if ( nBddSize > p->pPars->nBddMax )
        {
            Cudd_RecursiveDeref( p->ddG,  bNext );     bNext = NULL;
            break;
        }

        // get the new states
        bCurrent = Cudd_bddAnd( p->ddG, bNext, Cudd_Not(bReached) );
        if ( bCurrent == NULL )
        {
            Cudd_RecursiveDeref( p->ddG,  bNext );        bNext = NULL;
            Cudd_RecursiveDeref( p->ddG,  bReached );     bReached = NULL;
            break;
        }
        Cudd_Ref( bCurrent );
        // minimize the new states with the reached states
//        bCurrent = Cudd_bddConstrain( p->ddG, bTemp = bCurrent, Cudd_Not(bReached) ); Cudd_Ref( bCurrent );
//        bCurrent = Cudd_bddRestrict( p->ddG, bTemp = bCurrent, Cudd_Not(bReached) );  Cudd_Ref( bCurrent );
//        Cudd_RecursiveDeref( p->ddG, bTemp );
//printf( "Initial BDD =%7d. Constrained BDD =%7d.\n", Cudd_DagSize(bTemp), Cudd_DagSize(bCurrent) );

        // remap these states into the current state vars
//        bCurrent = Extra_TransferPermute( p->ddG, p->dd, bTemp = bCurrent, pGlo2Cs );   Cudd_Ref( bCurrent );
//        Cudd_RecursiveDeref( p->ddG, bTemp );
//        bCurrent = Extra_TransferPermuteTime( p->ddG, p->dd, bTemp = bCurrent, pGlo2Cs, p->pPars->TimeTarget );    
        bCurrent = Extra_TransferPermute( p->ddG, p->dd, bTemp = bCurrent, pGlo2Cs );    
        if ( bCurrent == NULL )
        {
            if ( !p->pPars->fSilent )
                printf( "Reached timeout (%d seconds) during image computation in transfer 2.\n",  p->pPars->TimeLimit );
            p->pPars->iFrame = nIters - 1;
            Cudd_RecursiveDeref( p->ddG, bTemp );  
            Cudd_RecursiveDeref( p->dd,  bConstrCs );  bConstrCs = NULL;
            Cudd_RecursiveDeref( p->dd,  bConstrNs );  bConstrNs = NULL;
            Cudd_RecursiveDeref( p->ddG, bReached );   bReached = NULL;
            return -1;
        }
        Cudd_Ref( bCurrent );
        Cudd_RecursiveDeref( p->ddG, bTemp ); 
        

        // add to the reached states
        bReached = Cudd_bddOr( p->ddG, bTemp = bReached, bNext );                       
        if ( bReached == NULL )
        {
            Cudd_RecursiveDeref( p->ddG,  bTemp );     bTemp = NULL;
            Cudd_RecursiveDeref( p->ddG,  bNext );     bNext = NULL;
            break;
        }
        Cudd_Ref( bReached );
        Cudd_RecursiveDeref( p->ddG, bTemp );
        Cudd_RecursiveDeref( p->ddG, bNext );
        bNext = NULL;

        if ( p->pPars->fVerbose )
        {
            fprintf( stdout, "F =%5d : ",    nIters );
            fprintf( stdout, "Im =%6d  ",    nBddSize );
            fprintf( stdout, "(%4d %3d)   ", Cudd_ReadReorderings(p->dd),  Cudd_ReadGarbageCollections(p->dd) );
            fprintf( stdout, "Rea =%6d  ",   Cudd_DagSize(bReached) );
            fprintf( stdout, "(%4d%4d)   ",  Cudd_ReadReorderings(p->ddG), Cudd_ReadGarbageCollections(p->ddG) );
            Abc_PrintTime( 1, "Time", Abc_Clock() - clk2 );
        }
/*
        if ( p->pPars->fVerbose )
        {
            double nMints = Cudd_CountMinterm(p->ddG, bReached, Saig_ManRegNum(p->pAig) );
//            Extra_bddPrint( p->ddG, bReached );printf( "\n" );
            fprintf( stdout, "Reachable states = %.0f. (Ratio = %.4f %%)\n", nMints, 100.0*nMints/pow(2.0, Saig_ManRegNum(p->pAig)) );
            fflush( stdout ); 
        }
*/
    }
    Cudd_RecursiveDeref( p->dd, bConstrCs ); bConstrCs = NULL;
    Cudd_RecursiveDeref( p->dd, bConstrNs ); bConstrNs = NULL;
    if ( bReached == NULL )
    {
        p->pPars->iFrame = nIters - 1;
        return 0; // reachable
    }
//    assert( bCurrent == NULL );
    if ( bCurrent )
        Cudd_RecursiveDeref( p->dd, bCurrent );
    // report the stats
    if ( p->pPars->fVerbose )
    {
        double nMints = Cudd_CountMinterm(p->ddG, bReached, Saig_ManRegNum(p->pAig) );
        if ( nIters >= p->pPars->nIterMax || nBddSize > p->pPars->nBddMax )
            fprintf( stdout, "Reachability analysis is stopped after %d frames.\n", nIters );
        else
            fprintf( stdout, "Reachability analysis completed after %d frames.\n", nIters );
        fprintf( stdout, "Reachable states = %.0f. (Ratio = %.4f %%)\n", nMints, 100.0*nMints/pow(2.0, Saig_ManRegNum(p->pAig)) );
        fflush( stdout ); 
    }
    if ( nIters >= p->pPars->nIterMax || nBddSize > p->pPars->nBddMax )
    {
        if ( !p->pPars->fSilent )
            printf( "Verified only for states reachable in %d frames.  ", nIters );
        p->pPars->iFrame = p->pPars->nIterMax;
        Cudd_RecursiveDeref( p->ddG, bReached );
        return -1; // undecided
    }
    if ( pddGlo ) 
    {
        assert( p->ddG->bFunc == NULL );
        p->ddG->bFunc = bReached; bReached = NULL;
        assert( *pddGlo == NULL );
        *pddGlo = p->ddG;  p->ddG = NULL;
    }
    else
        Cudd_RecursiveDeref( p->ddG, bReached );
    if ( !p->pPars->fSilent )
        printf( "The miter is proved unreachable after %d iterations.  ", nIters );
    p->pPars->iFrame = nIters - 1;
    return 1; // unreachable
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

