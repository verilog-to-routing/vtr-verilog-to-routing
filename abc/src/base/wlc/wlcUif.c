/**CFile****************************************************************

  FileName    [wlcUif.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Verilog parser.]

  Synopsis    [Abstraction for word-level networks.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - August 22, 2014.]

  Revision    [$Id: wlcUif.c,v 1.00 2014/09/12 00:00:00 alanmi Exp $]

***********************************************************************/

#include "wlc.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Collect adds and mults.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Wlc_NtkCollectBoxes( Wlc_Ntk_t * p, Vec_Int_t * vBoxIds )
{
    int i, iObj;
    Vec_Int_t * vBoxes = Vec_IntAlloc( Vec_IntSize(vBoxIds) + 1 );
    Vec_IntPush( vBoxes, Vec_IntSize(vBoxIds) );
    Vec_IntForEachEntry( vBoxIds, iObj, i )
        Vec_IntPush( vBoxes, Wlc_ObjNameId(p, iObj) );
    Abc_FrameSetBoxes( Vec_IntReleaseArray(vBoxes) );
    Vec_IntFree( vBoxes );
}
Vec_Int_t * Wlc_NtkCollectAddMult( Wlc_Ntk_t * p, Wlc_BstPar_t * pPar, int * pCountA, int * pCountM )
{
    Vec_Int_t * vBoxIds;
    Wlc_Obj_t * pObj;  int i;
    *pCountA = *pCountM = 0;
    if ( pPar->nAdderLimit == 0 && pPar->nMultLimit == 0 )
        return NULL;
    vBoxIds = Vec_IntAlloc( 100 );
    Wlc_NtkForEachObj( p, pObj, i )
    {
        if ( pObj->Type == WLC_OBJ_ARI_ADD && pPar->nAdderLimit && Wlc_ObjRange(pObj) >= pPar->nAdderLimit )
            Vec_IntPush( vBoxIds, i ), (*pCountA)++;
        else if ( pObj->Type == WLC_OBJ_ARI_MULTI && pPar->nMultLimit && Wlc_ObjRange(pObj) >= pPar->nMultLimit )
            Vec_IntPush( vBoxIds, i ), (*pCountM)++;
    }
    if ( Vec_IntSize( vBoxIds ) > 0 )
    {
        Wlc_NtkCollectBoxes( p, vBoxIds );
        return vBoxIds;
    }
    Vec_IntFree( vBoxIds );
    return NULL;
}

/**Function*************************************************************

  Synopsis    [Check if two objects have the same input/output signatures.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Wlc_NtkPairIsUifable( Wlc_Ntk_t * p, Wlc_Obj_t * pObj, Wlc_Obj_t * pObj2 )
{
    Wlc_Obj_t * pFanin, * pFanin2;  int k;
    if ( Wlc_ObjRange(pObj) != Wlc_ObjRange(pObj2) )
        return 0;
    if ( Wlc_ObjIsSigned(pObj) != Wlc_ObjIsSigned(pObj2) )
        return 0;
    if ( Wlc_ObjFaninNum(pObj) != Wlc_ObjFaninNum(pObj2) )
        return 0;
    for ( k = 0; k < Wlc_ObjFaninNum(pObj); k++ )
    {
        pFanin = Wlc_ObjFanin(p, pObj, k);
        pFanin2 = Wlc_ObjFanin(p, pObj2, k);
        if ( Wlc_ObjRange(pFanin) != Wlc_ObjRange(pFanin2) )
            return 0;
        if ( Wlc_ObjIsSigned(pFanin) != Wlc_ObjIsSigned(pFanin2) )
            return 0;
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    [Collect IDs of the multipliers.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Wlc_NtkCollectMultipliers( Wlc_Ntk_t * p )
{
    Wlc_Obj_t * pObj;  int i;
    Vec_Int_t * vBoxIds = Vec_IntAlloc( 100 );
    Wlc_NtkForEachObj( p, pObj, i )
        if ( pObj->Type == WLC_OBJ_ARI_MULTI )
            Vec_IntPush( vBoxIds, i );
    if ( Vec_IntSize( vBoxIds ) > 0 )
        return vBoxIds;
    Vec_IntFree( vBoxIds );
    return NULL;
}

/**Function*************************************************************

  Synopsis    [Returns all pairs of uifable multipliers.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Wlc_NtkFindUifableMultiplierPairs( Wlc_Ntk_t * p )
{
    Vec_Int_t * vMultis = Wlc_NtkCollectMultipliers( p );
    Vec_Int_t * vPairs = Vec_IntAlloc( 2 );
    Wlc_Obj_t * pObj, * pObj2;  int i, k;
    // iterate through unique pairs
    Wlc_NtkForEachObjVec( vMultis, p, pObj, i )
        Wlc_NtkForEachObjVec( vMultis, p, pObj2, k )
        {
            if ( k == i )  
                break;
            if ( Wlc_NtkPairIsUifable( p, pObj, pObj2 ) )
            {
                Vec_IntPush( vPairs, Wlc_ObjId(p, pObj) );
                Vec_IntPush( vPairs, Wlc_ObjId(p, pObj2) );
            }
        }
    Vec_IntFree( vMultis );
    if ( Vec_IntSize( vPairs ) > 0 )
        return vPairs;
    Vec_IntFree( vPairs );
    return NULL;
}



/**Function*************************************************************

  Synopsis    [Abstracts nodes by replacing their outputs with new PIs.]

  Description [If array is NULL, abstract all multipliers.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Wlc_Ntk_t * Wlc_NtkAbstractNodes( Wlc_Ntk_t * p, Vec_Int_t * vNodesInit )
{
    Vec_Int_t * vNodes = vNodesInit;
    Wlc_Ntk_t * pNew;
    Wlc_Obj_t * pObj;
    int i, k, iObj, iFanin;
    // get multipliers if not given
    if ( vNodes == NULL )
        vNodes = Wlc_NtkCollectMultipliers( p );
    if ( vNodes == NULL )
        return NULL;
    // mark nodes
    Wlc_NtkForEachObjVec( vNodes, p, pObj, i )
        pObj->Mark = 1;
    // iterate through the nodes in the DFS order
    Wlc_NtkCleanCopy( p );
    Wlc_NtkForEachObj( p, pObj, i )
    {
        if ( i == Vec_IntSize(&p->vCopies) )
            break;
        if ( pObj->Mark ) {
            // clean
            pObj->Mark = 0;
            // add fresh PI with the same number of bits
            iObj = Wlc_ObjAlloc( p, WLC_OBJ_PI, Wlc_ObjIsSigned(pObj), Wlc_ObjRange(pObj) - 1, 0 );
        }
        else {
            // update fanins 
            Wlc_ObjForEachFanin( pObj, iFanin, k )
                Wlc_ObjFanins(pObj)[k] = Wlc_ObjCopy(p, iFanin);
            // node to remain
            iObj = i;
        }
        Wlc_ObjSetCopy( p, i, iObj );
    }
    // POs do not change in this procedure
    if ( vNodes != vNodesInit )
        Vec_IntFree( vNodes );
    // reconstruct topological order
    pNew = Wlc_NtkDupDfs( p, 0, 1 );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Adds UIF constraints to node pairs and updates POs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Wlc_Ntk_t * Wlc_NtkUifNodePairs( Wlc_Ntk_t * p, Vec_Int_t * vPairsInit )
{
    Vec_Int_t * vPairs = vPairsInit;
    Wlc_Ntk_t * pNew;
    Wlc_Obj_t * pObj, * pObj2;
    Vec_Int_t * vUifConstrs, * vCompares, * vFanins;
    int i, k, iObj, iObj2, iObjNew, iObjNew2;
    int iFanin, iFanin2, iFaninNew;
    // get multiplier pairs if not given
    if ( vPairs == NULL )
        vPairs = Wlc_NtkFindUifableMultiplierPairs( p );
    if ( vPairs == NULL )
        return NULL;
    // sanity checks
    assert( Vec_IntSize(vPairs) > 0 && Vec_IntSize(vPairs) % 2 == 0 );
    // iterate through node pairs
    vFanins = Vec_IntAlloc( 100 );
    vCompares = Vec_IntAlloc( 100 );
    vUifConstrs = Vec_IntAlloc( 100 );
    Vec_IntForEachEntryDouble( vPairs, iObj, iObj2, i )
    {
        // get two nodes
        pObj  = Wlc_NtkObj( p, iObj );
        pObj2 = Wlc_NtkObj( p, iObj2 );
        assert( Wlc_NtkPairIsUifable(p, pObj, pObj2) );
        // create fanin comparator nodes
        Vec_IntClear( vCompares );
        Wlc_ObjForEachFanin( pObj, iFanin, k )
        {
            iFanin2 = Wlc_ObjFaninId( pObj2, k );
            Vec_IntFillTwo( vFanins, 2, iFanin, iFanin2 );
            iFaninNew = Wlc_ObjCreate( p, WLC_OBJ_COMP_NOTEQU, 0, 0, 0, vFanins );
            Vec_IntPush( vCompares, iFaninNew );
            // note that a pointer to Wlc_Obj_t (for example, pObj) can be invalidated after a call to 
            // Wlc_ObjCreate() due to a possible realloc of the internal array of objects...
            pObj = Wlc_NtkObj( p, iObj );
        }
        // concatenate fanin comparators
        iObjNew = Wlc_ObjCreate( p, WLC_OBJ_BIT_CONCAT, 0, Vec_IntSize(vCompares) - 1, 0, vCompares );
        // create reduction-OR node
        Vec_IntFill( vFanins, 1, iObjNew );
        iObjNew = Wlc_ObjCreate( p, WLC_OBJ_REDUCT_OR, 0, 0, 0, vFanins );
        // craete output comparator node
        Vec_IntFillTwo( vFanins, 2, iObj, iObj2 );
        iObjNew2 = Wlc_ObjCreate( p, WLC_OBJ_COMP_EQU, 0, 0, 0, vFanins );
        // create implication node (iObjNew is already complemented above)
        Vec_IntFillTwo( vFanins, 2, iObjNew, iObjNew2 );
        iObjNew = Wlc_ObjCreate( p, WLC_OBJ_LOGIC_OR, 0, 0, 0, vFanins );
        // save the constraint
        Vec_IntPush( vUifConstrs, iObjNew );
    }
    // derive the AND of the UIF contraints
    assert( Vec_IntSize(vUifConstrs) > 0 );
    if ( Vec_IntSize(vUifConstrs) == 1 )
        iObjNew = Vec_IntEntry( vUifConstrs, 0 );
    else
    {
        // concatenate
        iObjNew = Wlc_ObjCreate( p, WLC_OBJ_BIT_CONCAT, 0, Vec_IntSize(vUifConstrs) - 1, 0, vUifConstrs );
        // create reduction-AND node
        Vec_IntFill( vFanins, 1, iObjNew );
        iObjNew = Wlc_ObjCreate( p, WLC_OBJ_REDUCT_AND, 0, 0, 0, vFanins );
    }
    // update each PO to point to the new node
    Wlc_NtkForEachPo( p, pObj, i )
    {
        iObj = Wlc_ObjId(p, pObj);
        Vec_IntFillTwo( vFanins, 2, iObj, iObjNew );
        iObjNew = Wlc_ObjCreate( p, WLC_OBJ_LOGIC_AND, 0, 0, 0, vFanins );
        // note that a pointer to Wlc_Obj_t (for example, pObj) can be invalidated after a call to 
        // Wlc_ObjCreate() due to a possible realloc of the internal array of objects...
        pObj = Wlc_NtkObj( p, iObj );
        // update PO/CO arrays
        assert( Vec_IntEntry(&p->vPos, i) == iObj );
        assert( Vec_IntEntry(&p->vCos, i) == iObj );
        Vec_IntWriteEntry( &p->vPos, i, iObjNew );
        Vec_IntWriteEntry( &p->vCos, i, iObjNew );
        // transfer the PO attribute
        Wlc_NtkObj(p, iObjNew)->fIsPo = 1;
        assert( pObj->fIsPo );
        pObj->fIsPo = 0;
    }
    // cleanup
    Vec_IntFree( vUifConstrs );
    Vec_IntFree( vCompares );
    Vec_IntFree( vFanins );
    if ( vPairs != vPairsInit )
        Vec_IntFree( vPairs );
    // reconstruct topological order
    pNew = Wlc_NtkDupDfs( p, 0, 1 );
    return pNew;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

