/**CFile****************************************************************

  FileName    [mfsStrash.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [The good old minimization with complete don't-cares.]

  Synopsis    [Structural hashing of the window with ODCs.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: mfsStrash.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "mfsInt.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Recursively converts AIG from Aig_Man_t into Hop_Obj_t.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_MfsConvertAigToHop_rec( Aig_Obj_t * pObj, Hop_Man_t * pHop )
{
    assert( !Aig_IsComplement(pObj) );
    if ( pObj->pData )
        return;
    Abc_MfsConvertAigToHop_rec( Aig_ObjFanin0(pObj), pHop ); 
    Abc_MfsConvertAigToHop_rec( Aig_ObjFanin1(pObj), pHop );
    pObj->pData = Hop_And( pHop, (Hop_Obj_t *)Aig_ObjChild0Copy(pObj), (Hop_Obj_t *)Aig_ObjChild1Copy(pObj) ); 
    assert( !Hop_IsComplement((Hop_Obj_t *)pObj->pData) );
}

/**Function*************************************************************

  Synopsis    [Converts AIG from Aig_Man_t into Hop_Obj_t.]

  Description [Assumes that Aig_Man_t has exactly one primary outputs.
  Returns the pointer to the root node (Hop_Obj_t) in Hop_Man_t.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Hop_Obj_t * Abc_MfsConvertAigToHop( Aig_Man_t * pMan, Hop_Man_t * pHop )
{
    Aig_Obj_t * pRoot, * pObj;
    int i;
    assert( Aig_ManCoNum(pMan) == 1 );
    pRoot = Aig_ManCo( pMan, 0 );
    // check the case of a constant
    if ( Aig_ObjIsConst1( Aig_ObjFanin0(pRoot) ) )
        return Hop_NotCond( Hop_ManConst1(pHop), Aig_ObjFaninC0(pRoot) );
    // set the PI mapping
    Aig_ManCleanData( pMan );
    Aig_ManForEachCi( pMan, pObj, i )
        pObj->pData = Hop_IthVar( pHop, i );
    // construct the AIG
    Abc_MfsConvertAigToHop_rec( Aig_ObjFanin0(pRoot), pHop );
    return Hop_NotCond( (Hop_Obj_t *)Aig_ObjFanin0(pRoot)->pData, Aig_ObjFaninC0(pRoot) );
}

// should be called as follows:   pNodeNew->pData = Abc_MfsConvertAigToHop( pAigManInterpol, pNodeNew->pNtk->pManFunc );

/**Function*************************************************************

  Synopsis    [Construct BDDs and mark AIG nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_MfsConvertHopToAig_rec( Hop_Obj_t * pObj, Aig_Man_t * pMan )
{
    assert( !Hop_IsComplement(pObj) );
    if ( !Hop_ObjIsNode(pObj) || Hop_ObjIsMarkA(pObj) )
        return;
    Abc_MfsConvertHopToAig_rec( Hop_ObjFanin0(pObj), pMan ); 
    Abc_MfsConvertHopToAig_rec( Hop_ObjFanin1(pObj), pMan );
    pObj->pData = Aig_And( pMan, (Aig_Obj_t *)Hop_ObjChild0Copy(pObj), (Aig_Obj_t *)Hop_ObjChild1Copy(pObj) ); 
    assert( !Hop_ObjIsMarkA(pObj) ); // loop detection
    Hop_ObjSetMarkA( pObj );
}

/**Function*************************************************************

  Synopsis    [Converts the network from AIG to BDD representation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_MfsConvertHopToAig( Abc_Obj_t * pObjOld, Aig_Man_t * pMan )
{
    Hop_Man_t * pHopMan;
    Hop_Obj_t * pRoot;
    Abc_Obj_t * pFanin;
    int i;
    // get the local AIG
    pHopMan = (Hop_Man_t *)pObjOld->pNtk->pManFunc;
    pRoot = (Hop_Obj_t *)pObjOld->pData;
    // check the case of a constant
    if ( Hop_ObjIsConst1( Hop_Regular(pRoot) ) )
    {
        pObjOld->pCopy = (Abc_Obj_t *)Aig_NotCond( Aig_ManConst1(pMan), Hop_IsComplement(pRoot) );
        pObjOld->pNext = pObjOld->pCopy;
        return;
    }

    // assign the fanin nodes
    Abc_ObjForEachFanin( pObjOld, pFanin, i )
        Hop_ManPi(pHopMan, i)->pData = pFanin->pCopy;
    // construct the AIG
    Abc_MfsConvertHopToAig_rec( Hop_Regular(pRoot), pMan );
    pObjOld->pCopy = (Abc_Obj_t *)Aig_NotCond( (Aig_Obj_t *)Hop_Regular(pRoot)->pData, Hop_IsComplement(pRoot) );  
    Hop_ConeUnmark_rec( Hop_Regular(pRoot) );

    // assign the fanin nodes
    Abc_ObjForEachFanin( pObjOld, pFanin, i )
        Hop_ManPi(pHopMan, i)->pData = pFanin->pNext;
    // construct the AIG
    Abc_MfsConvertHopToAig_rec( Hop_Regular(pRoot), pMan );
    pObjOld->pNext = (Abc_Obj_t *)Aig_NotCond( (Aig_Obj_t *)Hop_Regular(pRoot)->pData, Hop_IsComplement(pRoot) );  
    Hop_ConeUnmark_rec( Hop_Regular(pRoot) );
}

/**Function*************************************************************

  Synopsis    [Computes the care set of the node under ODCs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Obj_t * Abc_NtkConstructAig_rec( Mfs_Man_t * p, Abc_Obj_t * pNode, Aig_Man_t * pMan )
{
    Aig_Obj_t * pRoot, * pExor;
    Abc_Obj_t * pObj;
    int i;
    // assign AIG nodes to the leaves
    Vec_PtrForEachEntry( Abc_Obj_t *, p->vSupp, pObj, i )
        pObj->pCopy = pObj->pNext = (Abc_Obj_t *)Aig_ObjCreateCi( pMan );
    // strash intermediate nodes
    Abc_NtkIncrementTravId( pNode->pNtk );
    Vec_PtrForEachEntry( Abc_Obj_t *, p->vNodes, pObj, i )
    {
        Abc_MfsConvertHopToAig( pObj, pMan );
        if ( pObj == pNode )
            pObj->pNext = Abc_ObjNot(pObj->pNext);
    }
    // create the observability condition
    pRoot = Aig_ManConst0(pMan);
    Vec_PtrForEachEntry( Abc_Obj_t *, p->vRoots, pObj, i )
    {
        pExor = Aig_Exor( pMan, (Aig_Obj_t *)pObj->pCopy, (Aig_Obj_t *)pObj->pNext );
        pRoot = Aig_Or( pMan, pRoot, pExor );
    }
    return pRoot;
}

/**Function*************************************************************

  Synopsis    [Adds relevant constraints.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Obj_t * Abc_NtkConstructCare_rec( Aig_Man_t * pCare, Aig_Obj_t * pObj, Aig_Man_t * pMan )
{
    Aig_Obj_t * pObj0, * pObj1;
    if ( Aig_ObjIsTravIdCurrent( pCare, pObj ) )
        return (Aig_Obj_t *)pObj->pData;
    Aig_ObjSetTravIdCurrent( pCare, pObj );
    if ( Aig_ObjIsCi(pObj) )
        return (Aig_Obj_t *)(pObj->pData = NULL);
    pObj0 = Abc_NtkConstructCare_rec( pCare, Aig_ObjFanin0(pObj), pMan );
    if ( pObj0 == NULL )
        return (Aig_Obj_t *)(pObj->pData = NULL);
    pObj1 = Abc_NtkConstructCare_rec( pCare, Aig_ObjFanin1(pObj), pMan );
    if ( pObj1 == NULL )
        return (Aig_Obj_t *)(pObj->pData = NULL);
    pObj0 = Aig_NotCond( pObj0, Aig_ObjFaninC0(pObj) );
    pObj1 = Aig_NotCond( pObj1, Aig_ObjFaninC1(pObj) );
    return (Aig_Obj_t *)(pObj->pData = Aig_And( pMan, pObj0, pObj1 ));
}

/**Function*************************************************************

  Synopsis    [Creates AIG for the window with constraints.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Abc_NtkConstructAig( Mfs_Man_t * p, Abc_Obj_t * pNode )
{
    Aig_Man_t * pMan;
    Abc_Obj_t * pFanin;
    Aig_Obj_t * pObjAig, * pPi, * pPo;
    Vec_Int_t * vOuts;
    int i, k, iOut;
    // start the new manager
    pMan = Aig_ManStart( 1000 );
    // construct the root node's AIG cone
    pObjAig = Abc_NtkConstructAig_rec( p, pNode, pMan );
//    assert( Aig_ManConst1(pMan) == pObjAig );
    Aig_ObjCreateCo( pMan, pObjAig );
    if ( p->pCare )
    {
        // mark the care set
        Aig_ManIncrementTravId( p->pCare );
        Vec_PtrForEachEntry( Abc_Obj_t *, p->vSupp, pFanin, i )
        {
            pPi = Aig_ManCi( p->pCare, (int)(ABC_PTRUINT_T)pFanin->pData );
            Aig_ObjSetTravIdCurrent( p->pCare, pPi );
            pPi->pData = pFanin->pCopy;
        }
        // construct the constraints
        Vec_PtrForEachEntry( Abc_Obj_t *, p->vSupp, pFanin, i )
        {
            vOuts = (Vec_Int_t *)Vec_PtrEntry( p->vSuppsInv, (int)(ABC_PTRUINT_T)pFanin->pData );
            Vec_IntForEachEntry( vOuts, iOut, k )
            {
                pPo = Aig_ManCo( p->pCare, iOut );
                if ( Aig_ObjIsTravIdCurrent( p->pCare, pPo ) )
                    continue;
                Aig_ObjSetTravIdCurrent( p->pCare, pPo );
                if ( Aig_ObjFanin0(pPo) == Aig_ManConst1(p->pCare) )
                    continue;
                pObjAig = Abc_NtkConstructCare_rec( p->pCare, Aig_ObjFanin0(pPo), pMan );
                if ( pObjAig == NULL )
                    continue;
                pObjAig = Aig_NotCond( pObjAig, Aig_ObjFaninC0(pPo) );
                Aig_ObjCreateCo( pMan, pObjAig );
            }
        }
/*
        Aig_ManForEachCo( p->pCare, pPo, i )
        {
//            assert( Aig_ObjFanin0(pPo) != Aig_ManConst1(p->pCare) );
            if ( Aig_ObjFanin0(pPo) == Aig_ManConst1(p->pCare) )
                continue;
            pObjAig = Abc_NtkConstructCare_rec( p->pCare, Aig_ObjFanin0(pPo), pMan );
            if ( pObjAig == NULL )
                continue;
            pObjAig = Aig_NotCond( pObjAig, Aig_ObjFaninC0(pPo) );
            Aig_ObjCreateCo( pMan, pObjAig );
        }
*/
    }
    if ( p->pPars->fResub )
    {
        // construct the node
        pObjAig = (Aig_Obj_t *)pNode->pCopy;
        Aig_ObjCreateCo( pMan, pObjAig );
        // construct the divisors
        Vec_PtrForEachEntry( Abc_Obj_t *, p->vDivs, pFanin, i )
        {
            pObjAig = (Aig_Obj_t *)pFanin->pCopy;
            Aig_ObjCreateCo( pMan, pObjAig );
        }
    }
    else
    {
        // construct the fanins
        Abc_ObjForEachFanin( pNode, pFanin, i )
        {
            pObjAig = (Aig_Obj_t *)pFanin->pCopy;
            Aig_ObjCreateCo( pMan, pObjAig );
        }
    }
    Aig_ManCleanup( pMan );
    return pMan;
}

/**Function*************************************************************

  Synopsis    [Creates AIG for the window with constraints.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Abc_NtkAigForConstraints( Mfs_Man_t * p, Abc_Obj_t * pNode )
{
    Abc_Obj_t * pFanin;
    Aig_Man_t * pMan;
    Aig_Obj_t * pPi, * pPo, * pObjAig, * pObjRoot;
    Vec_Int_t * vOuts;
    int i, k, iOut;
    if ( p->pCare == NULL )
        return NULL;
    pMan = Aig_ManStart( 1000 );
    // mark the care set
    Aig_ManIncrementTravId( p->pCare );
    Vec_PtrForEachEntry( Abc_Obj_t *, p->vSupp, pFanin, i )
    {
        pPi = Aig_ManCi( p->pCare, (int)(ABC_PTRUINT_T)pFanin->pData );
        Aig_ObjSetTravIdCurrent( p->pCare, pPi );
        pPi->pData = Aig_ObjCreateCi(pMan);
    }
    // construct the constraints
    pObjRoot = Aig_ManConst1(pMan);
    Vec_PtrForEachEntry( Abc_Obj_t *, p->vSupp, pFanin, i )
    {
        vOuts = (Vec_Int_t *)Vec_PtrEntry( p->vSuppsInv, (int)(ABC_PTRUINT_T)pFanin->pData );
        Vec_IntForEachEntry( vOuts, iOut, k )
        {
            pPo = Aig_ManCo( p->pCare, iOut );
            if ( Aig_ObjIsTravIdCurrent( p->pCare, pPo ) )
                continue;
            Aig_ObjSetTravIdCurrent( p->pCare, pPo );
            if ( Aig_ObjFanin0(pPo) == Aig_ManConst1(p->pCare) )
                continue;
            pObjAig = Abc_NtkConstructCare_rec( p->pCare, Aig_ObjFanin0(pPo), pMan );
            if ( pObjAig == NULL )
                continue;
            pObjAig = Aig_NotCond( pObjAig, Aig_ObjFaninC0(pPo) );
            pObjRoot = Aig_And( pMan, pObjRoot, pObjAig );
        }
    }
    Aig_ObjCreateCo( pMan, pObjRoot );
    Aig_ManCleanup( pMan );
    return pMan;
}

ABC_NAMESPACE_IMPL_END

#include "proof/fra/fra.h"

ABC_NAMESPACE_IMPL_START


/**Function*************************************************************

  Synopsis    [Compute the ratio of don't-cares.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
double Abc_NtkConstraintRatio( Mfs_Man_t * p, Abc_Obj_t * pNode )
{
    int nSimWords = 256;
    Aig_Man_t * pMan;
    Fra_Sml_t * pSim;
    int Counter;
    pMan = Abc_NtkAigForConstraints( p, pNode );
    pSim = Fra_SmlSimulateComb( pMan, nSimWords, 0 );
    Counter = Fra_SmlNodeCountOnes( pSim, Aig_ManCo(pMan, 0) );
    Aig_ManStop( pMan );
    Fra_SmlStop( pSim );
    return 1.0 * Counter / (32 * nSimWords);
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

