/**CFile****************************************************************

  FileName    [aigTruth.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [AIG package.]

  Synopsis    [Computes truth table for the cut.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - April 28, 2007.]

  Revision    [$Id: aigTruth.c,v 1.00 2007/04/28 00:00:00 alanmi Exp $]

***********************************************************************/

#include "aig.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Computes truth table of the cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned * Aig_ManCutTruthOne( Aig_Obj_t * pNode, unsigned * pTruth, int nWords )
{
    unsigned * pTruth0, * pTruth1;
    int i;
    pTruth0 = (unsigned *)Aig_ObjFanin0(pNode)->pData;
    pTruth1 = (unsigned *)Aig_ObjFanin1(pNode)->pData;
    if ( Aig_ObjIsExor(pNode) )
        for ( i = 0; i < nWords; i++ )
            pTruth[i] = pTruth0[i] ^ pTruth1[i];
    else if ( !Aig_ObjFaninC0(pNode) && !Aig_ObjFaninC1(pNode) )
        for ( i = 0; i < nWords; i++ )
            pTruth[i] = pTruth0[i] & pTruth1[i];
    else if ( !Aig_ObjFaninC0(pNode) && Aig_ObjFaninC1(pNode) )
        for ( i = 0; i < nWords; i++ )
            pTruth[i] = pTruth0[i] & ~pTruth1[i];
    else if ( Aig_ObjFaninC0(pNode) && !Aig_ObjFaninC1(pNode) )
        for ( i = 0; i < nWords; i++ )
            pTruth[i] = ~pTruth0[i] & pTruth1[i];
    else // if ( Aig_ObjFaninC0(pNode) && Aig_ObjFaninC1(pNode) )
        for ( i = 0; i < nWords; i++ )
            pTruth[i] = ~pTruth0[i] & ~pTruth1[i];
    return pTruth;
}

/**Function*************************************************************

  Synopsis    [Computes truth table of the cut.]

  Description [The returned pointer should be used immediately.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned * Aig_ManCutTruth( Aig_Obj_t * pRoot, Vec_Ptr_t * vLeaves, Vec_Ptr_t * vNodes, Vec_Ptr_t * vTruthElem, Vec_Ptr_t * vTruthStore )
{
    Aig_Obj_t * pObj;
    int i, nWords;
    assert( Vec_PtrSize(vLeaves) <= Vec_PtrSize(vTruthElem) );
    assert( Vec_PtrSize(vNodes) <= Vec_PtrSize(vTruthStore) );
    assert( Vec_PtrSize(vNodes) == 0 || pRoot == Vec_PtrEntryLast(vNodes) );  
    // assign elementary truth tables
    Vec_PtrForEachEntry( Aig_Obj_t *, vLeaves, pObj, i )
        pObj->pData = Vec_PtrEntry( vTruthElem, i );
    // compute truths for other nodes
    nWords = Abc_TruthWordNum( Vec_PtrSize(vLeaves) );
    Vec_PtrForEachEntry( Aig_Obj_t *, vNodes, pObj, i )
        pObj->pData = Aig_ManCutTruthOne( pObj, (unsigned *)Vec_PtrEntry(vTruthStore, i), nWords );
    return (unsigned *)pRoot->pData;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

