/**CFile****************************************************************

  FileName    [dsdTree.c]

  PackageName [DSD: Disjoint-support decomposition package.]

  Synopsis    [Managing the decomposition tree.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 8.0. Started - September 22, 2003.]

  Revision    [$Id: dsdTree.c,v 1.0 2002/22/09 00:00:00 alanmi Exp $]

***********************************************************************/

#include "dsdInt.h"
#include "misc/util/utilTruth.h"
#include "opt/dau/dau.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

static void Dsd_TreeUnmark_rec( Dsd_Node_t * pNode );
static void Dsd_TreeGetInfo_rec( Dsd_Node_t * pNode, int RankCur );
static int  Dsd_TreeCountNonTerminalNodes_rec( Dsd_Node_t * pNode );
static int  Dsd_TreeCountPrimeNodes_rec( Dsd_Node_t * pNode );
static int  Dsd_TreeCollectDecomposableVars_rec( DdManager * dd, Dsd_Node_t * pNode, int * pVars, int * nVars );
static void Dsd_TreeCollectNodesDfs_rec( Dsd_Node_t * pNode, Dsd_Node_t * ppNodes[], int * pnNodes );
static void Dsd_TreePrint_rec( FILE * pFile, Dsd_Node_t * pNode, int fCcmp, char * pInputNames[], char * pOutputName, int nOffset, int * pSigCounter, int fShortNames );
static void Dsd_NodePrint_rec( FILE * pFile, Dsd_Node_t * pNode, int fComp, char * pOutputName, int nOffset, int * pSigCounter );

////////////////////////////////////////////////////////////////////////
///                      STATIC VARIABLES                            ///
////////////////////////////////////////////////////////////////////////

static int s_DepthMax;
static int s_GateSizeMax;

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Create the DSD node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Dsd_Node_t * Dsd_TreeNodeCreate( int Type, int nDecs, int BlockNum )
{
    // allocate memory for this node 
    Dsd_Node_t * p = (Dsd_Node_t *) ABC_ALLOC( char, sizeof(Dsd_Node_t) );
    memset( p, 0, sizeof(Dsd_Node_t) );
    p->Type       = (Dsd_Type_t)Type;       // the type of this block
    p->nDecs      = nDecs;                  // the number of decompositions
    if ( p->nDecs )
    {
        p->pDecs      = (Dsd_Node_t **) ABC_ALLOC( char, p->nDecs * sizeof(Dsd_Node_t *) );
        p->pDecs[0]   = NULL;
    }
    return p;
}

/**Function*************************************************************

  Synopsis    [Frees the DSD node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Dsd_TreeNodeDelete( DdManager * dd, Dsd_Node_t * pNode )
{
    if ( pNode->G )  Cudd_RecursiveDeref( dd, pNode->G );
    if ( pNode->S )  Cudd_RecursiveDeref( dd, pNode->S );
    ABC_FREE( pNode->pDecs );
    ABC_FREE( pNode );
}

/**Function*************************************************************

  Synopsis    [Unmarks the decomposition tree.]

  Description [This function assumes that originally pNode->nVisits are 
  set to zero!]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Dsd_TreeUnmark( Dsd_Manager_t * pDsdMan )
{
    int i;
    for ( i = 0; i < pDsdMan->nRoots; i++ )
        Dsd_TreeUnmark_rec( Dsd_Regular( pDsdMan->pRoots[i] ) );
}


/**Function*************************************************************

  Synopsis    [Recursive unmarking.]

  Description [This function should be called with a non-complemented 
  pointer.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Dsd_TreeUnmark_rec( Dsd_Node_t * pNode )
{
    int i;

    assert( pNode );
    assert( !Dsd_IsComplement( pNode ) );
    assert( pNode->nVisits > 0 );

    if ( --pNode->nVisits ) // if this is not the last visit, return
        return;

    // upon the last visit, go through the list of successors and call recursively 
    if ( pNode->Type != DSD_NODE_BUF && pNode->Type != DSD_NODE_CONST1 )
    for ( i = 0; i < pNode->nDecs; i++ )
        Dsd_TreeUnmark_rec( Dsd_Regular(pNode->pDecs[i]) );
}

/**Function*************************************************************

  Synopsis    [Getting information about the node.]

  Description [This function computes the max depth and the max gate size 
  of the tree rooted at the node.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Dsd_TreeNodeGetInfo( Dsd_Manager_t * pDsdMan, int * DepthMax, int * GateSizeMax )
{
    int i;
    s_DepthMax    = 0;
    s_GateSizeMax = 0;

    for ( i = 0; i < pDsdMan->nRoots; i++ )
        Dsd_TreeGetInfo_rec( Dsd_Regular( pDsdMan->pRoots[i] ), 0 );

    if ( DepthMax ) 
        *DepthMax     = s_DepthMax;
    if ( GateSizeMax ) 
        *GateSizeMax  = s_GateSizeMax;
}

/**Function*************************************************************

  Synopsis    [Getting information about the node.]

  Description [This function computes the max depth and the max gate size 
  of the tree rooted at the node.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Dsd_TreeNodeGetInfoOne( Dsd_Node_t * pNode, int * DepthMax, int * GateSizeMax )
{
    s_DepthMax    = 0;
    s_GateSizeMax = 0;

    Dsd_TreeGetInfo_rec( Dsd_Regular(pNode), 0 );

    if ( DepthMax ) 
        *DepthMax     = s_DepthMax;
    if ( GateSizeMax ) 
        *GateSizeMax  = s_GateSizeMax;
}


/**Function*************************************************************

  Synopsis    [Performs the recursive step of Dsd_TreeNodeGetInfo().]

  Description [pNode is the node, for the tree rooted in which we are 
  determining info. RankCur is the current rank to assign to the node.
  fSetRank is the flag saying whether the rank will be written in the 
  node. s_DepthMax is the maximum depths of the tree. s_GateSizeMax is 
  the maximum gate size.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Dsd_TreeGetInfo_rec( Dsd_Node_t * pNode, int RankCur )
{
    int i;
    int GateSize;

    assert( pNode );
    assert( !Dsd_IsComplement( pNode ) );
    assert( pNode->nVisits >= 0 );

    // we don't want the two-input gates to count for non-decomposable blocks
    if ( pNode->Type == DSD_NODE_OR  || 
         pNode->Type == DSD_NODE_EXOR )
        GateSize = 2;
    else
        GateSize = pNode->nDecs;

    // update the max size of the node
    if ( s_GateSizeMax < GateSize )
         s_GateSizeMax = GateSize;

    if ( pNode->nDecs < 2 )
        return;

    // update the max rank
    if ( s_DepthMax < RankCur+1 )
         s_DepthMax = RankCur+1;

    // call recursively
    for ( i = 0; i < pNode->nDecs; i++ )
        Dsd_TreeGetInfo_rec( Dsd_Regular(pNode->pDecs[i]), RankCur+1 );
}

/**Function*************************************************************

  Synopsis    [Counts AIG nodes needed to implement this node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Dsd_TreeGetAigCost_rec( Dsd_Node_t * pNode )
{
    int i, Counter = 0;

    assert( pNode );
    assert( !Dsd_IsComplement( pNode ) );
    assert( pNode->nVisits >= 0 );

    if ( pNode->nDecs < 2 )
        return 0;

    // we don't want the two-input gates to count for non-decomposable blocks
    if ( pNode->Type == DSD_NODE_OR )
        Counter += pNode->nDecs - 1;
    else if ( pNode->Type == DSD_NODE_EXOR )
        Counter += 3*(pNode->nDecs - 1);
    else if ( pNode->Type == DSD_NODE_PRIME && pNode->nDecs == 3 )
        Counter += 3;

    // call recursively
    for ( i = 0; i < pNode->nDecs; i++ )
        Counter += Dsd_TreeGetAigCost_rec( Dsd_Regular(pNode->pDecs[i]) );
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Counts AIG nodes needed to implement this node.]

  Description [Assumes that the only primes of the DSD tree are MUXes.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Dsd_TreeGetAigCost( Dsd_Node_t * pNode )
{
    return Dsd_TreeGetAigCost_rec( Dsd_Regular(pNode) );
}

/**Function*************************************************************

  Synopsis    [Counts non-terminal nodes of the DSD tree.]

  Description [Nonterminal nodes include all the nodes with the
  support more than 1. These are OR, EXOR, and PRIME nodes. They
  do not include the elementary variable nodes and the constant 1
  node.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Dsd_TreeCountNonTerminalNodes( Dsd_Manager_t * pDsdMan )
{
    int Counter, i;
    Counter = 0;
    for ( i = 0; i < pDsdMan->nRoots; i++ )
        Counter += Dsd_TreeCountNonTerminalNodes_rec( Dsd_Regular( pDsdMan->pRoots[i] ) );
    Dsd_TreeUnmark( pDsdMan );
    return Counter;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Dsd_TreeCountNonTerminalNodesOne( Dsd_Node_t * pRoot )
{
    int Counter = 0;

    // go through the list of successors and call recursively 
    Counter = Dsd_TreeCountNonTerminalNodes_rec( Dsd_Regular(pRoot) );

    Dsd_TreeUnmark_rec( Dsd_Regular(pRoot) );
    return Counter;
}


/**Function*************************************************************

  Synopsis    [Counts non-terminal nodes for one root.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Dsd_TreeCountNonTerminalNodes_rec( Dsd_Node_t * pNode )
{
    int i;
    int Counter = 0;

    assert( pNode );
    assert( !Dsd_IsComplement( pNode ) );
    assert( pNode->nVisits >= 0 );

    if ( pNode->nVisits++ ) // if this is not the first visit, return zero
        return 0;

    if ( pNode->nDecs <= 1 )
        return 0;

    // upon the first visit, go through the list of successors and call recursively 
    for ( i = 0; i < pNode->nDecs; i++ )
        Counter += Dsd_TreeCountNonTerminalNodes_rec( Dsd_Regular(pNode->pDecs[i]) );

    return Counter + 1;
}


/**Function*************************************************************

  Synopsis    [Counts prime nodes of the DSD tree.]

  Description [Prime nodes are nodes with the support more than 2,
  that is not an OR or EXOR gate.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Dsd_TreeCountPrimeNodes( Dsd_Manager_t * pDsdMan )
{
    int Counter, i;
    Counter = 0;
    for ( i = 0; i < pDsdMan->nRoots; i++ )
        Counter += Dsd_TreeCountPrimeNodes_rec( Dsd_Regular( pDsdMan->pRoots[i] ) );
    Dsd_TreeUnmark( pDsdMan );
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Counts prime nodes for one root.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Dsd_TreeCountPrimeNodesOne( Dsd_Node_t * pRoot )
{
    int Counter = 0;

    // go through the list of successors and call recursively 
    Counter = Dsd_TreeCountPrimeNodes_rec( Dsd_Regular(pRoot) );

    Dsd_TreeUnmark_rec( Dsd_Regular(pRoot) );
    return Counter;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Dsd_TreeCountPrimeNodes_rec( Dsd_Node_t * pNode )
{
    int i;
    int Counter = 0;

    assert( pNode );
    assert( !Dsd_IsComplement( pNode ) );
    assert( pNode->nVisits >= 0 );

    if ( pNode->nVisits++ ) // if this is not the first visit, return zero
        return 0;

    if ( pNode->nDecs <= 1 )
        return 0;

    // upon the first visit, go through the list of successors and call recursively 
    for ( i = 0; i < pNode->nDecs; i++ )
        Counter += Dsd_TreeCountPrimeNodes_rec( Dsd_Regular(pNode->pDecs[i]) );

    if ( pNode->Type == DSD_NODE_PRIME )
        Counter++;

    return Counter;
}


/**Function*************************************************************

  Synopsis    [Collects the decomposable vars on the PI side.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Dsd_TreeCollectDecomposableVars( Dsd_Manager_t * pDsdMan, int * pVars )
{
    int nVars;

    // set the vars collected to 0
    nVars = 0;
    Dsd_TreeCollectDecomposableVars_rec( pDsdMan->dd, Dsd_Regular(pDsdMan->pRoots[0]), pVars, &nVars );
    // return the number of collected vars
    return nVars;
}

/**Function*************************************************************

  Synopsis    [Implements the recursive part of Dsd_TreeCollectDecomposableVars().]

  Description [Adds decomposable variables as they are found to pVars and increments 
  nVars. Returns 1 if a non-dec node with more than 4 inputs was encountered 
  in the processed subtree. Returns 0, otherwise. ]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Dsd_TreeCollectDecomposableVars_rec( DdManager * dd, Dsd_Node_t * pNode, int * pVars, int * nVars )
{
    int fSkipThisNode, i;
    Dsd_Node_t * pTemp;
    int fVerbose = 0;

    assert( pNode );
    assert( !Dsd_IsComplement( pNode ) );

    if ( pNode->nDecs <= 1 )
        return 0;

    // go through the list of successors and call recursively 
    fSkipThisNode = 0;
    for ( i = 0; i < pNode->nDecs; i++ )
        if ( Dsd_TreeCollectDecomposableVars_rec(dd, Dsd_Regular(pNode->pDecs[i]), pVars, nVars) )
            fSkipThisNode = 1;

    if ( !fSkipThisNode && (pNode->Type == DSD_NODE_OR || pNode->Type == DSD_NODE_EXOR || pNode->nDecs <= 4) )
    {
if ( fVerbose )
printf( "Node of type <%d> (OR=6,EXOR=8,RAND=1): ", pNode->Type );

        for ( i = 0; i < pNode->nDecs; i++ )
        {
            pTemp = Dsd_Regular(pNode->pDecs[i]);
            if ( pTemp->Type == DSD_NODE_BUF )
            {
                if ( pVars )
                    pVars[ (*nVars)++ ] = pTemp->S->index;
                else
                    (*nVars)++;
                    
if ( fVerbose )
printf( "%d ", pTemp->S->index );
            }
        }
if ( fVerbose )
printf( "\n" );
    }
    else
        fSkipThisNode = 1;


    return fSkipThisNode;
}


/**Function*************************************************************

  Synopsis    [Creates the DFS ordered array of DSD nodes in the tree.]

  Description [The collected nodes do not include the terminal nodes
  and the constant 1 node. The array of nodes is returned. The number
  of entries in the array is returned in the variale pnNodes.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Dsd_Node_t ** Dsd_TreeCollectNodesDfs( Dsd_Manager_t * pDsdMan, int * pnNodes )
{
    Dsd_Node_t ** ppNodes;
    int nNodes, nNodesAlloc;
    int i;

    nNodesAlloc = Dsd_TreeCountNonTerminalNodes(pDsdMan);
    nNodes  = 0;
    ppNodes = ABC_ALLOC( Dsd_Node_t *, nNodesAlloc );
    for ( i = 0; i < pDsdMan->nRoots; i++ )
        Dsd_TreeCollectNodesDfs_rec( Dsd_Regular(pDsdMan->pRoots[i]), ppNodes, &nNodes );
    Dsd_TreeUnmark( pDsdMan );
    assert( nNodesAlloc == nNodes );
    *pnNodes = nNodes;
    return ppNodes;
}

/**Function*************************************************************

  Synopsis    [Creates the DFS ordered array of DSD nodes in the tree.]

  Description [The collected nodes do not include the terminal nodes
  and the constant 1 node. The array of nodes is returned. The number
  of entries in the array is returned in the variale pnNodes.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Dsd_Node_t ** Dsd_TreeCollectNodesDfsOne( Dsd_Manager_t * pDsdMan, Dsd_Node_t * pNode, int * pnNodes )
{
    Dsd_Node_t ** ppNodes;
    int nNodes, nNodesAlloc;
    nNodesAlloc = Dsd_TreeCountNonTerminalNodesOne(pNode);
    nNodes  = 0;
    ppNodes = ABC_ALLOC( Dsd_Node_t *, nNodesAlloc );
    Dsd_TreeCollectNodesDfs_rec( Dsd_Regular(pNode), ppNodes, &nNodes );
    Dsd_TreeUnmark_rec(Dsd_Regular(pNode));
    assert( nNodesAlloc == nNodes );
    *pnNodes = nNodes;
    return ppNodes;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Dsd_TreeCollectNodesDfs_rec( Dsd_Node_t * pNode, Dsd_Node_t * ppNodes[], int * pnNodes )
{
    int i;
    assert( pNode );
    assert( !Dsd_IsComplement(pNode) );
    assert( pNode->nVisits >= 0 );

    if ( pNode->nVisits++ ) // if this is not the first visit, return zero
        return;
    if ( pNode->nDecs <= 1 )
        return;

    // upon the first visit, go through the list of successors and call recursively 
    for ( i = 0; i < pNode->nDecs; i++ )
        Dsd_TreeCollectNodesDfs_rec( Dsd_Regular(pNode->pDecs[i]), ppNodes, pnNodes );

    ppNodes[ (*pnNodes)++ ] = pNode;
}

/**Function*************************************************************

  Synopsis    [Prints the decompostion tree into file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Dsd_TreePrint( FILE * pFile, Dsd_Manager_t * pDsdMan, char * pInputNames[], char * pOutputNames[], int fShortNames, int Output )
{
    Dsd_Node_t * pNode;
    int SigCounter;
    int i;
    SigCounter = 1;

    if ( Output == -1 )
    {
        for ( i = 0; i < pDsdMan->nRoots; i++ )
        {
            pNode = Dsd_Regular( pDsdMan->pRoots[i] );
            Dsd_TreePrint_rec( pFile, pNode, (pNode != pDsdMan->pRoots[i]), pInputNames, pOutputNames[i], 0, &SigCounter, fShortNames );
        }
    }
    else
    {
        assert( Output >= 0 && Output < pDsdMan->nRoots );
        pNode = Dsd_Regular( pDsdMan->pRoots[Output] );
        Dsd_TreePrint_rec( pFile, pNode, (pNode != pDsdMan->pRoots[Output]), pInputNames, pOutputNames[Output], 0, &SigCounter, fShortNames );
    }
}

/**Function*************************************************************

  Synopsis    [Prints the decompostion tree into file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Dsd_TreePrint_rec( FILE * pFile, Dsd_Node_t * pNode, int fComp, char * pInputNames[], char * pOutputName, int nOffset, int * pSigCounter, int fShortNames )
{
    char Buffer[100];
    Dsd_Node_t * pInput;
    int * pInputNums;
    int fCompNew, i;

    assert( pNode->Type == DSD_NODE_BUF || pNode->Type == DSD_NODE_CONST1 || 
        pNode->Type == DSD_NODE_PRIME || pNode->Type == DSD_NODE_OR || pNode->Type == DSD_NODE_EXOR ); 

    Extra_PrintSymbols( pFile, ' ', nOffset, 0 );
    if ( !fComp )
        fprintf( pFile, "%s = ", pOutputName );
    else
        fprintf( pFile, "NOT(%s) = ", pOutputName );
    pInputNums = ABC_ALLOC( int, pNode->nDecs );
    if ( pNode->Type == DSD_NODE_CONST1 )
    {
        fprintf( pFile, " Constant 1.\n" );
    }
    else if ( pNode->Type == DSD_NODE_BUF )
    {
        if ( fShortNames )
            fprintf( pFile, "%d", 'a' + pNode->S->index );
        else
            fprintf( pFile, "%s", pInputNames[pNode->S->index] );
        fprintf( pFile, "\n" );
    }
    else if ( pNode->Type == DSD_NODE_PRIME )
    {
        // print the line
        fprintf( pFile, "PRIME(" );
        for ( i = 0; i < pNode->nDecs; i++ )
        {
            pInput   = Dsd_Regular( pNode->pDecs[i] );
            fCompNew = (int)( pInput != pNode->pDecs[i] );
            if ( i )
                fprintf( pFile, "," );
            if ( fCompNew )
                fprintf( pFile, " NOT(" );
            else
                fprintf( pFile, " " );
            if ( pInput->Type == DSD_NODE_BUF )
            {
                pInputNums[i] = 0;
                if ( fShortNames )
                    fprintf( pFile, "%d", pInput->S->index );
                else
                    fprintf( pFile, "%s", pInputNames[pInput->S->index] );
            }
            else
            {
                pInputNums[i] = (*pSigCounter)++;
                fprintf( pFile, "<%d>", pInputNums[i] );
            }
            if ( fCompNew )
                fprintf( pFile, ")" );
        }
        fprintf( pFile, " )\n" );
        // call recursively for the following blocks
        for ( i = 0; i < pNode->nDecs; i++ )
            if ( pInputNums[i] )
            {
                pInput   = Dsd_Regular( pNode->pDecs[i] );
                sprintf( Buffer, "<%d>", pInputNums[i] );
                Dsd_TreePrint_rec( pFile, Dsd_Regular( pNode->pDecs[i] ), 0, pInputNames, Buffer, nOffset + 6, pSigCounter, fShortNames );
            }
    }
    else if ( pNode->Type == DSD_NODE_OR )
    {
        // print the line
        fprintf( pFile, "OR(" );
        for ( i = 0; i < pNode->nDecs; i++ )
        {
            pInput = Dsd_Regular( pNode->pDecs[i] );
            fCompNew  = (int)( pInput != pNode->pDecs[i] );
            if ( i )
                fprintf( pFile, "," );
            if ( fCompNew )
                fprintf( pFile, " NOT(" );
            else
                fprintf( pFile, " " );
            if ( pInput->Type == DSD_NODE_BUF )
            {
                pInputNums[i] = 0;
                if ( fShortNames )
                    fprintf( pFile, "%c", 'a' + pInput->S->index );
                else
                    fprintf( pFile, "%s", pInputNames[pInput->S->index] );
            }
            else
            {
                pInputNums[i] = (*pSigCounter)++;
                fprintf( pFile, "<%d>", pInputNums[i] );
            }
            if ( fCompNew )
                fprintf( pFile, ")" );
        }
        fprintf( pFile, " )\n" );
        // call recursively for the following blocks
        for ( i = 0; i < pNode->nDecs; i++ )
            if ( pInputNums[i] )
            {
                pInput = Dsd_Regular( pNode->pDecs[i] );
                sprintf( Buffer, "<%d>", pInputNums[i] );
                Dsd_TreePrint_rec( pFile, Dsd_Regular( pNode->pDecs[i] ), 0, pInputNames, Buffer, nOffset + 6, pSigCounter, fShortNames );
            }
    }
    else if ( pNode->Type == DSD_NODE_EXOR )
    {
        // print the line
        fprintf( pFile, "EXOR(" );
        for ( i = 0; i < pNode->nDecs; i++ )
        {
            pInput = Dsd_Regular( pNode->pDecs[i] );
            fCompNew  = (int)( pInput != pNode->pDecs[i] );
            if ( i )
                fprintf( pFile, "," );
            if ( fCompNew )
                fprintf( pFile, " NOT(" );
            else
                fprintf( pFile, " " );
            if ( pInput->Type == DSD_NODE_BUF )
            {
                pInputNums[i] = 0;
                if ( fShortNames )
                    fprintf( pFile, "%c", 'a' + pInput->S->index );
                else
                    fprintf( pFile, "%s", pInputNames[pInput->S->index] );
            }
            else
            {
                pInputNums[i] = (*pSigCounter)++;
                fprintf( pFile, "<%d>", pInputNums[i] );
            }
            if ( fCompNew )
                fprintf( pFile, ")" );
        }
        fprintf( pFile, " )\n" );
        // call recursively for the following blocks
        for ( i = 0; i < pNode->nDecs; i++ )
            if ( pInputNums[i] )
            {
                pInput = Dsd_Regular( pNode->pDecs[i] );
                sprintf( Buffer, "<%d>", pInputNums[i] );
                Dsd_TreePrint_rec( pFile, Dsd_Regular( pNode->pDecs[i] ), 0, pInputNames, Buffer, nOffset + 6, pSigCounter, fShortNames );
            }
    }
    ABC_FREE( pInputNums );
}

/**Function*************************************************************

  Synopsis    [Prints the decompostion tree into file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
word Dsd_TreeFunc2Truth_rec( DdManager * dd, DdNode * bFunc )
{
    word Cof0, Cof1;
    int Level;
    if ( bFunc == b0 )
        return 0;
    if ( bFunc == b1 )
        return ~(word)0;
    if ( Cudd_IsComplement(bFunc) )
        return ~Dsd_TreeFunc2Truth_rec( dd, Cudd_Not(bFunc) );
    Level = dd->perm[bFunc->index];
    assert( Level >= 0 && Level < 6 );
    Cof0 = Dsd_TreeFunc2Truth_rec( dd, cuddE(bFunc) );
    Cof1 = Dsd_TreeFunc2Truth_rec( dd, cuddT(bFunc) );
    return (s_Truths6[Level] & Cof1) | (~s_Truths6[Level] & Cof0);
}
void Dsd_TreePrint2_rec( FILE * pFile, DdManager * dd, Dsd_Node_t * pNode, int fComp, char * pInputNames[] )
{
    int i;
    if ( pNode->Type == DSD_NODE_CONST1 )
    {
        fprintf( pFile, "Const%d", !fComp );
        return;
    }
    assert( pNode->Type == DSD_NODE_BUF || pNode->Type == DSD_NODE_PRIME || pNode->Type == DSD_NODE_OR || pNode->Type == DSD_NODE_EXOR ); 
//    fprintf( pFile, "%s", (fComp ^ (pNode->Type == DSD_NODE_OR))? "!" : "" );
    if ( pNode->Type == DSD_NODE_BUF )
    {
        fprintf( pFile, "%s", fComp? "!" : "" );
        fprintf( pFile, "%s", pInputNames[pNode->S->index] );
    }
    else if ( pNode->Type == DSD_NODE_PRIME )
    {
        fprintf( pFile, " " );
        if ( pNode->nDecs <= 6 )
        {
            char pCanonPerm[6]; int uCanonPhase;
            // compute truth table
            DdNode * bFunc = Dsd_TreeGetPrimeFunction( dd, pNode );  
            word uTruth = Dsd_TreeFunc2Truth_rec( dd, bFunc );
            Cudd_Ref( bFunc );
            Cudd_RecursiveDeref( dd, bFunc );
            // canonicize truth table
            uCanonPhase = Abc_TtCanonicize( &uTruth, pNode->nDecs, pCanonPerm );
            fprintf( pFile, "%s", (fComp ^ ((uCanonPhase >> pNode->nDecs) & 1)) ? "!" : "" );
            Abc_TtPrintHexRev( pFile, &uTruth, pNode->nDecs );
            fprintf( pFile, "{" );
            for ( i = 0; i < pNode->nDecs; i++ )
            {
                Dsd_Node_t * pInput = pNode->pDecs[(int)pCanonPerm[i]];
                Dsd_TreePrint2_rec( pFile, dd, Dsd_Regular(pInput), Dsd_IsComplement(pInput) ^ ((uCanonPhase>>i)&1), pInputNames );
            }
            fprintf( pFile, "} " );
        }
        else
        {
            fprintf( pFile, "|%d|", pNode->nDecs );
            fprintf( pFile, "{" );
            for ( i = 0; i < pNode->nDecs; i++ )
                Dsd_TreePrint2_rec( pFile, dd, Dsd_Regular(pNode->pDecs[i]), Dsd_IsComplement(pNode->pDecs[i]), pInputNames );
            fprintf( pFile, "} " );
        }
    }
    else if ( pNode->Type == DSD_NODE_OR )
    {
        fprintf( pFile, "%s", !fComp? "!" : "" );
        fprintf( pFile, "(" );
        for ( i = 0; i < pNode->nDecs; i++ )
            Dsd_TreePrint2_rec( pFile, dd, Dsd_Regular(pNode->pDecs[i]), !Dsd_IsComplement(pNode->pDecs[i]), pInputNames );
        fprintf( pFile, ")" );
    }
    else if ( pNode->Type == DSD_NODE_EXOR )
    {
        fprintf( pFile, "%s", fComp? "!" : "" );
        fprintf( pFile, "[" );
        for ( i = 0; i < pNode->nDecs; i++ )
            Dsd_TreePrint2_rec( pFile, dd, Dsd_Regular(pNode->pDecs[i]), Dsd_IsComplement(pNode->pDecs[i]), pInputNames );
        fprintf( pFile, "]" );
    }
}
void Dsd_TreePrint2( FILE * pFile, Dsd_Manager_t * pDsdMan, char * pInputNames[], char * pOutputNames[], int Output )
{
    if ( Output == -1 )
    {
        int i;
        for ( i = 0; i < pDsdMan->nRoots; i++ )
        {
            fprintf( pFile, "%8s = ", pOutputNames[i] );
            Dsd_TreePrint2_rec( pFile, pDsdMan->dd, Dsd_Regular(pDsdMan->pRoots[i]), Dsd_IsComplement(pDsdMan->pRoots[i]), pInputNames );
            fprintf( pFile, "\n" );
        }
    }
    else
    {
        assert( Output >= 0 && Output < pDsdMan->nRoots );
        fprintf( pFile, "%8s = ", pOutputNames[Output] );
        Dsd_TreePrint2_rec( pFile, pDsdMan->dd, Dsd_Regular(pDsdMan->pRoots[Output]), Dsd_IsComplement(pDsdMan->pRoots[Output]), pInputNames );
        fprintf( pFile, "\n" );
    }
}

/**Function*************************************************************

  Synopsis    [Prints the decompostion tree into file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Dsd_NodePrint( FILE * pFile, Dsd_Node_t * pNode )
{
    Dsd_Node_t * pNodeR;
    int SigCounter = 1;
    pNodeR = Dsd_Regular(pNode);
    Dsd_NodePrint_rec( pFile, pNodeR, pNodeR != pNode, "F", 0, &SigCounter );
}

/**Function*************************************************************

  Synopsis    [Prints one node of the decomposition tree.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Dsd_NodePrint_rec( FILE * pFile, Dsd_Node_t * pNode, int fComp, char * pOutputName, int nOffset, int * pSigCounter )
{
    char Buffer[100];
    Dsd_Node_t * pInput;
    int * pInputNums;
    int fCompNew, i;

    assert( pNode->Type == DSD_NODE_BUF || pNode->Type == DSD_NODE_CONST1 || 
        pNode->Type == DSD_NODE_PRIME || pNode->Type == DSD_NODE_OR || pNode->Type == DSD_NODE_EXOR ); 

    Extra_PrintSymbols( pFile, ' ', nOffset, 0 );
    if ( !fComp )
        fprintf( pFile, "%s = ", pOutputName );
    else
        fprintf( pFile, "NOT(%s) = ", pOutputName );
    pInputNums = ABC_ALLOC( int, pNode->nDecs );
    if ( pNode->Type == DSD_NODE_CONST1 )
    {
        fprintf( pFile, " Constant 1.\n" );
    }
    else if ( pNode->Type == DSD_NODE_BUF )
    {
        fprintf( pFile, " " );
        fprintf( pFile, "%c", 'a' + pNode->S->index );
        fprintf( pFile, "\n" );
    }
    else if ( pNode->Type == DSD_NODE_PRIME )
    {
        // print the line
        fprintf( pFile, "PRIME(" );
        for ( i = 0; i < pNode->nDecs; i++ )
        {
            pInput   = Dsd_Regular( pNode->pDecs[i] );
            fCompNew = (int)( pInput != pNode->pDecs[i] );
            assert( fCompNew == 0 );
            if ( i )
                fprintf( pFile, "," );
            if ( pInput->Type == DSD_NODE_BUF )
            {
                pInputNums[i] = 0;
                fprintf( pFile, " %c", 'a' + pInput->S->index );
            }
            else
            {
                pInputNums[i] = (*pSigCounter)++;
                fprintf( pFile, " <%d>", pInputNums[i] );
            }
            if ( fCompNew )
                fprintf( pFile, "\'" );
        }
        fprintf( pFile, " )\n" );
/*
        fprintf( pFile, " )  " );  
        {
            DdNode * bLocal;
            bLocal = Dsd_TreeGetPrimeFunction( dd, pNodeDsd );  Cudd_Ref( bLocal );
            Extra_bddPrint( dd, bLocal );
            Cudd_RecursiveDeref( dd, bLocal );
        }
*/
        // call recursively for the following blocks
        for ( i = 0; i < pNode->nDecs; i++ )
            if ( pInputNums[i] )
            {
                pInput   = Dsd_Regular( pNode->pDecs[i] );
                sprintf( Buffer, "<%d>", pInputNums[i] );
                Dsd_NodePrint_rec( pFile, Dsd_Regular( pNode->pDecs[i] ), 0, Buffer, nOffset + 6, pSigCounter );
            }
    }
    else if ( pNode->Type == DSD_NODE_OR )
    {
        // print the line
        fprintf( pFile, "OR(" );
        for ( i = 0; i < pNode->nDecs; i++ )
        {
            pInput = Dsd_Regular( pNode->pDecs[i] );
            fCompNew  = (int)( pInput != pNode->pDecs[i] );
            if ( i )
                fprintf( pFile, "," );
            if ( pInput->Type == DSD_NODE_BUF )
            {
                pInputNums[i] = 0;
                fprintf( pFile, " %c", 'a' + pInput->S->index );
            }
            else
            {
                pInputNums[i] = (*pSigCounter)++;
                fprintf( pFile, " <%d>", pInputNums[i] );
            }
            if ( fCompNew )
                fprintf( pFile, "\'" );
        }
        fprintf( pFile, " )\n" );
        // call recursively for the following blocks
        for ( i = 0; i < pNode->nDecs; i++ )
            if ( pInputNums[i] )
            {
                pInput = Dsd_Regular( pNode->pDecs[i] );
                sprintf( Buffer, "<%d>", pInputNums[i] );
                Dsd_NodePrint_rec( pFile, Dsd_Regular( pNode->pDecs[i] ), 0, Buffer, nOffset + 6, pSigCounter );
            }
    }
    else if ( pNode->Type == DSD_NODE_EXOR )
    {
        // print the line
        fprintf( pFile, "EXOR(" );
        for ( i = 0; i < pNode->nDecs; i++ )
        {
            pInput = Dsd_Regular( pNode->pDecs[i] );
            fCompNew  = (int)( pInput != pNode->pDecs[i] );
            assert( fCompNew == 0 );
            if ( i )
                fprintf( pFile, "," );
            if ( pInput->Type == DSD_NODE_BUF )
            {
                pInputNums[i] = 0;
                fprintf( pFile, " %c", 'a' + pInput->S->index );
            }
            else
            {
                pInputNums[i] = (*pSigCounter)++;
                fprintf( pFile, " <%d>", pInputNums[i] );
            }
            if ( fCompNew )
                fprintf( pFile, "\'" );
        }
        fprintf( pFile, " )\n" );
        // call recursively for the following blocks
        for ( i = 0; i < pNode->nDecs; i++ )
            if ( pInputNums[i] )
            {
                pInput = Dsd_Regular( pNode->pDecs[i] );
                sprintf( Buffer, "<%d>", pInputNums[i] );
                Dsd_NodePrint_rec( pFile, Dsd_Regular( pNode->pDecs[i] ), 0, Buffer, nOffset + 6, pSigCounter );
            }
    }
    ABC_FREE( pInputNums );
}


/**Function*************************************************************

  Synopsis    [Retuns the function of one node of the decomposition tree.]

  Description [This is the old procedure. It is now superceded by the
  procedure Dsd_TreeGetPrimeFunction() found in "dsdLocal.c".]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Dsd_TreeGetPrimeFunctionOld( DdManager * dd, Dsd_Node_t * pNode, int fRemap ) 
{
    DdNode * bCof0,  * bCof1, * bCube0, * bCube1, * bNewFunc, * bTemp;
    int i;
    static int Permute[MAXINPUTS];

    assert( pNode );
    assert( !Dsd_IsComplement( pNode ) );
    assert( pNode->Type == DSD_NODE_PRIME );

    // transform the function of this block to depend on inputs
    // corresponding to the formal inputs

    // first, substitute those inputs that have some blocks associated with them
    // second, remap the inputs to the top of the manager (then, it is easy to output them)

    // start the function
    bNewFunc = pNode->G;  Cudd_Ref( bNewFunc );
    // go over all primary inputs
    for ( i = 0; i < pNode->nDecs; i++ )
    if ( pNode->pDecs[i]->Type != DSD_NODE_BUF ) // remap only if it is not the buffer
    {
        bCube0 = Extra_bddFindOneCube( dd, Cudd_Not(pNode->pDecs[i]->G) );  Cudd_Ref( bCube0 );
        bCof0 = Cudd_Cofactor( dd, bNewFunc, bCube0 );                     Cudd_Ref( bCof0 );
        Cudd_RecursiveDeref( dd, bCube0 );

        bCube1 = Extra_bddFindOneCube( dd,          pNode->pDecs[i]->G  );  Cudd_Ref( bCube1 );
        bCof1 = Cudd_Cofactor( dd, bNewFunc, bCube1 );                     Cudd_Ref( bCof1 );
        Cudd_RecursiveDeref( dd, bCube1 );

        Cudd_RecursiveDeref( dd, bNewFunc );

        // use the variable in the i-th level of the manager
//      bNewFunc = Cudd_bddIte( dd, dd->vars[dd->invperm[i]],bCof1,bCof0 );     Cudd_Ref( bNewFunc );
        // use the first variale in the support of the component
        bNewFunc = Cudd_bddIte( dd, dd->vars[pNode->pDecs[i]->S->index],bCof1,bCof0 );     Cudd_Ref( bNewFunc );
        Cudd_RecursiveDeref( dd, bCof0 );
        Cudd_RecursiveDeref( dd, bCof1 );
    }

    if ( fRemap )
    {
        // remap the function to the top of the manager
        // remap the function to the first variables of the manager
        for ( i = 0; i < pNode->nDecs; i++ )
    //      Permute[ pNode->pDecs[i]->S->index ] = dd->invperm[i];
            Permute[ pNode->pDecs[i]->S->index ] = i;

        bNewFunc = Cudd_bddPermute( dd, bTemp = bNewFunc, Permute );   Cudd_Ref( bNewFunc );
        Cudd_RecursiveDeref( dd, bTemp );
    }

    Cudd_Deref( bNewFunc );
    return bNewFunc;
}


////////////////////////////////////////////////////////////////////////
///                           END OF FILE                            ///
////////////////////////////////////////////////////////////////////////
ABC_NAMESPACE_IMPL_END

