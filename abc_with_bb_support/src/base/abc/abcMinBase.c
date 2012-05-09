/**CFile****************************************************************

  FileName    [abcMinBase.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Network and node package.]

  Synopsis    [Makes nodes of the network minimum base.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: abcMinBase.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "abc.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////
 
static int Abc_NodeSupport( DdNode * bFunc, Vec_Str_t * vSupport, int nVars );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Makes nodes minimum base.]

  Description [Returns the number of changed nodes.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkMinimumBase( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pNode;
    int i, Counter;
    assert( Abc_NtkIsBddLogic(pNtk) );
    Counter = 0;
    Abc_NtkForEachNode( pNtk, pNode, i )
        Counter += Abc_NodeMinimumBase( pNode );
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Makes one node minimum base.]

  Description [Returns 1 if the node is changed.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NodeMinimumBase( Abc_Obj_t * pNode )
{
    Vec_Str_t * vSupport;
    Vec_Ptr_t * vFanins;
    DdNode * bTemp;
    int i, nVars;

    assert( Abc_NtkIsBddLogic(pNode->pNtk) );
    assert( Abc_ObjIsNode(pNode) );

    // compute support
    vSupport = Vec_StrAlloc( 10 );
    nVars = Abc_NodeSupport( Cudd_Regular(pNode->pData), vSupport, Abc_ObjFaninNum(pNode) );
    if ( nVars == Abc_ObjFaninNum(pNode) )
    {
        Vec_StrFree( vSupport );
        return 0;
    }

    // remove unused fanins
    vFanins = Vec_PtrAlloc( Abc_ObjFaninNum(pNode) );
    Abc_NodeCollectFanins( pNode, vFanins );
    for ( i = 0; i < vFanins->nSize; i++ )
        if ( vSupport->pArray[i] == 0 )
            Abc_ObjDeleteFanin( pNode, vFanins->pArray[i] );
    assert( nVars == Abc_ObjFaninNum(pNode) );

    // update the function of the node
    pNode->pData = Extra_bddRemapUp( pNode->pNtk->pManFunc, bTemp = pNode->pData );   Cudd_Ref( pNode->pData );
    Cudd_RecursiveDeref( pNode->pNtk->pManFunc, bTemp );
    Vec_PtrFree( vFanins );
    Vec_StrFree( vSupport );
    return 1;
}

/**Function*************************************************************

  Synopsis    [Makes nodes of the network fanin-dup-free.]

  Description [Returns the number of pairs of duplicated fanins.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkRemoveDupFanins( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pNode;
    int i, Counter;
    assert( Abc_NtkIsBddLogic(pNtk) );
    Counter = 0;
    Abc_NtkForEachNode( pNtk, pNode, i )
        Counter += Abc_NodeRemoveDupFanins( pNode );
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Removes one pair of duplicated fanins if present.]

  Description [Returns 1 if the node is changed.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NodeRemoveDupFanins_int( Abc_Obj_t * pNode )
{
    Abc_Obj_t * pFanin1, * pFanin2;
    int i, k;
    assert( Abc_NtkIsBddLogic(pNode->pNtk) );
    assert( Abc_ObjIsNode(pNode) );
    // make sure fanins are not duplicated
    Abc_ObjForEachFanin( pNode, pFanin2, i )
    {
        Abc_ObjForEachFanin( pNode, pFanin1, k )
        {
            if ( k >= i )
                break;
            if ( pFanin1 == pFanin2 )
            {
                DdManager * dd = pNode->pNtk->pManFunc;
                DdNode * bVar1 = Cudd_bddIthVar( dd, i );
                DdNode * bVar2 = Cudd_bddIthVar( dd, k );
                DdNode * bTrans, * bTemp;
                bTrans = Cudd_bddXnor( dd, bVar1, bVar2 ); Cudd_Ref( bTrans );
                pNode->pData = Cudd_bddAndAbstract( dd, bTemp = pNode->pData, bTrans, bVar2 ); Cudd_Ref( pNode->pData );
                Cudd_RecursiveDeref( dd, bTemp );
                Cudd_RecursiveDeref( dd, bTrans );
                Abc_NodeMinimumBase( pNode );
                return 1;
            }
        }
    }
    return 0;
}

/**Function*************************************************************

  Synopsis    [Removes duplicated fanins if present.]

  Description [Returns the number of fanins removed.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NodeRemoveDupFanins( Abc_Obj_t * pNode )
{
    int Counter = 0;
    while ( Abc_NodeRemoveDupFanins_int(pNode) )
        Counter++;
    return Counter;
}
/**Function*************************************************************

  Synopsis    [Computes support of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NodeSupport_rec( DdNode * bFunc, Vec_Str_t * vSupport )
{
    if ( cuddIsConstant(bFunc) || Cudd_IsComplement(bFunc->next) )
        return;
    vSupport->pArray[ bFunc->index ] = 1;
    Abc_NodeSupport_rec( cuddT(bFunc), vSupport );
    Abc_NodeSupport_rec( Cudd_Regular(cuddE(bFunc)), vSupport );
    bFunc->next = Cudd_Not(bFunc->next);
}

/**Function*************************************************************

  Synopsis    [Computes support of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NodeSupportClear_rec( DdNode * bFunc )
{
    if ( !Cudd_IsComplement(bFunc->next) )
        return;
    bFunc->next = Cudd_Regular(bFunc->next);
    if ( cuddIsConstant(bFunc) )
        return;
    Abc_NodeSupportClear_rec( cuddT(bFunc) );
    Abc_NodeSupportClear_rec( Cudd_Regular(cuddE(bFunc)) );
}

/**Function*************************************************************

  Synopsis    [Computes support of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NodeSupport( DdNode * bFunc, Vec_Str_t * vSupport, int nVars )
{
    int Counter, i;
    // compute the support by marking the BDD
    Vec_StrFill( vSupport, nVars, 0 );
    Abc_NodeSupport_rec( bFunc, vSupport );
    // clear the marak
    Abc_NodeSupportClear_rec( bFunc );
    // get the number of support variables
    Counter = 0;
    for ( i = 0; i < nVars; i++ )
        Counter += vSupport->pArray[i];
    return Counter;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


