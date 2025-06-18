/**CFile****************************************************************

  FileName    [bbrImage.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [BDD-based reachability analysis.]

  Synopsis    [Performs image computation.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: bbrImage.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "bbr.h"
#include "bdd/mtr/mtr.h"

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

typedef struct Bbr_ImageNode_t_  Bbr_ImageNode_t;
typedef struct Bbr_ImagePart_t_  Bbr_ImagePart_t;
typedef struct Bbr_ImageVar_t_   Bbr_ImageVar_t;

struct Bbr_ImageTree_t_
{
    Bbr_ImageNode_t *   pRoot;      // the root of quantification tree
    Bbr_ImageNode_t *   pCare;      // the leaf node with the care set
    DdNode *            bCareSupp;  // the cube to quantify from the care
    int                 fVerbose;   // the verbosity flag
    int                 nNodesMax;  // the max number of nodes in one iter
    int                 nNodesMaxT; // the overall max number of nodes
    int                 nIter;      // the number of iterations with this tree
    int                 nBddMax;    // the number of node to stop
};

struct Bbr_ImageNode_t_
{
    DdManager *         dd;         // the manager 
    DdNode *            bCube;      // the cube to quantify
    DdNode *            bImage;     // the partial image
    Bbr_ImageNode_t *   pNode1;     // the first branch
    Bbr_ImageNode_t *   pNode2;     // the second branch
    Bbr_ImagePart_t *   pPart;      // the partition (temporary)
};

struct Bbr_ImagePart_t_
{
    DdNode *            bFunc;      // the partition
    DdNode *            bSupp;      // the support of this partition
    int                 nNodes;     // the number of BDD nodes
    short               nSupp;      // the number of support variables
    short               iPart;      // the number of this partition
};

struct Bbr_ImageVar_t_
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

#define     b0     Cudd_Not((dd)->one)
#define     b1              (dd)->one

#ifndef ABC_PRB
#define ABC_PRB(dd,f)       printf("%s = ", #f); Bbr_bddPrint(dd,f); printf("\n")
#endif

/**AutomaticStart*************************************************************/


/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/

static Bbr_ImagePart_t ** Bbr_CreateParts( DdManager * dd,
    int nParts, DdNode ** pbParts, DdNode * bCare );
static Bbr_ImageVar_t ** Bbr_CreateVars( DdManager * dd,
    int nParts, Bbr_ImagePart_t ** pParts,
    int nVars, DdNode ** pbVarsNs );
static Bbr_ImageNode_t ** Bbr_CreateNodes( DdManager * dd, 
    int nParts, Bbr_ImagePart_t ** pParts, 
    int nVars,  Bbr_ImageVar_t ** pVars );
static void Bbr_DeleteParts_rec( Bbr_ImageNode_t * pNode );
static int Bbr_BuildTreeNode( DdManager * dd, 
    int nNodes, Bbr_ImageNode_t ** pNodes, 
    int nVars,  Bbr_ImageVar_t ** pVars, int * pfStop, int nBddMax );
static Bbr_ImageNode_t * Bbr_MergeTopNodes( DdManager * dd, 
    int nNodes, Bbr_ImageNode_t ** pNodes );
static void Bbr_bddImageTreeDelete_rec( Bbr_ImageNode_t * pNode );
static int Bbr_bddImageCompute_rec( Bbr_ImageTree_t * pTree, Bbr_ImageNode_t * pNode );
static int Bbr_FindBestVariable( DdManager * dd,
    int nNodes, Bbr_ImageNode_t ** pNodes, 
    int nVars,  Bbr_ImageVar_t ** pVars );
static void Bbr_FindBestPartitions( DdManager * dd, DdNode * bParts, 
    int nNodes, Bbr_ImageNode_t ** pNodes, 
    int * piNode1, int * piNode2 );
static Bbr_ImageNode_t * Bbr_CombineTwoNodes( DdManager * dd, DdNode * bCube,
    Bbr_ImageNode_t * pNode1, Bbr_ImageNode_t * pNode2 );

static void Bbr_bddImagePrintLatchDependency( DdManager * dd, DdNode * bCare,
    int nParts, DdNode ** pbParts,
    int nVars, DdNode ** pbVars );
static void Bbr_bddImagePrintLatchDependencyOne( DdManager * dd, DdNode * bFunc, 
    DdNode * bVarsCs, DdNode * bVarsNs, int iPart );

static void Bbr_bddImagePrintTree( Bbr_ImageTree_t * pTree );
static void Bbr_bddImagePrintTree_rec( Bbr_ImageNode_t * pNode, int nOffset );

static void Bbr_bddPrint( DdManager * dd, DdNode * F );

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
  In this case, Bbr_bddImageCompute() should be called.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Bbr_ImageTree_t * Bbr_bddImageStart( 
    DdManager * dd, DdNode * bCare, // the care set
    int nParts, DdNode ** pbParts,  // the partitions for image computation
    int nVars, DdNode ** pbVars, int nBddMax, int fVerbose )   // the NS and parameter variables (not quantified!)
{
    Bbr_ImageTree_t * pTree;
    Bbr_ImagePart_t ** pParts;
    Bbr_ImageVar_t ** pVars;
    Bbr_ImageNode_t ** pNodes, * pCare;
    int fStop, v;

    if ( fVerbose && dd->size <= 80 )
        Bbr_bddImagePrintLatchDependency( dd, bCare, nParts, pbParts, nVars, pbVars );

    // create variables, partitions and leaf nodes
    pParts = Bbr_CreateParts( dd, nParts, pbParts, bCare );
    pVars  = Bbr_CreateVars( dd, nParts + 1, pParts, nVars, pbVars );
    pNodes = Bbr_CreateNodes( dd, nParts + 1, pParts, dd->size, pVars );
    pCare  = pNodes[nParts];

    // process the nodes
    while ( Bbr_BuildTreeNode( dd, nParts + 1, pNodes, dd->size, pVars, &fStop, nBddMax ) );

    // consider the case of BDD node blowup
    if ( fStop )
    {
        for ( v = 0; v < dd->size; v++ )
            if ( pVars[v] )
                ABC_FREE( pVars[v] );
        ABC_FREE( pVars );
        for ( v = 0; v <= nParts; v++ )
            if ( pNodes[v] )
            {
                Bbr_DeleteParts_rec( pNodes[v] );
                Bbr_bddImageTreeDelete_rec( pNodes[v] );
            }
        ABC_FREE( pNodes );
        ABC_FREE( pParts );
        return NULL;
    }

    // make sure the variables are gone
    for ( v = 0; v < dd->size; v++ )
        assert( pVars[v] == NULL );
    ABC_FREE( pVars );
    
    // create the tree
    pTree = ABC_ALLOC( Bbr_ImageTree_t, 1 );
    memset( pTree, 0, sizeof(Bbr_ImageTree_t) );
    pTree->pCare = pCare;
    pTree->nBddMax = nBddMax;
    pTree->fVerbose = fVerbose;

    // merge the topmost nodes
    while ( (pTree->pRoot = Bbr_MergeTopNodes( dd, nParts + 1, pNodes )) == NULL );

    // make sure the nodes are gone
    for ( v = 0; v < nParts + 1; v++ )
        assert( pNodes[v] == NULL );
    ABC_FREE( pNodes );

//    if ( fVerbose )
//        Bbr_bddImagePrintTree( pTree );

    // set the support of the care set
    pTree->bCareSupp = Cudd_Support( dd, bCare );  Cudd_Ref( pTree->bCareSupp );

    // clean the partitions
    Bbr_DeleteParts_rec( pTree->pRoot );
    ABC_FREE( pParts );

    return pTree;
}

/**Function*************************************************************

  Synopsis    [Compute the image.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Bbr_bddImageCompute( Bbr_ImageTree_t * pTree, DdNode * bCare )
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
    if ( !Bbr_bddImageCompute_rec( pTree, pTree->pRoot ) )
        return NULL;
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
void Bbr_bddImageTreeDelete( Bbr_ImageTree_t * pTree )
{
    if ( pTree->bCareSupp )
        Cudd_RecursiveDeref( pTree->pRoot->dd, pTree->bCareSupp );
    Bbr_bddImageTreeDelete_rec( pTree->pRoot );
    ABC_FREE( pTree );
}

/**Function*************************************************************

  Synopsis    [Reads the image from the tree.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Bbr_bddImageRead( Bbr_ImageTree_t * pTree )
{
    return pTree->pRoot->bImage;
}

/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis    [Outputs the BDD in a readable format.]

  Description []

  SideEffects [None]

  SeeAlso     []

******************************************************************************/
void Bbr_bddPrint( DdManager * dd, DdNode * F )
{
    DdGen * Gen;
    int * Cube;
    CUDD_VALUE_TYPE Value;
    int nVars = dd->size;
    int fFirstCube = 1;
    int i;

    if ( F == NULL )
    {
        printf("NULL");
        return;
    }
    if ( F == b0 )
    {
        printf("Constant 0");
        return;
    }
    if ( F == b1 )
    {
        printf("Constant 1");
        return;
    }

    Cudd_ForeachCube( dd, F, Gen, Cube, Value )
    {
        if ( fFirstCube )
            fFirstCube = 0;
        else
//          Output << " + ";
            printf( " + " );

        for ( i = 0; i < nVars; i++ )
            if ( Cube[i] == 0 )
                printf( "[%d]'", i );
//              printf( "%c'", (char)('a'+i) );
            else if ( Cube[i] == 1 )
                printf( "[%d]", i );
//              printf( "%c", (char)('a'+i) );
    }

//  printf("\n");
}

/*---------------------------------------------------------------------------*/
/* Definition of static Functions                                            */
/*---------------------------------------------------------------------------*/

/**Function*************************************************************

  Synopsis    [Creates partitions.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Bbr_ImagePart_t ** Bbr_CreateParts( DdManager * dd,
    int nParts, DdNode ** pbParts, DdNode * bCare )
{
    Bbr_ImagePart_t ** pParts;
    int i;

    // start the partitions
    pParts = ABC_ALLOC( Bbr_ImagePart_t *, nParts + 1 );
    // create structures for each variable
    for ( i = 0; i < nParts; i++ )
    {
        pParts[i] = ABC_ALLOC( Bbr_ImagePart_t, 1 );
        pParts[i]->bFunc  = pbParts[i];                           Cudd_Ref( pParts[i]->bFunc );
        pParts[i]->bSupp  = Cudd_Support( dd, pParts[i]->bFunc ); Cudd_Ref( pParts[i]->bSupp );
        pParts[i]->nSupp  = Cudd_SupportSize( dd, pParts[i]->bSupp );
        pParts[i]->nNodes = Cudd_DagSize( pParts[i]->bFunc );
        pParts[i]->iPart  = i;
    }
    // add the care set as the last partition
    pParts[nParts] = ABC_ALLOC( Bbr_ImagePart_t, 1 );
    pParts[nParts]->bFunc = bCare;                                     Cudd_Ref( pParts[nParts]->bFunc );
    pParts[nParts]->bSupp = Cudd_Support( dd, pParts[nParts]->bFunc ); Cudd_Ref( pParts[nParts]->bSupp );
    pParts[nParts]->nSupp = Cudd_SupportSize( dd, pParts[nParts]->bSupp );
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
Bbr_ImageVar_t ** Bbr_CreateVars( DdManager * dd,
    int nParts, Bbr_ImagePart_t ** pParts,
    int nVars, DdNode ** pbVars )
{
    Bbr_ImageVar_t ** pVars;
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
    nVarsTotal = Cudd_SupportSize( dd, bSupp );

    // start the variables
    pVars = ABC_ALLOC( Bbr_ImageVar_t *, dd->size );
    memset( pVars, 0, sizeof(Bbr_ImageVar_t *) * dd->size );
    // create structures for each variable
    for ( bSuppTemp = bSupp; bSuppTemp != b1; bSuppTemp = cuddT(bSuppTemp) )
    {
        iVar = bSuppTemp->index;
        pVars[iVar] = ABC_ALLOC( Bbr_ImageVar_t, 1 );
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
Bbr_ImageNode_t ** Bbr_CreateNodes( DdManager * dd, 
    int nParts, Bbr_ImagePart_t ** pParts, 
    int nVars,  Bbr_ImageVar_t ** pVars )
{
    Bbr_ImageNode_t ** pNodes;
    Bbr_ImageNode_t * pNode;
    DdNode * bTemp;
    int i, v, iPart;
/*
    DdManager *         dd;       // the manager 
    DdNode *            bCube;    // the cube to quantify
    DdNode *            bImage;   // the partial image
    Bbr_ImageNode_t * pNode1;   // the first branch
    Bbr_ImageNode_t * pNode2;   // the second branch
    Bbr_ImagePart_t * pPart;    // the partition (temporary)
*/
    // start the partitions
    pNodes = ABC_ALLOC( Bbr_ImageNode_t *, nParts );
    // create structures for each leaf nodes
    for ( i = 0; i < nParts; i++ )
    {
        pNodes[i] = ABC_ALLOC( Bbr_ImageNode_t, 1 );
        memset( pNodes[i], 0, sizeof(Bbr_ImageNode_t) );
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
            pParts[i]->nSupp  = Cudd_SupportSize( dd, pParts[i]->bSupp );
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
void Bbr_DeleteParts_rec( Bbr_ImageNode_t * pNode )
{
    Bbr_ImagePart_t * pPart;
    if ( pNode->pNode1 )
        Bbr_DeleteParts_rec( pNode->pNode1 );
    if ( pNode->pNode2 )
        Bbr_DeleteParts_rec( pNode->pNode2 );
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
void Bbr_bddImageTreeDelete_rec( Bbr_ImageNode_t * pNode )
{
    if ( pNode->pNode1 )
        Bbr_bddImageTreeDelete_rec( pNode->pNode1 );
    if ( pNode->pNode2 )
        Bbr_bddImageTreeDelete_rec( pNode->pNode2 );
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
int Bbr_bddImageCompute_rec( Bbr_ImageTree_t * pTree, Bbr_ImageNode_t * pNode )
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
        return 1;
    }

    // compute the children
    if ( pNode->pNode1 )
        if ( !Bbr_bddImageCompute_rec( pTree, pNode->pNode1 ) )
            return 0;
    if ( pNode->pNode2 )
        if ( !Bbr_bddImageCompute_rec( pTree, pNode->pNode2 ) )
            return 0;

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
    if ( dd->keys-dd->dead > (unsigned)pTree->nBddMax )
        return 0;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Builds the tree.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Bbr_BuildTreeNode( DdManager * dd, 
    int nNodes, Bbr_ImageNode_t ** pNodes, 
    int nVars,  Bbr_ImageVar_t ** pVars, int * pfStop, int nBddMax )
{
    Bbr_ImageNode_t * pNode1, * pNode2;
    Bbr_ImageVar_t * pVar;
    Bbr_ImageNode_t * pNode;
    DdNode * bCube, * bTemp, * bSuppTemp;//, * bParts;
    int iNode1, iNode2;
    int iVarBest, nSupp, v;

    // find the best variable
    iVarBest = Bbr_FindBestVariable( dd, nNodes, pNodes, nVars, pVars );
    if ( iVarBest == -1 )
        return 0;
/*
for ( v = 0; v < nVars; v++ )
{
    DdNode * bSupp;
    if ( pVars[v] == NULL )
        continue;
    printf( "%3d :", v );
    printf( "%3d ", pVars[v]->nParts );
    bSupp = Cudd_Support( dd, pVars[v]->bParts );  Cudd_Ref( bSupp );
    Bbr_bddPrint( dd, bSupp ); printf( "\n" );
    Cudd_RecursiveDeref( dd, bSupp );
}
*/
    pVar = pVars[iVarBest];

    // this var cannot appear in one partition only
    nSupp = Cudd_SupportSize( dd, pVar->bParts );
    assert( nSupp == pVar->nParts );
    assert( nSupp != 1 );
//printf( "var = %d  supp = %d\n\n", iVarBest, nSupp );

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
        pNode = Bbr_CombineTwoNodes( dd, bCube, pNode1, pNode2 );
        Cudd_RecursiveDeref( dd, bCube );
    }
    else // if ( pVar->nParts > 2 )
    {
        // find two smallest BDDs that have this var
        Bbr_FindBestPartitions( dd, pVar->bParts, nNodes, pNodes, &iNode1, &iNode2 );
        pNode1 = pNodes[iNode1];
        pNode2 = pNodes[iNode2];
//printf( "smallest bdds with this var: %d %d\n", iNode1, iNode2 );
/*
        // it is not possible that a var appears only in these two
        // otherwise, it would have a different cost
        bParts = Cudd_bddAnd( dd, dd->vars[iNode1], dd->vars[iNode2] ); Cudd_Ref( bParts );
        for ( v = 0; v < nVars; v++ )
            if ( pVars[v] && pVars[v]->bParts == bParts )
                assert( 0 );
        Cudd_RecursiveDeref( dd, bParts );
*/
        // combines two nodes
        pNode = Bbr_CombineTwoNodes( dd, b1, pNode1, pNode2 );
    }

    // clean the old nodes
    pNodes[iNode1] = pNode;
    pNodes[iNode2] = NULL;
//printf( "Removing node %d (leaving node %d)\n", iNode2, iNode1 );
    
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
        pVar->nParts = Cudd_SupportSize( dd, pVar->bParts );
    }

    *pfStop = 0;
    if ( dd->keys-dd->dead > (unsigned)nBddMax )
    {
        *pfStop = 1;
        return 0;
    }
    return 1;
}


/**Function*************************************************************

  Synopsis    [Merges the nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Bbr_ImageNode_t * Bbr_MergeTopNodes(
    DdManager * dd, int nNodes, Bbr_ImageNode_t ** pNodes )
{
    Bbr_ImageNode_t * pNode;
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
    pNode = Bbr_CombineTwoNodes( dd, b1, pNodes[n1], pNodes[n2] );

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
Bbr_ImageNode_t * Bbr_CombineTwoNodes( DdManager * dd, DdNode * bCube,
    Bbr_ImageNode_t * pNode1, Bbr_ImageNode_t * pNode2 )
{
    Bbr_ImageNode_t * pNode;
    Bbr_ImagePart_t * pPart;

    // create a new partition
    pPart = ABC_ALLOC( Bbr_ImagePart_t, 1 );
    memset( pPart, 0, sizeof(Bbr_ImagePart_t) );
    // create the function
    pPart->bFunc = Cudd_bddAndAbstract( dd, pNode1->pPart->bFunc, pNode2->pPart->bFunc, bCube );
    Cudd_Ref( pPart->bFunc );
    // update the support the partition
    pPart->bSupp = Cudd_bddAndAbstract( dd, pNode1->pPart->bSupp, pNode2->pPart->bSupp, bCube );
    Cudd_Ref( pPart->bSupp );
    // update the numbers
    pPart->nSupp  = Cudd_SupportSize( dd, pPart->bSupp );
    pPart->nNodes = Cudd_DagSize( pPart->bFunc );
    pPart->iPart = -1;
/*
ABC_PRB( dd, pNode1->pPart->bSupp );
ABC_PRB( dd, pNode2->pPart->bSupp );
ABC_PRB( dd, pPart->bSupp );
*/
    // create a new node
    pNode = ABC_ALLOC( Bbr_ImageNode_t, 1 );
    memset( pNode, 0, sizeof(Bbr_ImageNode_t) );
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
int Bbr_FindBestVariable( DdManager * dd,
    int nNodes, Bbr_ImageNode_t ** pNodes, 
    int nVars,  Bbr_ImageVar_t ** pVars )
{
    DdNode * bTemp;
    int iVarBest, v;
    double CostBest, CostCur;

    CostBest = 100000000000000.0;
    iVarBest = -1;

    // check if there are two-variable partitions
    for ( v = 0; v < nVars; v++ )
        if ( pVars[v] && pVars[v]->nParts == 2 )
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
    if ( iVarBest >= 0 )
        return iVarBest;

    // find other partition
    for ( v = 0; v < nVars; v++ )
        if ( pVars[v] )
        {
            assert( pVars[v]->nParts > 1 );
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
void Bbr_FindBestPartitions( DdManager * dd, DdNode * bParts, 
    int nNodes, Bbr_ImageNode_t ** pNodes, 
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
void Bbr_bddImagePrintLatchDependency( 
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
        Bbr_bddImagePrintLatchDependencyOne( dd, pbParts[i], bVarsCs, bVarsNs, i );
    Bbr_bddImagePrintLatchDependencyOne( dd, bCare, bVarsCs, bVarsNs, nParts );

    Cudd_RecursiveDeref( dd, bVarsCs );
    Cudd_RecursiveDeref( dd, bVarsNs );
}

/**Function*************************************************************

  Synopsis    [Prints one row of the latch dependency matrix.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Bbr_bddImagePrintLatchDependencyOne(
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
void Bbr_bddImagePrintTree( Bbr_ImageTree_t * pTree )
{
    printf( "The quantification scheduling tree:\n" );
    Bbr_bddImagePrintTree_rec( pTree->pRoot, 1 );
}

/**Function*************************************************************

  Synopsis    [Prints the tree for quenstification scheduling.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Bbr_bddImagePrintTree_rec( Bbr_ImageNode_t * pNode, int Offset )
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
    Bbr_bddImagePrintTree_rec( pNode->pNode1, Offset + 1 );

    for ( i = 0; i < Offset; i++ )
        printf( "    " );
    Bbr_bddImagePrintTree_rec( pNode->pNode2, Offset + 1 );
}

/**Function********************************************************************

  Synopsis    [Computes the positive polarty cube composed of the first vars in the array.]

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
DdNode * Bbr_bddComputeCube( DdManager * dd, DdNode ** bXVars, int nVars )
{
    DdNode * bRes;
    DdNode * bTemp;
    int i;

    bRes = b1; Cudd_Ref( bRes );
    for ( i = 0; i < nVars; i++ )
    {
        bRes = Cudd_bddAnd( dd, bTemp = bRes, bXVars[i] );  Cudd_Ref( bRes );
        Cudd_RecursiveDeref( dd, bTemp );
    }

    Cudd_Deref( bRes );
    return bRes;
}





struct Bbr_ImageTree2_t_
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
Bbr_ImageTree2_t * Bbr_bddImageStart2( 
    DdManager * dd, DdNode * bCare,
    int nParts, DdNode ** pbParts,
    int nVars, DdNode ** pbVars, int fVerbose )
{
    Bbr_ImageTree2_t * pTree;
    DdNode * bCubeAll, * bCubeNot, * bTemp;
    int i;

    pTree = ABC_ALLOC( Bbr_ImageTree2_t, 1 );
    pTree->dd = dd;
    pTree->bImage = NULL;

    bCubeAll = Bbr_bddComputeCube( dd, dd->vars, dd->size );      Cudd_Ref( bCubeAll );
    bCubeNot = Bbr_bddComputeCube( dd, pbVars,   nVars );         Cudd_Ref( bCubeNot );
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
    Bbr_bddImageCompute2( pTree, bCare );
    return pTree;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Bbr_bddImageCompute2( Bbr_ImageTree2_t * pTree, DdNode * bCare )
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
void Bbr_bddImageTreeDelete2( Bbr_ImageTree2_t * pTree )
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
DdNode * Bbr_bddImageRead2( Bbr_ImageTree2_t * pTree )
{
    return pTree->bImage;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

