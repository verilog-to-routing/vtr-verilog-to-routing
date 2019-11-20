/**CFile****************************************************************

  FileName    [abcDsd.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Network and node package.]

  Synopsis    [Decomposes the network using disjoint-support decomposition.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: abcDsd.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "abc.h"
#include "dsd.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////
 
static Abc_Ntk_t *     Abc_NtkDsdInternal( Abc_Ntk_t * pNtk, bool fVerbose, bool fPrint, bool fShort );
static void            Abc_NtkDsdConstruct( Dsd_Manager_t * pManDsd, Abc_Ntk_t * pNtk, Abc_Ntk_t * pNtkNew );
static Abc_Obj_t *     Abc_NtkDsdConstructNode( Dsd_Manager_t * pManDsd, Dsd_Node_t * pNodeDsd, Abc_Ntk_t * pNtkNew, int * pCounters );

static Vec_Ptr_t *     Abc_NtkCollectNodesForDsd( Abc_Ntk_t * pNtk );
static void            Abc_NodeDecompDsdAndMux( Abc_Obj_t * pNode, Vec_Ptr_t * vNodes, Dsd_Manager_t * pManDsd, bool fRecursive, int * pCounters );
static bool            Abc_NodeIsForDsd( Abc_Obj_t * pNode );
static int             Abc_NodeFindMuxVar( DdManager * dd, DdNode * bFunc, int nVars );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Derives the DSD network.]

  Description [Takes the strashed network (pNtk), derives global BDDs for
  the combinational outputs of this network, and decomposes these BDDs using
  disjoint support decomposition. Finally, constructs and return a new 
  network, which is topologically equivalent to the decomposition tree.
  Allocates and frees a new BDD manager and a new DSD manager.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkDsdGlobal( Abc_Ntk_t * pNtk, bool fVerbose, bool fPrint, bool fShort )
{
    DdManager * dd;
    Abc_Ntk_t * pNtkNew;
    assert( Abc_NtkIsStrash(pNtk) );
    dd = Abc_NtkBuildGlobalBdds( pNtk, 10000000, 1, 1, fVerbose );
    if ( dd == NULL )
        return NULL;
    if ( fVerbose )
        printf( "Shared BDD size = %6d nodes.\n", Cudd_ReadKeys(dd) - Cudd_ReadDead(dd) );
    // transform the result of mapping into a BDD network
    pNtkNew = Abc_NtkDsdInternal( pNtk, fVerbose, fPrint, fShort );
    Extra_StopManager( dd );
    if ( pNtkNew == NULL )
        return NULL;
    // copy EXDC network
    if ( pNtk->pExdc )
        pNtkNew->pExdc = Abc_NtkDup( pNtk->pExdc );
    if ( !Abc_NtkCheck( pNtkNew ) )
    {
        printf( "Abc_NtkDsdGlobal: The network check has failed.\n" );
        Abc_NtkDelete( pNtkNew );
        return NULL;
    }
    return pNtkNew;
}

/**Function*************************************************************

  Synopsis    [Constructs the decomposed network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkDsdInternal( Abc_Ntk_t * pNtk, bool fVerbose, bool fPrint, bool fShort )
{
    char ** ppNamesCi, ** ppNamesCo;
    Vec_Ptr_t * vFuncsGlob;
    Dsd_Manager_t * pManDsd;
    Abc_Ntk_t * pNtkNew;
    DdManager * dd;
    Abc_Obj_t * pObj;
    int i;

    // complement the global functions
    vFuncsGlob = Vec_PtrAlloc( Abc_NtkCoNum(pNtk) );
    Abc_NtkForEachCo( pNtk, pObj, i )
        Vec_PtrPush( vFuncsGlob, Cudd_NotCond(Abc_ObjGlobalBdd(pObj), Abc_ObjFaninC0(pObj)) );

    // perform the decomposition
    dd = Abc_NtkGlobalBddMan(pNtk);
    pManDsd = Dsd_ManagerStart( dd, Abc_NtkCiNum(pNtk), fVerbose );
    Dsd_Decompose( pManDsd, (DdNode **)vFuncsGlob->pArray, Abc_NtkCoNum(pNtk) );
    Vec_PtrFree( vFuncsGlob );
    Abc_NtkFreeGlobalBdds( pNtk, 0 );
    if ( pManDsd == NULL )
    {
        Cudd_Quit( dd );
        return NULL;
    }

    // start the new network
    pNtkNew = Abc_NtkStartFrom( pNtk, ABC_NTK_LOGIC, ABC_FUNC_BDD );
    // make sure the new manager has enough inputs
    Cudd_bddIthVar( pNtkNew->pManFunc, dd->size-1 );
    // put the results into the new network (save new CO drivers in old CO drivers)
    Abc_NtkDsdConstruct( pManDsd, pNtk, pNtkNew );
    // finalize the new network
    Abc_NtkFinalize( pNtk, pNtkNew );
    // fix the problem with complemented and duplicated CO edges
    Abc_NtkLogicMakeSimpleCos( pNtkNew, 0 );
    if ( fPrint )
    {
        ppNamesCi = Abc_NtkCollectCioNames( pNtk, 0 );
        ppNamesCo = Abc_NtkCollectCioNames( pNtk, 1 );
        Dsd_TreePrint( stdout, pManDsd, ppNamesCi, ppNamesCo, fShort, -1 );
        free( ppNamesCi );
        free( ppNamesCo );
    }

    // stop the DSD manager
    Dsd_ManagerStop( pManDsd );
    return pNtkNew;
}

/**Function*************************************************************

  Synopsis    [Constructs the decomposed network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkDsdConstruct( Dsd_Manager_t * pManDsd, Abc_Ntk_t * pNtk, Abc_Ntk_t * pNtkNew )
{
    Dsd_Node_t ** ppNodesDsd;
    Dsd_Node_t * pNodeDsd;
    Abc_Obj_t * pNode, * pNodeNew, * pDriver;
    int i, nNodesDsd;

    // save the CI nodes in the DSD nodes
    Dsd_NodeSetMark( Dsd_ManagerReadConst1(pManDsd), (int)Abc_NtkCreateNodeConst1(pNtkNew) );
    Abc_NtkForEachCi( pNtk, pNode, i )
    {
        pNodeDsd = Dsd_ManagerReadInput( pManDsd, i );
        Dsd_NodeSetMark( pNodeDsd, (int)pNode->pCopy );
    }

    // collect DSD nodes in DFS order (leaves and const1 are not collected)
    ppNodesDsd = Dsd_TreeCollectNodesDfs( pManDsd, &nNodesDsd );
    for ( i = 0; i < nNodesDsd; i++ )
        Abc_NtkDsdConstructNode( pManDsd, ppNodesDsd[i], pNtkNew, NULL );
    free( ppNodesDsd );

    // set the pointers to the CO drivers
    Abc_NtkForEachCo( pNtk, pNode, i )
    {
        pDriver = Abc_ObjFanin0( pNode );
        if ( !Abc_ObjIsNode(pDriver) )
            continue;
        if ( !Abc_AigNodeIsAnd(pDriver) )
            continue;
        pNodeDsd = Dsd_ManagerReadRoot( pManDsd, i );
        pNodeNew = (Abc_Obj_t *)Dsd_NodeReadMark( Dsd_Regular(pNodeDsd) );
        assert( !Abc_ObjIsComplement(pNodeNew) );
        pDriver->pCopy = Abc_ObjNotCond( pNodeNew, Dsd_IsComplement(pNodeDsd) );
    }
}

/**Function*************************************************************

  Synopsis    [Performs DSD using the manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Obj_t * Abc_NtkDsdConstructNode( Dsd_Manager_t * pManDsd, Dsd_Node_t * pNodeDsd, Abc_Ntk_t * pNtkNew, int * pCounters )
{
    DdManager * ddDsd = Dsd_ManagerReadDd( pManDsd );
    DdManager * ddNew = pNtkNew->pManFunc;
    Dsd_Node_t * pFaninDsd;
    Abc_Obj_t * pNodeNew, * pFanin;
    DdNode * bLocal, * bTemp, * bVar;
    Dsd_Type_t Type;
    int i, nDecs;

    // create the new node
    pNodeNew = Abc_NtkCreateNode( pNtkNew );
    // add the fanins
    Type  = Dsd_NodeReadType( pNodeDsd );
    nDecs = Dsd_NodeReadDecsNum( pNodeDsd );
    assert( nDecs > 1 );
    for ( i = 0; i < nDecs; i++ )
    {
        pFaninDsd  = Dsd_NodeReadDec( pNodeDsd, i );
        pFanin     = (Abc_Obj_t *)Dsd_NodeReadMark(Dsd_Regular(pFaninDsd));
        Abc_ObjAddFanin( pNodeNew, pFanin );
        assert( Type == DSD_NODE_OR || !Dsd_IsComplement(pFaninDsd) );
    }

    // create the local function depending on the type of the node
    ddNew = pNtkNew->pManFunc;
    switch ( Type )
    {
        case DSD_NODE_CONST1:
        {
            bLocal = ddNew->one; Cudd_Ref( bLocal );
            break;
        }
        case DSD_NODE_OR:
        {
            bLocal = Cudd_Not(ddNew->one); Cudd_Ref( bLocal );
            for ( i = 0; i < nDecs; i++ )
            {
                pFaninDsd  = Dsd_NodeReadDec( pNodeDsd, i );
                bVar   = Cudd_NotCond( ddNew->vars[i], Dsd_IsComplement(pFaninDsd) );
                bLocal = Cudd_bddOr( ddNew, bTemp = bLocal, bVar );               Cudd_Ref( bLocal );
                Cudd_RecursiveDeref( ddNew, bTemp );
            }
            break;
        }
        case DSD_NODE_EXOR:
        {
            bLocal = Cudd_Not(ddNew->one); Cudd_Ref( bLocal );
            for ( i = 0; i < nDecs; i++ )
            {
                bLocal = Cudd_bddXor( ddNew, bTemp = bLocal, ddNew->vars[i] );    Cudd_Ref( bLocal );
                Cudd_RecursiveDeref( ddNew, bTemp );
            }
            break;
        }
        case DSD_NODE_PRIME:
        {
            if ( pCounters )
            {
                if ( nDecs < 10 )
                    pCounters[nDecs]++;
                else
                    pCounters[10]++;
            }
            bLocal = Dsd_TreeGetPrimeFunction( ddDsd, pNodeDsd );                Cudd_Ref( bLocal );
            bLocal = Extra_TransferLevelByLevel( ddDsd, ddNew, bTemp = bLocal ); Cudd_Ref( bLocal );
/*
if ( nDecs == 3 )
{
Extra_bddPrint( ddDsd, bTemp );
printf( "\n" );
}
*/
            Cudd_RecursiveDeref( ddDsd, bTemp );
            // bLocal is now in the new BDD manager
            break;
        }
        default:
        {
            assert( 0 );
            break;
        }
    }
    pNodeNew->pData = bLocal;
    Dsd_NodeSetMark( pNodeDsd, (int)pNodeNew );
    return pNodeNew;
}






/**Function*************************************************************

  Synopsis    [Recursively decomposes internal nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkDsdLocal( Abc_Ntk_t * pNtk, bool fVerbose, bool fRecursive )
{
    Dsd_Manager_t * pManDsd;
    DdManager * dd = pNtk->pManFunc;
    Vec_Ptr_t * vNodes;
    int i;
    int pCounters[11] = {0};

    assert( Abc_NtkIsBddLogic(pNtk) );

    // make the network minimum base
    Abc_NtkMinimumBase( pNtk );

    // start the DSD manager
    pManDsd = Dsd_ManagerStart( dd, dd->size, 0 );

    // collect nodes for decomposition
    vNodes = Abc_NtkCollectNodesForDsd( pNtk );
    for ( i = 0; i < vNodes->nSize; i++ )
        Abc_NodeDecompDsdAndMux( vNodes->pArray[i], vNodes, pManDsd, fRecursive, pCounters );
    Vec_PtrFree( vNodes );

    printf( "Number of non-decomposable functions:\n" );
    for ( i = 3; i < 10; i++ )
        printf( "Inputs = %d.  Functions = %6d.\n", i, pCounters[i] );
    printf( "Inputs > %d.  Functions = %6d.\n", 9, pCounters[10] );

    // stop the DSD manager
    Dsd_ManagerStop( pManDsd );

    // make sure everything is okay
    if ( !Abc_NtkCheck( pNtk ) )
    {
        printf( "Abc_NtkDsdRecursive: The network check has failed.\n" );
        return 0;
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    [Collects the nodes that may need decomposition.]

  Description [The nodes that do not need decomposition are those
  whose BDD has more internal nodes than the support size.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Abc_NtkCollectNodesForDsd( Abc_Ntk_t * pNtk )
{
    Vec_Ptr_t * vNodes;
    Abc_Obj_t * pNode;
    int i;
    vNodes = Vec_PtrAlloc( 100 );
    Abc_NtkForEachNode( pNtk, pNode, i )
    {
        if ( Abc_NodeIsForDsd(pNode) )
            Vec_PtrPush( vNodes, pNode );
    }
    return vNodes;
}

/**Function*************************************************************

  Synopsis    [Performs decomposition of one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NodeDecompDsdAndMux( Abc_Obj_t * pNode, Vec_Ptr_t * vNodes, Dsd_Manager_t * pManDsd, bool fRecursive, int * pCounters )
{
    DdManager * dd = pNode->pNtk->pManFunc;
    Abc_Obj_t * pRoot, * pFanin, * pNode1, * pNode2, * pNodeC;
    Dsd_Node_t ** ppNodesDsd, * pNodeDsd, * pFaninDsd;
    int i, nNodesDsd, iVar, fCompl;

    // try disjoint support decomposition
    pNodeDsd = Dsd_DecomposeOne( pManDsd, pNode->pData );
    fCompl   = Dsd_IsComplement( pNodeDsd );
    pNodeDsd = Dsd_Regular( pNodeDsd );

    // determine what decomposition to use   
    if ( !fRecursive || Dsd_NodeReadDecsNum(pNodeDsd) != Abc_ObjFaninNum(pNode) )
    { // perform DSD

        // set the inputs
        Abc_ObjForEachFanin( pNode, pFanin, i )
        {
            pFaninDsd = Dsd_ManagerReadInput( pManDsd, i );
            Dsd_NodeSetMark( pFaninDsd, (int)pFanin );
        }

        // construct the intermediate nodes
        ppNodesDsd = Dsd_TreeCollectNodesDfsOne( pManDsd, pNodeDsd, &nNodesDsd );
        for ( i = 0; i < nNodesDsd; i++ )
        {
            pRoot = Abc_NtkDsdConstructNode( pManDsd, ppNodesDsd[i], pNode->pNtk, pCounters );
            if ( Abc_NodeIsForDsd(pRoot) && fRecursive )
                Vec_PtrPush( vNodes, pRoot );
        }
        free( ppNodesDsd );

        // remove the current fanins
        Abc_ObjRemoveFanins( pNode );
        // add fanin to the root
        Abc_ObjAddFanin( pNode, pRoot );
        // update the function to be that of buffer
        Cudd_RecursiveDeref( dd, pNode->pData );
        pNode->pData = Cudd_NotCond( dd->vars[0], fCompl ); Cudd_Ref( pNode->pData );
    }
    else // perform MUX-decomposition
    {
        // get the cofactoring variable
        iVar = Abc_NodeFindMuxVar( dd, pNode->pData, Abc_ObjFaninNum(pNode) );
        pNodeC = Abc_ObjFanin( pNode, iVar );

        // get the negative cofactor
        pNode1 = Abc_NtkCloneObj( pNode );
        pNode1->pData = Cudd_Cofactor( dd, pNode->pData, Cudd_Not(dd->vars[iVar]) );  Cudd_Ref( pNode1->pData );
        Abc_NodeMinimumBase( pNode1 );
        if ( Abc_NodeIsForDsd(pNode1) )
            Vec_PtrPush( vNodes, pNode1 );

        // get the positive cofactor
        pNode2 = Abc_NtkCloneObj( pNode );
        pNode2->pData = Cudd_Cofactor( dd, pNode->pData, dd->vars[iVar] );            Cudd_Ref( pNode2->pData );
        Abc_NodeMinimumBase( pNode2 );
        if ( Abc_NodeIsForDsd(pNode2) )
            Vec_PtrPush( vNodes, pNode2 );

        // remove the current fanins
        Abc_ObjRemoveFanins( pNode );
        // add new fanins
        Abc_ObjAddFanin( pNode, pNodeC );
        Abc_ObjAddFanin( pNode, pNode2 );
        Abc_ObjAddFanin( pNode, pNode1 );
        // update the function to be that of MUX
        Cudd_RecursiveDeref( dd, pNode->pData );
        pNode->pData = Cudd_bddIte( dd, dd->vars[0], dd->vars[1], dd->vars[2] );    Cudd_Ref( pNode->pData );
    }
}

/**Function*************************************************************

  Synopsis    [Checks if the node should be decomposed by DSD.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Abc_NodeIsForDsd( Abc_Obj_t * pNode )
{
    DdManager * dd = pNode->pNtk->pManFunc;
//    DdNode * bFunc, * bFunc0, * bFunc1;
    assert( Abc_ObjIsNode(pNode) );
//    if ( Cudd_DagSize(pNode->pData)-1 > Abc_ObjFaninNum(pNode) )
//        return 1;
//    return 0;

/*
    // this does not catch things like a(b+c), which should be decomposed
    for ( bFunc = Cudd_Regular(pNode->pData); !cuddIsConstant(bFunc); )
    {
        bFunc0 = Cudd_Regular( cuddE(bFunc) );
        bFunc1 = cuddT(bFunc);
        if ( bFunc0 == b1 )
            bFunc = bFunc1;
        else if ( bFunc1 == b1 || bFunc0 == bFunc1 )
            bFunc = bFunc0;
        else
            return 1;
    }
*/
    if ( Abc_ObjFaninNum(pNode) > 2 )
        return 1;
    return 0;
}

/**Function*************************************************************

  Synopsis    [Determines a cofactoring variable.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NodeFindMuxVar( DdManager * dd, DdNode * bFunc, int nVars )
{
	DdNode * bVar, * bCof0, * bCof1;
	int SuppSumMin = 1000000;
	int i, nSSD, nSSQ, iVar;

//	printf( "\n\nCofactors:\n\n" );
	iVar = -1;
	for ( i = 0; i < nVars; i++ )
	{
		bVar = dd->vars[i];

		bCof0 = Cudd_Cofactor( dd, bFunc, Cudd_Not(bVar) );  Cudd_Ref( bCof0 );
		bCof1 = Cudd_Cofactor( dd, bFunc,          bVar  );  Cudd_Ref( bCof1 );

//		nodD = Cudd_DagSize(bCof0);
//		nodQ = Cudd_DagSize(bCof1);
//		printf( "+%02d: D=%2d. Q=%2d.  ", i, nodD, nodQ );
//		printf( "S=%2d. D=%2d.  ", nodD + nodQ, abs(nodD-nodQ) );

		nSSD = Cudd_SupportSize( dd, bCof0 );
		nSSQ = Cudd_SupportSize( dd, bCof1 );

//		printf( "SD=%2d. SQ=%2d.  ", nSSD, nSSQ );
//		printf( "S=%2d. D=%2d.  ", nSSD + nSSQ, abs(nSSD - nSSQ) );
//		printf( "Cost=%3d. ", Cost(nodD,nodQ,nSSD,nSSQ) );
//		printf( "\n" );

		Cudd_RecursiveDeref( dd, bCof0 );
		Cudd_RecursiveDeref( dd, bCof1 );

		if ( SuppSumMin > nSSD + nSSQ )
		{
			 SuppSumMin = nSSD + nSSQ;
			 iVar = i;
		}
	}
    return iVar;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


