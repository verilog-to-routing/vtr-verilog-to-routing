/**CFile****************************************************************

  FileName    [extraBddImage.c]

  PackageName [extra]

  Synopsis    [Various reusable software utilities.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 1, 2003.]

  Revision    [$Id: extraBddImage.c,v 1.0 2003/09/01 00:00:00 alanmi Exp $]

***********************************************************************/

#include "extraBdd.h"

ABC_NAMESPACE_IMPL_START


/* 
    The ideas implemented in this file are inspired by the paper:
    Pankaj Chauhan, Edmund Clarke, Somesh Jha, Jim Kukula, Tom Shiple, 
    Helmut Veith, Dong Wang. Non-linear Quantification Scheduling in 
    Image Computation. ICCAD, 2001.
*/

/*---------------------------------------------------------------------------*/
/* Constant declarations                                                     */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Stucture declarations                                                     */
/*---------------------------------------------------------------------------*/

typedef struct Extra_ImageNode_t_  Extra_ImageNode_t;
typedef struct Extra_ImagePart_t_  Extra_ImagePart_t;
typedef struct Extra_ImageVar_t_   Extra_ImageVar_t;

struct Extra_ImageTree_t_
{
    Extra_ImageNode_t * pRoot;      // the root of quantification tree
    Extra_ImageNode_t * pCare;      // the leaf node with the care set
    DdNode *            bCareSupp;  // the cube to quantify from the care
    int                 fVerbose;   // the verbosity flag
    int                 nNodesMax;  // the max number of nodes in one iter
    int                 nNodesMaxT; // the overall max number of nodes
    int                 nIter;      // the number of iterations with this tree
};

struct Extra_ImageNode_t_
{
    DdManager *         dd;         // the manager 
    DdNode *            bCube;      // the cube to quantify
    DdNode *            bImage;     // the partial image
    Extra_ImageNode_t * pNode1;     // the first branch
    Extra_ImageNode_t * pNode2;     // the second branch
    Extra_ImagePart_t * pPart;      // the partition (temporary)
};

struct Extra_ImagePart_t_
{
    DdNode *            bFunc;      // the partition
    DdNode *            bSupp;      // the support of this partition
    int                 nNodes;     // the number of BDD nodes
    short               nSupp;      // the number of support variables
    short               iPart;      // the number of this partition
};

struct Extra_ImageVar_t_
{
    int                 iNum;       // the BDD index of this variable
    DdNode *            bParts;     // the partition numbers
    int                 nParts;     // the number of partitions
};

/*---------------------------------------------------------------------------*/
/* Type declarations                                                         */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Variable declarations                                                     */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Macro declarations                                                        */
/*---------------------------------------------------------------------------*/

/**AutomaticStart*************************************************************/


/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/

static Extra_ImagePart_t ** Extra_CreateParts( DdManager * dd,
    int nParts, DdNode ** pbParts, DdNode * bCare );
static Extra_ImageVar_t ** Extra_CreateVars( DdManager * dd,
    int nParts, Extra_ImagePart_t ** pParts,
    int nVars, DdNode ** pbVarsNs );
static Extra_ImageNode_t ** Extra_CreateNodes( DdManager * dd, 
    int nParts, Extra_ImagePart_t ** pParts, 
    int nVars,  Extra_ImageVar_t ** pVars );
static void Extra_DeleteParts_rec( Extra_ImageNode_t * pNode );
static int Extra_BuildTreeNode( DdManager * dd, 
    int nNodes, Extra_ImageNode_t ** pNodes, 
    int nVars,  Extra_ImageVar_t ** pVars );
static Extra_ImageNode_t * Extra_MergeTopNodes( DdManager * dd, 
    int nNodes, Extra_ImageNode_t ** pNodes );
static void Extra_bddImageTreeDelete_rec( Extra_ImageNode_t * pNode );
static void Extra_bddImageCompute_rec( Extra_ImageTree_t * pTree, Extra_ImageNode_t * pNode );
static int Extra_FindBestVariable( DdManager * dd,
    int nNodes, Extra_ImageNode_t ** pNodes, 
    int nVars,  Extra_ImageVar_t ** pVars );
static void Extra_FindBestPartitions( DdManager * dd, DdNode * bParts, 
    int nNodes, Extra_ImageNode_t ** pNodes, 
    int * piNode1, int * piNode2 );
static Extra_ImageNode_t * Extra_CombineTwoNodes( DdManager * dd, DdNode * bCube,
    Extra_ImageNode_t * pNode1, Extra_ImageNode_t * pNode2 );

static void Extra_bddImagePrintLatchDependency( DdManager * dd, DdNode * bCare,
    int nParts, DdNode ** pbParts,
    int nVars, DdNode ** pbVars );
static void Extra_bddImagePrintLatchDependencyOne( DdManager * dd, DdNode * bFunc, 
    DdNode * bVarsCs, DdNode * bVarsNs, int iPart );

static void Extra_bddImagePrintTree( Extra_ImageTree_t * pTree );
static void Extra_bddImagePrintTree_rec( Extra_ImageNode_t * pNode, int nOffset );


/**AutomaticEnd***************************************************************/


/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/

/**Function*************************************************************

  Synopsis    [Starts the image computation using tree-based scheduling.]

  Description [This procedure starts the image computation. It uses
  the given care set to test-run the image computation and creates the 
  quantification tree by scheduling variable quantifications. The tree can 
  be used to compute images for other care sets without rescheduling.
  In this case, Extra_bddImageCompute() should be called.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Extra_ImageTree_t * Extra_bddImageStart( 
    DdManager * dd, DdNode * bCare, // the care set
    int nParts, DdNode ** pbParts,  // the partitions for image computation
    int nVars, DdNode ** pbVars, int fVerbose )   // the NS and parameter variables (not quantified!)
{
    Extra_ImageTree_t * pTree;
    Extra_ImagePart_t ** pParts;
    Extra_ImageVar_t ** pVars;
    Extra_ImageNode_t ** pNodes;
    int v;

    if ( fVerbose && dd->size <= 80 )
        Extra_bddImagePrintLatchDependency( dd, bCare, nParts, pbParts, nVars, pbVars );

    // create variables, partitions and leaf nodes
    pParts = Extra_CreateParts( dd, nParts, pbParts, bCare );
    pVars  = Extra_CreateVars( dd, nParts + 1, pParts, nVars, pbVars );
    pNodes = Extra_CreateNodes( dd, nParts + 1, pParts, dd->size, pVars );
    
    // create the tree
    pTree = ABC_ALLOC( Extra_ImageTree_t, 1 );
    memset( pTree, 0, sizeof(Extra_ImageTree_t) );
    pTree->pCare = pNodes[nParts];
    pTree->fVerbose = fVerbose;

    // process the nodes
    while ( Extra_BuildTreeNode( dd, nParts + 1, pNodes, dd->size, pVars ) );

    // make sure the variables are gone
    for ( v = 0; v < dd->size; v++ )
        assert( pVars[v] == NULL );
    ABC_FREE( pVars );

    // merge the topmost nodes
    while ( (pTree->pRoot = Extra_MergeTopNodes( dd, nParts + 1, pNodes )) == NULL );

    // make sure the nodes are gone
    for ( v = 0; v < nParts + 1; v++ )
        assert( pNodes[v] == NULL );
    ABC_FREE( pNodes );

//    if ( fVerbose )
//        Extra_bddImagePrintTree( pTree );

    // set the support of the care set
    pTree->bCareSupp = Cudd_Support( dd, bCare );  Cudd_Ref( pTree->bCareSupp );

    // clean the partitions
    Extra_DeleteParts_rec( pTree->pRoot );
    ABC_FREE( pParts );
    return pTree;
}

/**Function*************************************************************

  Synopsis    [Compute the image.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Extra_bddImageCompute( Extra_ImageTree_t * pTree, DdNode * bCare )
{
    DdManager * dd = pTree->pCare->dd;
    DdNode * bSupp, * bRem;

    pTree->nIter++;

    // make sure the supports are okay
    bSupp = Cudd_Support( dd, bCare );        Cudd_Ref( bSupp );
    if ( bSupp != pTree->bCareSupp )
    {
        bRem = Cudd_bddExistAbstract( dd, bSupp, pTree->bCareSupp );  Cudd_Ref( bRem );
        if ( bRem != b1 )
        {
printf( "Original care set support: " );
ABC_PRB( dd, pTree->bCareSupp );
printf( "Current care set support: " );
ABC_PRB( dd, bSupp );
            Cudd_RecursiveDeref( dd, bSupp );
            Cudd_RecursiveDeref( dd, bRem );
            printf( "The care set depends on some vars that were not in the care set during scheduling.\n" );
            return NULL;
        }
        Cudd_RecursiveDeref( dd, bRem );
    }
    Cudd_RecursiveDeref( dd, bSupp );

    // remove the previous image
    Cudd_RecursiveDeref( dd, pTree->pCare->bImage );
    pTree->pCare->bImage = bCare;   Cudd_Ref( bCare );

    // compute the image
    pTree->nNodesMax = 0;
    Extra_bddImageCompute_rec( pTree, pTree->pRoot );
    if ( pTree->nNodesMaxT < pTree->nNodesMax )
        pTree->nNodesMaxT = pTree->nNodesMax;

//    if ( pTree->fVerbose )
//        printf( "Iter %2d : Max nodes = %5d.\n", pTree->nIter, pTree->nNodesMax );
    return pTree->pRoot->bImage;
}

/**Function*************************************************************

  Synopsis    [Delete the tree.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Extra_bddImageTreeDelete( Extra_ImageTree_t * pTree )
{
    if ( pTree->bCareSupp )
        Cudd_RecursiveDeref( pTree->pRoot->dd, pTree->bCareSupp );
    Extra_bddImageTreeDelete_rec( pTree->pRoot );
    ABC_FREE( pTree );
}

/**Function*************************************************************

  Synopsis    [Reads the image from the tree.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Extra_bddImageRead( Extra_ImageTree_t * pTree )
{
    return pTree->pRoot->bImage;
}

/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Definition of static Functions                                            */
/*---------------------------------------------------------------------------*/

/**Function*************************************************************

  Synopsis    [Creates partitions.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Extra_ImagePart_t ** Extra_CreateParts( DdManager * dd,
    int nParts, DdNode ** pbParts, DdNode * bCare )
{
    Extra_ImagePart_t ** pParts;
    int i;

    // start the partitions
    pParts = ABC_ALLOC( Extra_ImagePart_t *, nParts + 1 );
    // create structures for each variable
    for ( i = 0; i < nParts; i++ )
    {
        pParts[i] = ABC_ALLOC( Extra_ImagePart_t, 1 );
        pParts[i]->bFunc  = pbParts[i];                           Cudd_Ref( pParts[i]->bFunc );
        pParts[i]->bSupp  = Cudd_Support( dd, pParts[i]->bFunc ); Cudd_Ref( pParts[i]->bSupp );
        pParts[i]->nSupp  = Extra_bddSuppSize( dd, pParts[i]->bSupp );
        pParts[i]->nNodes = Cudd_DagSize( pParts[i]->bFunc );
        pParts[i]->iPart  = i;
    }
    // add the care set as the last partition
    pParts[nParts] = ABC_ALLOC( Extra_ImagePart_t, 1 );
    pParts[nParts]->bFunc = bCare;                                     Cudd_Ref( pParts[nParts]->bFunc );
    pParts[nParts]->bSupp = Cudd_Support( dd, pParts[nParts]->bFunc ); Cudd_Ref( pParts[nParts]->bSupp );
    pParts[nParts]->nSupp = Extra_bddSuppSize( dd, pParts[nParts]->bSupp );
    pParts[nParts]->nNodes = Cudd_DagSize( pParts[nParts]->bFunc );
    pParts[nParts]->iPart  = nParts;
    return pParts;
}

/**Function*************************************************************

  Synopsis    [Creates variables.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Extra_ImageVar_t ** Extra_CreateVars( DdManager * dd,
    int nParts, Extra_ImagePart_t ** pParts,
    int nVars, DdNode ** pbVars )
{
    Extra_ImageVar_t ** pVars;
    DdNode ** pbFuncs;
    DdNode * bCubeNs, * bSupp, * bParts, * bTemp, * bSuppTemp;
    int nVarsTotal, iVar, p, Counter;

    // put all the functions into one array
    pbFuncs = ABC_ALLOC( DdNode *, nParts );
    for ( p = 0; p < nParts; p++ )
        pbFuncs[p] = pParts[p]->bSupp;
    bSupp = Cudd_VectorSupport( dd, pbFuncs, nParts );  Cudd_Ref( bSupp );
    ABC_FREE( pbFuncs );

    // remove the NS vars
    bCubeNs = Cudd_bddComputeCube( dd, pbVars, NULL, nVars );        Cudd_Ref( bCubeNs );
    bSupp = Cudd_bddExistAbstract( dd, bTemp = bSupp, bCubeNs );     Cudd_Ref( bSupp );
    Cudd_RecursiveDeref( dd, bTemp );
    Cudd_RecursiveDeref( dd, bCubeNs );

    // get the number of I and CS variables to be quantified
    nVarsTotal = Extra_bddSuppSize( dd, bSupp );

    // start the variables
    pVars = ABC_ALLOC( Extra_ImageVar_t *, dd->size );
    memset( pVars, 0, sizeof(Extra_ImageVar_t *) * dd->size );
    // create structures for each variable
    for ( bSuppTemp = bSupp; bSuppTemp != b1; bSuppTemp = cuddT(bSuppTemp) )
    {
        iVar = bSuppTemp->index;
        pVars[iVar] = ABC_ALLOC( Extra_ImageVar_t, 1 );
        pVars[iVar]->iNum = iVar;
        // collect all the parts this var belongs to
        Counter = 0;
        bParts = b1; Cudd_Ref( bParts );
        for ( p = 0; p < nParts; p++ )
            if ( Cudd_bddLeq( dd, pParts[p]->bSupp, dd->vars[bSuppTemp->index] ) )
            {
                bParts = Cudd_bddAnd( dd, bTemp = bParts, dd->vars[p] );  Cudd_Ref( bParts );
                Cudd_RecursiveDeref( dd, bTemp );
                Counter++;
            }
        pVars[iVar]->bParts = bParts; // takes ref
        pVars[iVar]->nParts = Counter;
    }
    Cudd_RecursiveDeref( dd, bSupp );
    return pVars;
}

/**Function*************************************************************

  Synopsis    [Creates variables.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Extra_ImageNode_t ** Extra_CreateNodes( DdManager * dd, 
    int nParts, Extra_ImagePart_t ** pParts, 
    int nVars,  Extra_ImageVar_t ** pVars )
{
    Extra_ImageNode_t ** pNodes;
    Extra_ImageNode_t * pNode;
    DdNode * bTemp;
    int i, v, iPart;
/*
    DdManager *         dd;       // the manager 
    DdNode *            bCube;    // the cube to quantify
    DdNode *            bImage;   // the partial image
    Extra_ImageNode_t * pNode1;   // the first branch
    Extra_ImageNode_t * pNode2;   // the second branch
    Extra_ImagePart_t * pPart;    // the partition (temporary)
*/
    // start the partitions
    pNodes = ABC_ALLOC( Extra_ImageNode_t *, nParts );
    // create structures for each leaf nodes
    for ( i = 0; i < nParts; i++ )
    {
        pNodes[i] = ABC_ALLOC( Extra_ImageNode_t, 1 );
        memset( pNodes[i], 0, sizeof(Extra_ImageNode_t) );
        pNodes[i]->dd    = dd;
        pNodes[i]->pPart = pParts[i];
    }
    // find the quantification cubes for each leaf node
    for ( v = 0; v < nVars; v++ )
    {
        if ( pVars[v] == NULL )
            continue;
        assert( pVars[v]->nParts > 0 );
        if ( pVars[v]->nParts > 1 )
            continue;
        iPart = pVars[v]->bParts->index;
        if ( pNodes[iPart]->bCube == NULL )
        {
            pNodes[iPart]->bCube = dd->vars[v];   
            Cudd_Ref( dd->vars[v] );
        }
        else
        {
            pNodes[iPart]->bCube = Cudd_bddAnd( dd, bTemp = pNodes[iPart]->bCube, dd->vars[v] );  
            Cudd_Ref( pNodes[iPart]->bCube );
            Cudd_RecursiveDeref( dd, bTemp );
        }
        // remove these  variables
        Cudd_RecursiveDeref( dd, pVars[v]->bParts );
        ABC_FREE( pVars[v] );
    }

    // assign the leaf node images
    for ( i = 0; i < nParts; i++ )
    {
        pNode = pNodes[i];
        if ( pNode->bCube )
        {
            // update the partition
            pParts[i]->bFunc = Cudd_bddExistAbstract( dd, bTemp = pParts[i]->bFunc, pNode->bCube );
            Cudd_Ref( pParts[i]->bFunc );
            Cudd_RecursiveDeref( dd, bTemp );
            // update the support the partition
            pParts[i]->bSupp = Cudd_bddExistAbstract( dd, bTemp = pParts[i]->bSupp, pNode->bCube ); 
            Cudd_Ref( pParts[i]->bSupp );
            Cudd_RecursiveDeref( dd, bTemp );
            // update the numbers
            pParts[i]->nSupp  = Extra_bddSuppSize( dd, pParts[i]->bSupp );
            pParts[i]->nNodes = Cudd_DagSize( pParts[i]->bFunc );
            // get rid of the cube
            // save the last (care set) quantification cube
            if ( i < nParts - 1 )
            {
                Cudd_RecursiveDeref( dd, pNode->bCube );
                pNode->bCube = NULL;
            }
        }
        // copy the function
        pNode->bImage = pParts[i]->bFunc;   Cudd_Ref( pNode->bImage );
    }
/*
    for ( i = 0; i < nParts; i++ )
    {
        pNode = pNodes[i];
ABC_PRB( dd, pNode->bCube );
ABC_PRB( dd, pNode->pPart->bFunc );
ABC_PRB( dd, pNode->pPart->bSupp );
printf( "\n" );
    }
*/
    return pNodes;
}


/**Function*************************************************************

  Synopsis    [Delete the partitions from the nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Extra_DeleteParts_rec( Extra_ImageNode_t * pNode )
{
    Extra_ImagePart_t * pPart;
    if ( pNode->pNode1 )
        Extra_DeleteParts_rec( pNode->pNode1 );
    if ( pNode->pNode2 )
        Extra_DeleteParts_rec( pNode->pNode2 );
    pPart = pNode->pPart;
    Cudd_RecursiveDeref( pNode->dd, pPart->bFunc );
    Cudd_RecursiveDeref( pNode->dd, pPart->bSupp );
    ABC_FREE( pNode->pPart );
}

/**Function*************************************************************

  Synopsis    [Delete the partitions from the nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Extra_bddImageTreeDelete_rec( Extra_ImageNode_t * pNode )
{
    if ( pNode->pNode1 )
        Extra_bddImageTreeDelete_rec( pNode->pNode1 );
    if ( pNode->pNode2 )
        Extra_bddImageTreeDelete_rec( pNode->pNode2 );
    if ( pNode->bCube )
        Cudd_RecursiveDeref( pNode->dd, pNode->bCube );
    if ( pNode->bImage )
        Cudd_RecursiveDeref( pNode->dd, pNode->bImage );
    assert( pNode->pPart == NULL );
    ABC_FREE( pNode );
}

/**Function*************************************************************

  Synopsis    [Recompute the image.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Extra_bddImageCompute_rec( Extra_ImageTree_t * pTree, Extra_ImageNode_t * pNode )
{
    DdManager * dd = pNode->dd;
    DdNode * bTemp;
    int nNodes;

    // trivial case
    if ( pNode->pNode1 == NULL )
    {
        if ( pNode->bCube )
        {
            pNode->bImage = Cudd_bddExistAbstract( dd, bTemp = pNode->bImage, pNode->bCube ); 
            Cudd_Ref( pNode->bImage );
            Cudd_RecursiveDeref( dd, bTemp );
        }
        return;
    }

    // compute the children
    if ( pNode->pNode1 )
        Extra_bddImageCompute_rec( pTree, pNode->pNode1 );
    if ( pNode->pNode2 )
        Extra_bddImageCompute_rec( pTree, pNode->pNode2 );

    // clean the old image
    if ( pNode->bImage )
        Cudd_RecursiveDeref( dd, pNode->bImage );
    pNode->bImage = NULL;

    // compute the new image
    if ( pNode->bCube )
        pNode->bImage = Cudd_bddAndAbstract( dd, 
            pNode->pNode1->bImage, pNode->pNode2->bImage, pNode->bCube );
    else
        pNode->bImage = Cudd_bddAnd( dd, pNode->pNode1->bImage, pNode->pNode2->bImage );
    Cudd_Ref( pNode->bImage );

    if ( pTree->fVerbose )
    {
        nNodes = Cudd_DagSize( pNode->bImage );
        if ( pTree->nNodesMax < nNodes )
            pTree->nNodesMax = nNodes;
    }
}

/**Function*************************************************************

  Synopsis    [Builds the tree.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Extra_BuildTreeNode( DdManager * dd, 
    int nNodes, Extra_ImageNode_t ** pNodes, 
    int nVars,  Extra_ImageVar_t ** pVars )
{
    Extra_ImageNode_t * pNode1, * pNode2;
    Extra_ImageVar_t * pVar;
    Extra_ImageNode_t * pNode;
    DdNode * bCube, * bTemp, * bSuppTemp, * bParts;
    int iNode1, iNode2;
    int iVarBest, nSupp, v;

    // find the best variable
    iVarBest = Extra_FindBestVariable( dd, nNodes, pNodes, nVars, pVars );
    if ( iVarBest == -1 )
        return 0;

    pVar = pVars[iVarBest];

    // this var cannot appear in one partition only
    nSupp = Extra_bddSuppSize( dd, pVar->bParts );
    assert( nSupp == pVar->nParts );
    assert( nSupp != 1 );

    // if it appears in only two partitions, quantify it
    if ( pVar->nParts == 2 )
    {
        // get the nodes
        iNode1 = pVar->bParts->index;
        iNode2 = cuddT(pVar->bParts)->index;
        pNode1 = pNodes[iNode1];
        pNode2 = pNodes[iNode2];

        // get the quantification cube
        bCube = dd->vars[pVar->iNum];    Cudd_Ref( bCube );
        // add the variables that appear only in these partitions
        for ( v = 0; v < nVars; v++ )
            if ( pVars[v] && v != iVarBest && pVars[v]->bParts == pVars[iVarBest]->bParts )
            {
                // add this var
                bCube = Cudd_bddAnd( dd, bTemp = bCube, dd->vars[pVars[v]->iNum] );   Cudd_Ref( bCube );
                Cudd_RecursiveDeref( dd, bTemp );
                // clean this var
                Cudd_RecursiveDeref( dd, pVars[v]->bParts );
                ABC_FREE( pVars[v] );
            }
        // clean the best var
        Cudd_RecursiveDeref( dd, pVars[iVarBest]->bParts );
        ABC_FREE( pVars[iVarBest] );

        // combines two nodes
        pNode = Extra_CombineTwoNodes( dd, bCube, pNode1, pNode2 );
        Cudd_RecursiveDeref( dd, bCube );
    }
    else // if ( pVar->nParts > 2 )
    {
        // find two smallest BDDs that have this var
        Extra_FindBestPartitions( dd, pVar->bParts, nNodes, pNodes, &iNode1, &iNode2 );
        pNode1 = pNodes[iNode1];
        pNode2 = pNodes[iNode2];

        // it is not possible that a var appears only in these two
        // otherwise, it would have a different cost
        bParts = Cudd_bddAnd( dd, dd->vars[iNode1], dd->vars[iNode2] ); Cudd_Ref( bParts );
        for ( v = 0; v < nVars; v++ )
            if ( pVars[v] && pVars[v]->bParts == bParts )
                assert( 0 );
        Cudd_RecursiveDeref( dd, bParts );

        // combines two nodes
        pNode = Extra_CombineTwoNodes( dd, b1, pNode1, pNode2 );
    }

    // clean the old nodes
    pNodes[iNode1] = pNode;
    pNodes[iNode2] = NULL;
    
    // update the variables that appear in pNode[iNode2]
    for ( bSuppTemp = pNode2->pPart->bSupp; bSuppTemp != b1; bSuppTemp = cuddT(bSuppTemp) )
    {
        pVar = pVars[bSuppTemp->index];
        if ( pVar == NULL ) // this variable is not be quantified
            continue;
        // quantify this var
        assert( Cudd_bddLeq( dd, pVar->bParts, dd->vars[iNode2] ) );
        pVar->bParts = Cudd_bddExistAbstract( dd, bTemp = pVar->bParts, dd->vars[iNode2] ); Cudd_Ref( pVar->bParts );
        Cudd_RecursiveDeref( dd, bTemp );
        // add the new var
        pVar->bParts = Cudd_bddAnd( dd, bTemp = pVar->bParts, dd->vars[iNode1] ); Cudd_Ref( pVar->bParts );
        Cudd_RecursiveDeref( dd, bTemp );
        // update the score
        pVar->nParts = Extra_bddSuppSize( dd, pVar->bParts );
    }
    return 1;
}


/**Function*************************************************************

  Synopsis    [Merges the nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Extra_ImageNode_t * Extra_MergeTopNodes(
    DdManager * dd, int nNodes, Extra_ImageNode_t ** pNodes )
{
    Extra_ImageNode_t * pNode;
    int n1 = -1, n2 = -1, n;

    // find the first and the second non-empty spots
    for ( n = 0; n < nNodes; n++ )
        if ( pNodes[n] )
        {
            if ( n1 == -1 )
                n1 = n;
            else if ( n2 == -1 )
            {
                n2 = n;
                break;
            }
        }
    assert( n1 != -1 );
    // check the situation when only one such node is detected
    if ( n2 == -1 )
    {
        // save the node
        pNode = pNodes[n1];
        // clean the node
        pNodes[n1] = NULL;
        return pNode;
    }
  
    // combines two nodes
    pNode = Extra_CombineTwoNodes( dd, b1, pNodes[n1], pNodes[n2] );

    // clean the old nodes
    pNodes[n1] = pNode;
    pNodes[n2] = NULL;
    return NULL;
}

/**Function*************************************************************

  Synopsis    [Merges two nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Extra_ImageNode_t * Extra_CombineTwoNodes( DdManager * dd, DdNode * bCube,
    Extra_ImageNode_t * pNode1, Extra_ImageNode_t * pNode2 )
{
    Extra_ImageNode_t * pNode;
    Extra_ImagePart_t * pPart;

    // create a new partition
    pPart = ABC_ALLOC( Extra_ImagePart_t, 1 );
    memset( pPart, 0, sizeof(Extra_ImagePart_t) );
    // create the function
    pPart->bFunc = Cudd_bddAndAbstract( dd, pNode1->pPart->bFunc, pNode2->pPart->bFunc, bCube );
    Cudd_Ref( pPart->bFunc );
    // update the support the partition
    pPart->bSupp = Cudd_bddAndAbstract( dd, pNode1->pPart->bSupp, pNode2->pPart->bSupp, bCube );
    Cudd_Ref( pPart->bSupp );
    // update the numbers
    pPart->nSupp  = Extra_bddSuppSize( dd, pPart->bSupp );
    pPart->nNodes = Cudd_DagSize( pPart->bFunc );
    pPart->iPart = -1;
/*
ABC_PRB( dd, pNode1->pPart->bSupp );
ABC_PRB( dd, pNode2->pPart->bSupp );
ABC_PRB( dd, pPart->bSupp );
*/
    // create a new node
    pNode = ABC_ALLOC( Extra_ImageNode_t, 1 );
    memset( pNode, 0, sizeof(Extra_ImageNode_t) );
    pNode->dd     = dd;
    pNode->pPart  = pPart;
    pNode->pNode1 = pNode1;
    pNode->pNode2 = pNode2;
    // compute the image
    pNode->bImage = Cudd_bddAndAbstract( dd, pNode1->bImage, pNode2->bImage, bCube ); 
    Cudd_Ref( pNode->bImage );
    // save the cube
    if ( bCube != b1 )
    {
        pNode->bCube = bCube;   Cudd_Ref( bCube );
    }
    return pNode;
}

/**Function*************************************************************

  Synopsis    [Computes the best variable.]

  Description [The variables is the best if the sum of squares of the
  BDD sizes of the partitions, in which it participates, is the minimum.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Extra_FindBestVariable( DdManager * dd,
    int nNodes, Extra_ImageNode_t ** pNodes, 
    int nVars,  Extra_ImageVar_t ** pVars )
{
    DdNode * bTemp;
    int iVarBest, v;
    double CostBest, CostCur;

    CostBest = 100000000000000.0;
    iVarBest = -1;
    for ( v = 0; v < nVars; v++ )
        if ( pVars[v] )
        {
            CostCur = 0;
            for ( bTemp = pVars[v]->bParts; bTemp != b1; bTemp = cuddT(bTemp) )
                CostCur += pNodes[bTemp->index]->pPart->nNodes * 
                           pNodes[bTemp->index]->pPart->nNodes;
            if ( CostBest > CostCur )
            {
                CostBest = CostCur;
                iVarBest = v;
            }
        }
    return iVarBest;
}

/**Function*************************************************************

  Synopsis    [Computes two smallest partions that have this var.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Extra_FindBestPartitions( DdManager * dd, DdNode * bParts, 
    int nNodes, Extra_ImageNode_t ** pNodes, 
    int * piNode1, int * piNode2 )
{
    DdNode * bTemp;
    int iPart1, iPart2;
    int CostMin1, CostMin2, Cost;

    // go through the partitions
    iPart1 = iPart2 = -1;
    CostMin1 = CostMin2 = 1000000;
    for ( bTemp = bParts; bTemp != b1; bTemp = cuddT(bTemp) )
    {
        Cost = pNodes[bTemp->index]->pPart->nNodes;
        if ( CostMin1 > Cost )
        {
            CostMin2 = CostMin1;    iPart2 = iPart1;
            CostMin1 = Cost;        iPart1 = bTemp->index;
        }
        else if ( CostMin2 > Cost )
        {
            CostMin2 = Cost;        iPart2 = bTemp->index;
        }
    }

    *piNode1 = iPart1;
    *piNode2 = iPart2;
}

/**Function*************************************************************

  Synopsis    [Prints the latch dependency matrix.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Extra_bddImagePrintLatchDependency( 
    DdManager * dd, DdNode * bCare, // the care set
    int nParts, DdNode ** pbParts,  // the partitions for image computation
    int nVars, DdNode ** pbVars )   // the NS and parameter variables (not quantified!)
{
    int i;
    DdNode * bVarsCs, * bVarsNs;

    bVarsCs = Cudd_Support( dd, bCare );                       Cudd_Ref( bVarsCs );
    bVarsNs = Cudd_bddComputeCube( dd, pbVars, NULL, nVars );  Cudd_Ref( bVarsNs );

    printf( "The latch dependency matrix:\n" );
    printf( "Partitions = %d   Variables: total = %d  non-quantifiable = %d\n",
        nParts, dd->size, nVars );
    printf( "     : " );
    for ( i = 0; i < dd->size; i++ )
        printf( "%d", i % 10 );
    printf( "\n" );

    for ( i = 0; i < nParts; i++ )
        Extra_bddImagePrintLatchDependencyOne( dd, pbParts[i], bVarsCs, bVarsNs, i );
    Extra_bddImagePrintLatchDependencyOne( dd, bCare, bVarsCs, bVarsNs, nParts );

    Cudd_RecursiveDeref( dd, bVarsCs );
    Cudd_RecursiveDeref( dd, bVarsNs );
}

/**Function*************************************************************

  Synopsis    [Prints one row of the latch dependency matrix.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Extra_bddImagePrintLatchDependencyOne(
    DdManager * dd, DdNode * bFunc,      // the function
    DdNode * bVarsCs, DdNode * bVarsNs,  // the current/next state vars
    int iPart )
{
    DdNode * bSupport;
    int v;
    bSupport = Cudd_Support( dd, bFunc );  Cudd_Ref( bSupport );
    printf( " %3d : ", iPart );
    for ( v = 0; v < dd->size; v++ )
    {
        if ( Cudd_bddLeq( dd, bSupport, dd->vars[v] ) )
        {
            if ( Cudd_bddLeq( dd, bVarsCs, dd->vars[v] ) )
                printf( "c" );
            else if ( Cudd_bddLeq( dd, bVarsNs, dd->vars[v] ) ) 
                printf( "n" );
            else
                printf( "i" );
        }
        else
            printf( "." );
    }
    printf( "\n" );
    Cudd_RecursiveDeref( dd, bSupport );
}


/**Function*************************************************************

  Synopsis    [Prints the tree for quenstification scheduling.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Extra_bddImagePrintTree( Extra_ImageTree_t * pTree )
{
    printf( "The quantification scheduling tree:\n" );
    Extra_bddImagePrintTree_rec( pTree->pRoot, 1 );
}

/**Function*************************************************************

  Synopsis    [Prints the tree for quenstification scheduling.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Extra_bddImagePrintTree_rec( Extra_ImageNode_t * pNode, int Offset )
{
    DdNode * Cube;
    int i;

    Cube = pNode->bCube;

    if ( pNode->pNode1 == NULL )
    {
        printf( "<%d> ", pNode->pPart->iPart );
        if ( Cube != NULL )
        {
            ABC_PRB( pNode->dd, Cube );
        }
        else
            printf( "\n" );
        return;
    }

    printf( "<*> " );
    if ( Cube != NULL )
    {
        ABC_PRB( pNode->dd, Cube );
    }
    else
        printf( "\n" );

    for ( i = 0; i < Offset; i++ )
        printf( "    " );
    Extra_bddImagePrintTree_rec( pNode->pNode1, Offset + 1 );

    for ( i = 0; i < Offset; i++ )
        printf( "    " );
    Extra_bddImagePrintTree_rec( pNode->pNode2, Offset + 1 );
}





struct Extra_ImageTree2_t_
{
    DdManager * dd;
    DdNode *    bRel;
    DdNode *    bCube;
    DdNode *    bImage;
};

/**Function*************************************************************

  Synopsis    [Starts the monolithic image computation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Extra_ImageTree2_t * Extra_bddImageStart2( 
    DdManager * dd, DdNode * bCare,
    int nParts, DdNode ** pbParts,
    int nVars, DdNode ** pbVars, int fVerbose )
{
    Extra_ImageTree2_t * pTree;
    DdNode * bCubeAll, * bCubeNot, * bTemp;
    int i;

    pTree = ABC_ALLOC( Extra_ImageTree2_t, 1 );
    pTree->dd = dd;
    pTree->bImage = NULL;

    bCubeAll = Extra_bddComputeCube( dd, dd->vars, dd->size );      Cudd_Ref( bCubeAll );
    bCubeNot = Extra_bddComputeCube( dd, pbVars,   nVars );         Cudd_Ref( bCubeNot );
    pTree->bCube = Cudd_bddExistAbstract( dd, bCubeAll, bCubeNot ); Cudd_Ref( pTree->bCube );
    Cudd_RecursiveDeref( dd, bCubeAll );
    Cudd_RecursiveDeref( dd, bCubeNot );

    // derive the monolithic relation
    pTree->bRel = b1;   Cudd_Ref( pTree->bRel );
    for ( i = 0; i < nParts; i++ )
    {
        pTree->bRel = Cudd_bddAnd( dd, bTemp = pTree->bRel, pbParts[i] ); Cudd_Ref( pTree->bRel );
        Cudd_RecursiveDeref( dd, bTemp );
    }
    Extra_bddImageCompute2( pTree, bCare );
    return pTree;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Extra_bddImageCompute2( Extra_ImageTree2_t * pTree, DdNode * bCare )
{
    if ( pTree->bImage )
        Cudd_RecursiveDeref( pTree->dd, pTree->bImage );
    pTree->bImage = Cudd_bddAndAbstract( pTree->dd, pTree->bRel, bCare, pTree->bCube ); 
    Cudd_Ref( pTree->bImage );
    return pTree->bImage;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Extra_bddImageTreeDelete2( Extra_ImageTree2_t * pTree )
{
    if ( pTree->bRel )
        Cudd_RecursiveDeref( pTree->dd, pTree->bRel );
    if ( pTree->bCube )
        Cudd_RecursiveDeref( pTree->dd, pTree->bCube );
    if ( pTree->bImage )
        Cudd_RecursiveDeref( pTree->dd, pTree->bImage );
    ABC_FREE( pTree );
}

/**Function*************************************************************

  Synopsis    [Returns the previously computed image.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Extra_bddImageRead2( Extra_ImageTree2_t * pTree )
{
    return pTree->bImage;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

