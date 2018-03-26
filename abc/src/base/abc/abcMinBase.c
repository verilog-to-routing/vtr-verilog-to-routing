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

#ifdef ABC_USE_CUDD
#include "bdd/extrab/extraBdd.h"
#endif

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////
 
#ifdef ABC_USE_CUDD

extern int Abc_NodeSupport( DdNode * bFunc, Vec_Str_t * vSupport, int nVars );

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
            Abc_ObjDeleteFanin( pNode, (Abc_Obj_t *)vFanins->pArray[i] );
    assert( nVars == Abc_ObjFaninNum(pNode) );

    // update the function of the node
    pNode->pData = Extra_bddRemapUp( (DdManager *)pNode->pNtk->pManFunc, bTemp = (DdNode *)pNode->pData );   Cudd_Ref( (DdNode *)pNode->pData );
    Cudd_RecursiveDeref( (DdManager *)pNode->pNtk->pManFunc, bTemp );
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
                DdManager * dd = (DdManager *)pNode->pNtk->pManFunc;
                DdNode * bVar1 = Cudd_bddIthVar( dd, i );
                DdNode * bVar2 = Cudd_bddIthVar( dd, k );
                DdNode * bTrans, * bTemp;
                bTrans = Cudd_bddXnor( dd, bVar1, bVar2 ); Cudd_Ref( bTrans );
                pNode->pData = Cudd_bddAndAbstract( dd, bTemp = (DdNode *)pNode->pData, bTrans, bVar2 ); Cudd_Ref( (DdNode *)pNode->pData );
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



/**Function*************************************************************

  Synopsis    [Find the number of unique variables after collapsing.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NodeCheckDupFanin( Abc_Obj_t * pFanin, Abc_Obj_t * pFanout, int * piFanin )
{
    Abc_Obj_t * pObj;
    int i, Counter = 0;
    Abc_ObjForEachFanin( pFanout, pObj, i )
        if ( pObj == pFanin )
        {
            if ( piFanin )
                *piFanin = i;
            Counter++;
        }
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Find the number of unique variables after collapsing.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NodeCollapseSuppSize( Abc_Obj_t * pFanin, Abc_Obj_t * pFanout, Vec_Ptr_t * vFanins )
{
    Abc_Obj_t * pObj;
    int i;
    Vec_PtrClear( vFanins );
    Abc_ObjForEachFanin( pFanout, pObj, i )
        if ( pObj != pFanin )
            Vec_PtrPushUnique( vFanins, pObj );
    Abc_ObjForEachFanin( pFanin, pObj, i )
        Vec_PtrPushUnique( vFanins, pObj );
    return Vec_PtrSize( vFanins );
}

/**Function*************************************************************

  Synopsis    [Returns the index of the new fanin.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_ObjFaninNumberNew( Vec_Ptr_t * vFanins, Abc_Obj_t * pFanin )
{
    Abc_Obj_t * pObj;
    int i;
    Vec_PtrForEachEntry( Abc_Obj_t *, vFanins, pObj, i )
        if ( pObj == pFanin )
            return i;
    return -1;
}

/**Function*************************************************************

  Synopsis    [Find the permutation map for the given node into the new order.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NodeCollapsePermMap( Abc_Obj_t * pNode, Abc_Obj_t * pSkip, Vec_Ptr_t * vFanins, int * pPerm )
{
    Abc_Obj_t * pFanin;
    int i;
    for ( i = 0; i < Vec_PtrSize(vFanins); i++ )
        pPerm[i] = i;
    Abc_ObjForEachFanin( pNode, pFanin, i )
    {
        if ( pFanin == pSkip )
            continue;
        pPerm[i] = Abc_ObjFaninNumberNew( vFanins, pFanin );
        if ( pPerm[i] == -1 )
            return 0;
    }
    return 1;
}



/**Function*************************************************************

  Synopsis    [Eliminates the nodes into their fanouts if the node size does not exceed this number.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Abc_NodeCollapseFunc( Abc_Obj_t * pFanin, Abc_Obj_t * pFanout, Vec_Ptr_t * vFanins, int * pPermFanin, int * pPermFanout )
{
    DdManager * dd = (DdManager *)pFanin->pNtk->pManFunc;
    DdNode * bVar, * bFunc0, * bFunc1, * bTemp, * bFanin, * bFanout;
    int RetValue, nSize, iFanin;
    // can only eliminate if fanin occurs in the fanin list of the fanout exactly once
    if ( Abc_NodeCheckDupFanin( pFanin, pFanout, &iFanin ) != 1 )
        return NULL;
    // find the new number of fanins after collapsing
    nSize = Abc_NodeCollapseSuppSize( pFanin, pFanout, vFanins );
    bVar = Cudd_bddIthVar( dd, nSize - 1 );
    assert( nSize <= dd->size );
    // find the permutation after collapsing
    RetValue = Abc_NodeCollapsePermMap( pFanin, NULL, vFanins, pPermFanin );
    assert( RetValue );
    RetValue = Abc_NodeCollapsePermMap( pFanout, pFanin, vFanins, pPermFanout );
    assert( RetValue );
    // cofactor the local function of the node
    bVar   = Cudd_bddIthVar( dd, iFanin );
    bFunc0 = Cudd_Cofactor( dd, (DdNode *)pFanout->pData, Cudd_Not(bVar) ); Cudd_Ref( bFunc0 );
    bFunc1 = Cudd_Cofactor( dd, (DdNode *)pFanout->pData, bVar );           Cudd_Ref( bFunc1 );
    // find the permutation after collapsing
    bFunc0 = Cudd_bddPermute( dd, bTemp = bFunc0, pPermFanout );  Cudd_Ref( bFunc0 );
    Cudd_RecursiveDeref( dd, bTemp ); 
    bFunc1 = Cudd_bddPermute( dd, bTemp = bFunc1, pPermFanout );  Cudd_Ref( bFunc1 );
    Cudd_RecursiveDeref( dd, bTemp );
    bFanin = Cudd_bddPermute( dd, (DdNode *)pFanin->pData, pPermFanin );    Cudd_Ref( bFanin );
    // create the new function
    bFanout = Cudd_bddIte( dd, bFanin, bFunc1, bFunc0 );          Cudd_Ref( bFanout );
    Cudd_RecursiveDeref( dd, bFanin );
    Cudd_RecursiveDeref( dd, bFunc1 );
    Cudd_RecursiveDeref( dd, bFunc0 );
    Cudd_Deref( bFanout );
    return bFanout;
}
int Abc_NodeCollapse( Abc_Obj_t * pFanin, Abc_Obj_t * pFanout, Vec_Ptr_t * vFanins, int * pPermFanin, int * pPermFanout )
{
    Abc_Obj_t * pFanoutNew, * pObj;
    DdNode * bFanoutNew;
    int i;
    assert( Abc_NtkIsBddLogic(pFanin->pNtk) );
    assert( Abc_ObjIsNode(pFanin) );
    assert( Abc_ObjIsNode(pFanout) );
    bFanoutNew = Abc_NodeCollapseFunc( pFanin, pFanout, vFanins, pPermFanin, pPermFanout );  
    if ( bFanoutNew == NULL )
        return 0;
    Cudd_Ref( bFanoutNew );
    // create the new node
    pFanoutNew = Abc_NtkCreateNode( pFanin->pNtk );
    Vec_PtrForEachEntry( Abc_Obj_t *, vFanins, pObj, i )
        Abc_ObjAddFanin( pFanoutNew, pObj );
    pFanoutNew->pData = bFanoutNew;
    // minimize the node
    Abc_NodeMinimumBase( pFanoutNew );
    // transfer the fanout
    Abc_ObjTransferFanout( pFanout, pFanoutNew );
    assert( Abc_ObjFanoutNum( pFanout ) == 0 );
    Abc_NtkDeleteObj_rec( pFanout, 1 );
    return 1;
}
int Abc_NtkEliminate( Abc_Ntk_t * pNtk, int nMaxSize, int fReverse, int fVerbose )
{
    extern void Abc_NtkBddReorder( Abc_Ntk_t * pNtk, int fVerbose );
    Vec_Ptr_t * vFanouts, * vFanins, * vNodes;
    Abc_Obj_t * pNode, * pFanout;
    int * pPermFanin, * pPermFanout;
    int RetValue, i, k;
    assert( nMaxSize > 0 );
    assert( Abc_NtkIsLogic(pNtk) ); 
    // convert network to BDD representation
    if ( !Abc_NtkToBdd(pNtk) )
    {
        fprintf( stdout, "Converting to BDD has failed.\n" );
        return 0;
    }
    // prepare nodes for sweeping
    Abc_NtkRemoveDupFanins( pNtk );
    Abc_NtkMinimumBase( pNtk );
    Abc_NtkCleanup( pNtk, 0 );
    // get the nodes in the given order
    vNodes = fReverse? Abc_NtkDfsReverse( pNtk ) : Abc_NtkDfs( pNtk, 0 );
    // go through the nodes and decide is they can be eliminated
    pPermFanin = ABC_ALLOC( int, nMaxSize + 1000 );
    pPermFanout = ABC_ALLOC( int, nMaxSize + 1000 );
    vFanins = Vec_PtrAlloc( 1000 );
    vFanouts = Vec_PtrAlloc( 1000 );
    Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pNode, i )
    {
        if ( !Abc_ObjIsNode(pNode) ) // skip deleted nodes
            continue;
        if ( Abc_NodeFindCoFanout(pNode) != NULL )
            continue;
        if ( Abc_ObjFaninNum(pNode) > nMaxSize )
            continue;
        Abc_ObjForEachFanout( pNode, pFanout, k )
            if ( Abc_NodeCollapseSuppSize(pNode, pFanout, vFanins) > nMaxSize )
                break;
        if ( k < Abc_ObjFanoutNum(pNode) )
            continue;
        // perform elimination
        Abc_NodeCollectFanouts( pNode, vFanouts );
        Vec_PtrForEachEntry( Abc_Obj_t *, vFanouts, pFanout, k )
        {
            if ( fVerbose )
                printf( "Collapsing fanin %5d (supp =%2d) into fanout %5d (supp =%2d) ",
                    Abc_ObjId(pNode), Abc_ObjFaninNum(pNode), Abc_ObjId(pFanout), Abc_ObjFaninNum(pFanout) ); 
            RetValue = Abc_NodeCollapse( pNode, pFanout, vFanins, pPermFanin, pPermFanout );
            assert( RetValue );
            if ( fVerbose )
            {
                Abc_Obj_t * pNodeNew = Abc_NtkObj( pNtk, Abc_NtkObjNumMax(pNtk) - 1 );
                if ( pNodeNew )
                    printf( "resulting in node %5d (supp =%2d).\n", Abc_ObjId(pNodeNew), Abc_ObjFaninNum(pNodeNew) );
            }
        }
    }
    Abc_NtkBddReorder( pNtk, 0 );
    Vec_PtrFree( vFanins );
    Vec_PtrFree( vFanouts );
    Vec_PtrFree( vNodes );
    ABC_FREE( pPermFanin );
    ABC_FREE( pPermFanout );
    return 1;
}



/**Function*************************************************************

  Synopsis    [Check how many times fanin appears in the FF of the fanout.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NodeCountAppearances( Abc_Obj_t * pFanin, Abc_Obj_t * pFanout )
{
    Hop_Man_t * pMan = (Hop_Man_t *)pFanin->pNtk->pManFunc;
    int iFanin = Abc_NodeFindFanin( pFanout, pFanin );
    assert( iFanin >= 0 && iFanin < Hop_ManPiNum(pMan) );
    return Hop_ObjFanoutCount( (Hop_Obj_t *)pFanout->pData, Hop_IthVar(pMan, iFanin) );
}
int Abc_NodeCountAppearancesAll( Abc_Obj_t * pNode )
{
    Abc_Obj_t * pFanout;
    int i, Count = 0;
    Abc_ObjForEachFanout( pNode, pFanout, i )
        Count += Abc_NodeCountAppearances( pNode, pFanout );
    return Count;
}

/**Function*************************************************************

  Synopsis    [Performs traditional eliminate -1.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Hop_Obj_t * Abc_NodeCollapseFunc1( Abc_Obj_t * pFanin, Abc_Obj_t * pFanout, Vec_Ptr_t * vFanins, int * pPermFanin, int * pPermFanout )
{
    Hop_Man_t * pMan = (Hop_Man_t *)pFanin->pNtk->pManFunc;
    Hop_Obj_t * bFanin, * bFanout;
    int RetValue, nSize, iFanin;
    // can only eliminate if fanin occurs in the fanin list of the fanout exactly once
    if ( Abc_NodeCheckDupFanin( pFanin, pFanout, &iFanin ) != 1 )
        return NULL;
    // find the new number of fanins after collapsing
    nSize = Abc_NodeCollapseSuppSize( pFanin, pFanout, vFanins );
    Hop_IthVar( pMan, nSize ); // use additional var for fanin variable
    assert( nSize + 1 <= Hop_ManPiNum(pMan) );
    // find the permutation after collapsing
    RetValue = Abc_NodeCollapsePermMap( pFanin, NULL, vFanins, pPermFanin );
    assert( RetValue );
    RetValue = Abc_NodeCollapsePermMap( pFanout, pFanin, vFanins, pPermFanout );
    assert( RetValue );
    // include fanin's variable
    pPermFanout[iFanin] = nSize;
    // create new function of fanin and fanout
    bFanin  = Hop_Permute( pMan, (Hop_Obj_t *)pFanin->pData,  Abc_ObjFaninNum(pFanin),  pPermFanin );
    bFanout = Hop_Permute( pMan, (Hop_Obj_t *)pFanout->pData, Abc_ObjFaninNum(pFanout), pPermFanout );
    // compose fanin into fanout
    return Hop_Compose( pMan, bFanout, bFanin, nSize );
}
int Abc_NodeCollapse1( Abc_Obj_t * pFanin, Abc_Obj_t * pFanout, Vec_Ptr_t * vFanins, int * pPermFanin, int * pPermFanout )
{
    Abc_Obj_t * pFanoutNew, * pObj;
    Hop_Obj_t * bFanoutNew;
    int i;
    assert( Abc_NtkIsAigLogic(pFanin->pNtk) );
    assert( Abc_ObjIsNode(pFanin) );
    assert( Abc_ObjIsNode(pFanout) );
    bFanoutNew = Abc_NodeCollapseFunc1( pFanin, pFanout, vFanins, pPermFanin, pPermFanout );  
    if ( bFanoutNew == NULL )
        return 0;
    // create the new node
    pFanoutNew = Abc_NtkCreateNode( pFanin->pNtk );
    Vec_PtrForEachEntry( Abc_Obj_t *, vFanins, pObj, i )
        Abc_ObjAddFanin( pFanoutNew, pObj );
    pFanoutNew->pData = bFanoutNew;
    // transfer the fanout
    Abc_ObjTransferFanout( pFanout, pFanoutNew );
    assert( Abc_ObjFanoutNum( pFanout ) == 0 );
    Abc_NtkDeleteObj_rec( pFanout, 1 );
    return 1;
}
int Abc_NodeIsExor( Abc_Obj_t * pNode )
{
    Hop_Man_t * pMan;
    word Truth;
    if ( Abc_ObjFaninNum(pNode) < 3 || Abc_ObjFaninNum(pNode) > 6 )
        return 0;
    pMan = (Hop_Man_t *)pNode->pNtk->pManFunc;
    Truth = Hop_ManComputeTruth6( pMan, (Hop_Obj_t *)pNode->pData, Abc_ObjFaninNum(pNode) );
    if ( Truth == 0x6666666666666666 || Truth == 0x9999999999999999 || 
         Truth == 0x9696969696969696 || Truth == 0x6969696969696969 || 
         Truth == 0x6996699669966996 || Truth == 0x9669966996699669 || 
         Truth == 0x9669699696696996 || Truth == 0x6996966969969669 ||
         Truth == 0x6996966996696996 || Truth == 0x9669699669969669 )
         return 1;
    return 0;
}
int Abc_NtkEliminate1One( Abc_Ntk_t * pNtk, int ElimValue, int nMaxSize, int fReverse, int fVerbose )
{
    Vec_Ptr_t * vFanouts, * vFanins, * vNodes;
    Abc_Obj_t * pNode, * pFanout;
    int * pPermFanin, * pPermFanout;
    int RetValue, i, k;
    assert( nMaxSize > 0 );
    assert( Abc_NtkIsLogic(pNtk) ); 
    // convert network to BDD representation
    if ( !Abc_NtkToAig(pNtk) )
    {
        fprintf( stdout, "Converting to AIG has failed.\n" );
        return 0;
    }
    // get the nodes in the given order
    vNodes = fReverse? Abc_NtkDfsReverse( pNtk ) : Abc_NtkDfs( pNtk, 0 );
    // go through the nodes and decide is they can be eliminated
    pPermFanin = ABC_ALLOC( int, nMaxSize + 1000 );
    pPermFanout = ABC_ALLOC( int, nMaxSize + 1000 );
    vFanins = Vec_PtrAlloc( 1000 );
    vFanouts = Vec_PtrAlloc( 1000 );
    Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pNode, i )
    {
        if ( !Abc_ObjIsNode(pNode) ) // skip deleted nodes
            continue;
        if ( Abc_NodeFindCoFanout(pNode) != NULL )
            continue;
        if ( Abc_ObjFaninNum(pNode) > nMaxSize )
            continue;
        if ( Abc_NodeIsExor(pNode) )
            continue;
        // skip nodes with more than one fanout
//        if ( Abc_ObjFanoutNum(pNode) != 1 ) 
//            continue;
        // skip nodes that appear in the FF of their fanout more than once
        if ( Abc_NodeCountAppearancesAll( pNode ) > ElimValue + 2 ) 
            continue;       
        Abc_ObjForEachFanout( pNode, pFanout, k )
            if ( Abc_NodeCollapseSuppSize(pNode, pFanout, vFanins) > nMaxSize )
                break;
        if ( k < Abc_ObjFanoutNum(pNode) )
            continue;
        // perform elimination
        Abc_NodeCollectFanouts( pNode, vFanouts );
        Vec_PtrForEachEntry( Abc_Obj_t *, vFanouts, pFanout, k )
        {
            if ( fVerbose )
                printf( "Collapsing fanin %5d (supp =%2d) into fanout %5d (supp =%2d) ",
                    Abc_ObjId(pNode), Abc_ObjFaninNum(pNode), Abc_ObjId(pFanout), Abc_ObjFaninNum(pFanout) ); 
            RetValue = Abc_NodeCollapse1( pNode, pFanout, vFanins, pPermFanin, pPermFanout );
            assert( RetValue );
            if ( fVerbose )
            {
                Abc_Obj_t * pNodeNew = Abc_NtkObj( pNtk, Abc_NtkObjNumMax(pNtk) - 1 );
                if ( pNodeNew )
                    printf( "resulting in node %5d (supp =%2d).\n", Abc_ObjId(pNodeNew), Abc_ObjFaninNum(pNodeNew) );
            }
        }
    }
    Vec_PtrFree( vFanins );
    Vec_PtrFree( vFanouts );
    Vec_PtrFree( vNodes );
    ABC_FREE( pPermFanin );
    ABC_FREE( pPermFanout );
    return 1;
}
int Abc_NtkEliminate1( Abc_Ntk_t * pNtk, int ElimValue, int nMaxSize, int nIterMax, int fReverse, int fVerbose )
{
    int i;
    for ( i = 0; i < nIterMax; i++ )
    {
        int nNodes = Abc_NtkNodeNum(pNtk);
//        printf( "%d ", nNodes );
        if ( !Abc_NtkEliminate1One(pNtk, ElimValue, nMaxSize, fReverse, fVerbose) )
            return 0;
        if ( nNodes == Abc_NtkNodeNum(pNtk) )
            break;
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    [Sort nodes in the reverse topo order.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_ObjCompareByNumber( Abc_Obj_t ** pp1, Abc_Obj_t ** pp2 )
{
    return Abc_ObjRegular(*pp1)->iTemp - Abc_ObjRegular(*pp2)->iTemp;
}
void Abc_ObjSortInReverseOrder( Abc_Ntk_t * pNtk, Vec_Ptr_t * vNodes )
{
    Vec_Ptr_t * vOrder;
    Abc_Obj_t * pNode; 
    int i;
    vOrder = Abc_NtkDfsReverse( pNtk );
    Vec_PtrForEachEntry( Abc_Obj_t *, vOrder, pNode, i )
        pNode->iTemp = i;
    Vec_PtrSort( vNodes, (int (*)())Abc_ObjCompareByNumber );
    Vec_PtrForEachEntry( Abc_Obj_t *, vOrder, pNode, i )
        pNode->iTemp = 0;
    Vec_PtrFree( vOrder );
}


/**Function*************************************************************

  Synopsis    [Performs traditional eliminate -1.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkEliminateSpecial( Abc_Ntk_t * pNtk, int nMaxSize, int fVerbose )
{
    extern void Abc_NtkBddReorder( Abc_Ntk_t * pNtk, int fVerbose );
    Vec_Ptr_t * vFanouts, * vFanins, * vNodes;
    Abc_Obj_t * pNode, * pFanout;
    int * pPermFanin, * pPermFanout;
    int RetValue, i, k;
    assert( nMaxSize > 0 );
    assert( Abc_NtkIsLogic(pNtk) ); 


    // convert network to BDD representation
    if ( !Abc_NtkToBdd(pNtk) )
    {
        fprintf( stdout, "Converting to BDD has failed.\n" );
        return 0;
    }

    // prepare nodes for sweeping
    Abc_NtkRemoveDupFanins( pNtk );
    Abc_NtkMinimumBase( pNtk );
    Abc_NtkCleanup( pNtk, 0 );

    // convert network to SOPs
    if ( !Abc_NtkToSop(pNtk, -1, ABC_INFINITY) )
    {
        fprintf( stdout, "Converting to SOP has failed.\n" );
        return 0;
    }

    // collect info about the nodes to be eliminated
    vNodes = Vec_PtrAlloc( 1000 );
    Abc_NtkForEachNode( pNtk, pNode, i )
    {
        if ( Abc_ObjFanoutNum(pNode) != 1 )
            continue;
        pFanout = Abc_ObjFanout0(pNode);
        if ( !Abc_ObjIsNode(pFanout) )
            continue;
        if ( Abc_SopGetCubeNum((char *)pNode->pData) != 1 )
            continue;
        if ( Abc_SopGetCubeNum((char *)pFanout->pData) != 1 )
            continue;
        // find the fanout's fanin
        RetValue = Abc_NodeFindFanin( pFanout, pNode );
        assert( RetValue >= 0 && RetValue < Abc_ObjFaninNum(pFanout) );
        // both pNode and pFanout are AND/OR type nodes
        if ( Abc_SopIsComplement((char *)pNode->pData) == Abc_SopGetIthCareLit((char *)pFanout->pData, RetValue) )
            continue;
        Vec_PtrPush( vNodes, pNode );
    }
    if ( Vec_PtrSize(vNodes) == 0 )
    {
        Vec_PtrFree( vNodes );
        return 1;
    }
    Abc_ObjSortInReverseOrder( pNtk, vNodes );

    // convert network to BDD representation
    if ( !Abc_NtkToBdd(pNtk) )
    {
        fprintf( stdout, "Converting to BDD has failed.\n" );
        return 0;
    }

    // go through the nodes and decide is they can be eliminated
    pPermFanin = ABC_ALLOC( int, nMaxSize + 1000 );
    pPermFanout = ABC_ALLOC( int, nMaxSize + 1000 );
    vFanins = Vec_PtrAlloc( 1000 );
    vFanouts = Vec_PtrAlloc( 1000 );
    Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pNode, i )
    {
        assert( Abc_ObjIsNode(pNode) );
        assert( Abc_NodeFindCoFanout(pNode) == NULL );
        // perform elimination
        Abc_NodeCollectFanouts( pNode, vFanouts );
        Vec_PtrForEachEntry( Abc_Obj_t *, vFanouts, pFanout, k )
        {
            if ( fVerbose )
                printf( "Collapsing fanin %5d (supp =%2d) into fanout %5d (supp =%2d) ",
                    Abc_ObjId(pNode), Abc_ObjFaninNum(pNode), Abc_ObjId(pFanout), Abc_ObjFaninNum(pFanout) ); 
            RetValue = Abc_NodeCollapse( pNode, pFanout, vFanins, pPermFanin, pPermFanout );
            assert( RetValue );
            if ( fVerbose )
            {
                Abc_Obj_t * pNodeNew = Abc_NtkObj( pNtk, Abc_NtkObjNumMax(pNtk) - 1 );
                if ( pNodeNew )
                    printf( "resulting in node %5d (supp =%2d).\n", Abc_ObjId(pNodeNew), Abc_ObjFaninNum(pNodeNew) );
            }
        }
    }
    Abc_NtkBddReorder( pNtk, 0 );
    Vec_PtrFree( vFanins );
    Vec_PtrFree( vFanouts );
    Vec_PtrFree( vNodes );
    ABC_FREE( pPermFanin );
    ABC_FREE( pPermFanout );
    return 1;
}

#else

int Abc_NtkMinimumBase( Abc_Ntk_t * pNtk )     { return 0; }
int Abc_NodeMinimumBase( Abc_Obj_t * pNode )   { return 0; }
int Abc_NtkRemoveDupFanins( Abc_Ntk_t * pNtk ) { return 0; }
int Abc_NtkEliminateSpecial( Abc_Ntk_t * pNtk, int nMaxSize, int fVerbose ) { return 0; }
int Abc_NtkEliminate( Abc_Ntk_t * pNtk, int nMaxSize, int fReverse, int fVerbose ) { return 0; }
int Abc_NtkEliminate1( Abc_Ntk_t * pNtk, int ElimValue, int nMaxSize, int nIterMax, int fReverse, int fVerbose ) { return 0; }

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

