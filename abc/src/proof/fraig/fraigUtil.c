/**CFile****************************************************************

  FileName    [fraigUtil.c]

  PackageName [FRAIG: Functionally reduced AND-INV graphs.]

  Synopsis    [Various utilities.]

  Author      [Alan Mishchenko <alanmi@eecs.berkeley.edu>]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 2.0. Started - October 1, 2004]

  Revision    [$Id: fraigUtil.c,v 1.15 2005/07/08 01:01:34 alanmi Exp $]

***********************************************************************/

#include "fraigInt.h"
#include <limits.h>

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static int bit_count[256] = {
  0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4,1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,
  1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
  1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
  2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
  1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
  2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
  2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
  3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,4,5,5,6,5,6,6,7,5,6,6,7,6,7,7,8
};

static void Fraig_Dfs_rec( Fraig_Man_t * pMan, Fraig_Node_t * pNode, Fraig_NodeVec_t * vNodes, int fEquiv );
static int  Fraig_CheckTfi_rec( Fraig_Man_t * pMan, Fraig_Node_t * pNode, Fraig_Node_t * pOld );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Computes the DFS ordering of the nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fraig_NodeVec_t * Fraig_Dfs( Fraig_Man_t * pMan, int fEquiv )
{
    Fraig_NodeVec_t * vNodes;
    int i;
    pMan->nTravIds++;
    vNodes = Fraig_NodeVecAlloc( 100 );
    for ( i = 0; i < pMan->vOutputs->nSize; i++ )
        Fraig_Dfs_rec( pMan, Fraig_Regular(pMan->vOutputs->pArray[i]), vNodes, fEquiv );
    return vNodes;
}

/**Function*************************************************************

  Synopsis    [Computes the DFS ordering of the nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fraig_NodeVec_t * Fraig_DfsOne( Fraig_Man_t * pMan, Fraig_Node_t * pNode, int fEquiv )
{
    Fraig_NodeVec_t * vNodes;
    pMan->nTravIds++;
    vNodes = Fraig_NodeVecAlloc( 100 );
    Fraig_Dfs_rec( pMan, Fraig_Regular(pNode), vNodes, fEquiv );
    return vNodes;
}

/**Function*************************************************************

  Synopsis    [Computes the DFS ordering of the nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fraig_NodeVec_t * Fraig_DfsNodes( Fraig_Man_t * pMan, Fraig_Node_t ** ppNodes, int nNodes, int fEquiv )
{
    Fraig_NodeVec_t * vNodes;
    int i;
    pMan->nTravIds++;
    vNodes = Fraig_NodeVecAlloc( 100 );
    for ( i = 0; i < nNodes; i++ )
        Fraig_Dfs_rec( pMan, Fraig_Regular(ppNodes[i]), vNodes, fEquiv );
    return vNodes;
}

/**Function*************************************************************

  Synopsis    [Recursively computes the DFS ordering of the nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_Dfs_rec( Fraig_Man_t * pMan, Fraig_Node_t * pNode, Fraig_NodeVec_t * vNodes, int fEquiv )
{
    assert( !Fraig_IsComplement(pNode) );
    // skip the visited node
    if ( pNode->TravId == pMan->nTravIds )
        return;
    pNode->TravId = pMan->nTravIds;
    // visit the transitive fanin
    if ( Fraig_NodeIsAnd(pNode) )
    {
        Fraig_Dfs_rec( pMan, Fraig_Regular(pNode->p1), vNodes, fEquiv );
        Fraig_Dfs_rec( pMan, Fraig_Regular(pNode->p2), vNodes, fEquiv );
    }
    if ( fEquiv && pNode->pNextE )
        Fraig_Dfs_rec( pMan, pNode->pNextE, vNodes, fEquiv );
    // save the node
    Fraig_NodeVecPush( vNodes, pNode );
}

/**Function*************************************************************

  Synopsis    [Computes the DFS ordering of the nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fraig_CountNodes( Fraig_Man_t * pMan, int fEquiv )
{
    Fraig_NodeVec_t * vNodes;
    int RetValue;
    vNodes = Fraig_Dfs( pMan, fEquiv );
    RetValue = vNodes->nSize;
    Fraig_NodeVecFree( vNodes );
    return RetValue;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if pOld is in the TFI of pNew.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fraig_CheckTfi( Fraig_Man_t * pMan, Fraig_Node_t * pOld, Fraig_Node_t * pNew )
{
    assert( !Fraig_IsComplement(pOld) );
    assert( !Fraig_IsComplement(pNew) );
    pMan->nTravIds++;
    return Fraig_CheckTfi_rec( pMan, pNew, pOld );
}

/**Function*************************************************************

  Synopsis    [Returns 1 if pOld is in the TFI of pNew.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fraig_CheckTfi_rec( Fraig_Man_t * pMan, Fraig_Node_t * pNode, Fraig_Node_t * pOld )
{
    // check the trivial cases
    if ( pNode == NULL )
        return 0;
    if ( pNode->Num < pOld->Num && !pMan->fChoicing )
        return 0;
    if ( pNode == pOld )
        return 1;
    // skip the visited node
    if ( pNode->TravId == pMan->nTravIds )
        return 0;
    pNode->TravId = pMan->nTravIds;
    // check the children
    if ( Fraig_CheckTfi_rec( pMan, Fraig_Regular(pNode->p1), pOld ) )
        return 1;
    if ( Fraig_CheckTfi_rec( pMan, Fraig_Regular(pNode->p2), pOld ) )
        return 1;
    // check equivalent nodes
    return Fraig_CheckTfi_rec( pMan, pNode->pNextE, pOld );
}


/**Function*************************************************************

  Synopsis    [Returns 1 if pOld is in the TFI of pNew.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fraig_CheckTfi2( Fraig_Man_t * pMan, Fraig_Node_t * pOld, Fraig_Node_t * pNew )
{
    Fraig_NodeVec_t * vNodes;
    int RetValue;
    vNodes = Fraig_DfsOne( pMan, pNew, 1 );
    RetValue = (pOld->TravId == pMan->nTravIds);
    Fraig_NodeVecFree( vNodes );
    return RetValue;
}

/**Function*************************************************************

  Synopsis    [Sets the number of fanouts (none, one, or many).]

  Description [This procedure collects the nodes reachable from
  the POs of the AIG and sets the type of fanout counter (none, one,
  or many) for each node. This procedure is useful to determine
  fanout-free cones of AND-nodes, which is helpful for rebalancing
  the AIG (see procedure Fraig_ManRebalance, or something like that).]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_ManMarkRealFanouts( Fraig_Man_t * p )
{
    Fraig_NodeVec_t * vNodes;
    Fraig_Node_t * pNodeR;
    int i;
    // collect the nodes reachable
    vNodes = Fraig_Dfs( p, 0 );
    // clean the fanouts field
    for ( i = 0; i < vNodes->nSize; i++ )
    {
        vNodes->pArray[i]->nFanouts = 0;
        vNodes->pArray[i]->pData0 = NULL;
    }
    // mark reachable nodes by setting the two-bit counter pNode->nFans
    for ( i = 0; i < vNodes->nSize; i++ )
    {
        pNodeR = Fraig_Regular(vNodes->pArray[i]->p1);
        if ( pNodeR && ++pNodeR->nFanouts == 3 )
            pNodeR->nFanouts = 2;
        pNodeR = Fraig_Regular(vNodes->pArray[i]->p2);
        if ( pNodeR && ++pNodeR->nFanouts == 3 )
            pNodeR->nFanouts = 2;
    }
    Fraig_NodeVecFree( vNodes );
}

/**Function*************************************************************

  Synopsis    [Creates the constant 1 node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fraig_BitStringCountOnes( unsigned * pString, int nWords )
{
    unsigned char * pSuppBytes = (unsigned char *)pString;
    int i, nOnes, nBytes = sizeof(unsigned) * nWords;
    // count the number of ones in the simulation vector
    for ( i = nOnes = 0; i < nBytes; i++ )
        nOnes += bit_count[pSuppBytes[i]];
    return nOnes;
}

/**Function*************************************************************

  Synopsis    [Verify one useful property.]

  Description [This procedure verifies one useful property. After 
  the FRAIG construction with choice nodes is over, each primary node 
  should have fanins that are primary nodes. The primary nodes is the 
  one that does not have pNode->pRepr set to point to another node.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fraig_ManCheckConsistency( Fraig_Man_t * p )
{
    Fraig_Node_t * pNode;
    Fraig_NodeVec_t * pVec;
    int i;
    pVec = Fraig_Dfs( p, 0 );
    for ( i = 0; i < pVec->nSize; i++ )
    {
        pNode = pVec->pArray[i];
        if ( Fraig_NodeIsVar(pNode) )
        {
            if ( pNode->pRepr )
                printf( "Primary input %d is a secondary node.\n", pNode->Num );
        }
        else if ( Fraig_NodeIsConst(pNode) )
        {
            if ( pNode->pRepr )
                printf( "Constant 1 %d is a secondary node.\n", pNode->Num );
        }
        else
        {
            if ( pNode->pRepr )
                printf( "Internal node %d is a secondary node.\n", pNode->Num );
            if ( Fraig_Regular(pNode->p1)->pRepr )
                printf( "Internal node %d has first fanin %d that is a secondary node.\n", 
                    pNode->Num, Fraig_Regular(pNode->p1)->Num );
            if ( Fraig_Regular(pNode->p2)->pRepr )
                printf( "Internal node %d has second fanin %d that is a secondary node.\n", 
                    pNode->Num, Fraig_Regular(pNode->p2)->Num );
        }
    }
    Fraig_NodeVecFree( pVec );
    return 1;
}

/**Function*************************************************************

  Synopsis    [Prints the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_PrintNode( Fraig_Man_t * p, Fraig_Node_t * pNode )
{
    Fraig_NodeVec_t * vNodes;
    Fraig_Node_t * pTemp;
    int fCompl1, fCompl2, i;

    vNodes = Fraig_DfsOne( p, pNode, 0 );
    for ( i = 0; i < vNodes->nSize; i++ )
    {
        pTemp = vNodes->pArray[i];
        if ( Fraig_NodeIsVar(pTemp) )
        {
            printf( "%3d : PI          ", pTemp->Num );
            Fraig_PrintBinary( stdout, (unsigned *)&pTemp->puSimR, 20 );
            printf( "   " );
            Fraig_PrintBinary( stdout, (unsigned *)&pTemp->puSimD, 20 );
            printf( "  %d\n", pTemp->fInv );
            continue;
        }

        fCompl1 = Fraig_IsComplement(pTemp->p1);
        fCompl2 = Fraig_IsComplement(pTemp->p2);
        printf( "%3d : %c%3d %c%3d   ", pTemp->Num,
            (fCompl1? '-':'+'), Fraig_Regular(pTemp->p1)->Num,
            (fCompl2? '-':'+'), Fraig_Regular(pTemp->p2)->Num );
        Fraig_PrintBinary( stdout, (unsigned *)&pTemp->puSimR, 20 );
        printf( "   " );
        Fraig_PrintBinary( stdout, (unsigned *)&pTemp->puSimD, 20 );
        printf( "  %d\n", pTemp->fInv );
    }
    Fraig_NodeVecFree( vNodes );
}

/**Function*************************************************************

  Synopsis    [Prints the bit string.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_PrintBinary( FILE * pFile, unsigned * pSign, int nBits )
{
    int Remainder, nWords;
    int w, i;

    Remainder = (nBits%(sizeof(unsigned)*8));
    nWords    = (nBits/(sizeof(unsigned)*8)) + (Remainder>0);

    for ( w = nWords-1; w >= 0; w-- )
        for ( i = ((w == nWords-1 && Remainder)? Remainder-1: 31); i >= 0; i-- )
            fprintf( pFile, "%c", '0' + (int)((pSign[w] & (1<<i)) > 0) );

//  fprintf( pFile, "\n" );
}

/**Function*************************************************************

  Synopsis    [Sets up the mask.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fraig_GetMaxLevel( Fraig_Man_t * pMan )
{
    int nLevelMax, i;
    nLevelMax = 0;
    for ( i = 0; i < pMan->vOutputs->nSize; i++ )
        nLevelMax = nLevelMax > Fraig_Regular(pMan->vOutputs->pArray[i])->Level? 
                nLevelMax : Fraig_Regular(pMan->vOutputs->pArray[i])->Level;
    return nLevelMax;
}

/**Function*************************************************************

  Synopsis    [Analyses choice nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fraig_MappingUpdateLevel_rec( Fraig_Man_t * pMan, Fraig_Node_t * pNode, int fMaximum )
{
    Fraig_Node_t * pTemp;
    int Level1, Level2, LevelE;
    assert( !Fraig_IsComplement(pNode) );
    if ( !Fraig_NodeIsAnd(pNode) )
        return pNode->Level;
    // skip the visited node
    if ( pNode->TravId == pMan->nTravIds )
        return pNode->Level;
    pNode->TravId = pMan->nTravIds;
    // compute levels of the children nodes
    Level1 = Fraig_MappingUpdateLevel_rec( pMan, Fraig_Regular(pNode->p1), fMaximum );
    Level2 = Fraig_MappingUpdateLevel_rec( pMan, Fraig_Regular(pNode->p2), fMaximum );
    pNode->Level = 1 + Abc_MaxInt( Level1, Level2 );
    if ( pNode->pNextE )
    {
        LevelE = Fraig_MappingUpdateLevel_rec( pMan, pNode->pNextE, fMaximum );
        if ( fMaximum )
        {
            if ( pNode->Level < LevelE )
                pNode->Level = LevelE;
        }
        else
        {
            if ( pNode->Level > LevelE )
                pNode->Level = LevelE;
        }
        // set the level of all equivalent nodes to be the same minimum
        if ( pNode->pRepr == NULL ) // the primary node
            for ( pTemp = pNode->pNextE; pTemp; pTemp = pTemp->pNextE )
                pTemp->Level = pNode->Level;
    }
    return pNode->Level;
}

/**Function*************************************************************

  Synopsis    [Resets the levels of the nodes in the choice graph.]

  Description [Makes the level of the choice nodes to be equal to the
  maximum of the level of the nodes in the equivalence class. This way
  sorting by level leads to the reverse topological order, which is
  needed for the required time computation.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_MappingSetChoiceLevels( Fraig_Man_t * pMan, int fMaximum )
{
    int i;
    pMan->nTravIds++;
    for ( i = 0; i < pMan->vOutputs->nSize; i++ )
        Fraig_MappingUpdateLevel_rec( pMan, Fraig_Regular(pMan->vOutputs->pArray[i]), fMaximum );
}

/**Function*************************************************************

  Synopsis    [Reports statistics on choice nodes.]

  Description [The number of choice nodes is the number of primary nodes,
  which has pNextE set to a pointer. The number of choices is the number
  of entries in the equivalent-node lists of the primary nodes.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_ManReportChoices( Fraig_Man_t * pMan )
{
    Fraig_Node_t * pNode, * pTemp;
    int nChoiceNodes, nChoices;
    int i, LevelMax1, LevelMax2;

    // report the number of levels
    LevelMax1 = Fraig_GetMaxLevel( pMan );
    Fraig_MappingSetChoiceLevels( pMan, 0 );
    LevelMax2 = Fraig_GetMaxLevel( pMan );

    // report statistics about choices
    nChoiceNodes = nChoices = 0;
    for ( i = 0; i < pMan->vNodes->nSize; i++ )
    {
        pNode = pMan->vNodes->pArray[i];
        if ( pNode->pRepr == NULL && pNode->pNextE != NULL )
        { // this is a choice node = the primary node that has equivalent nodes
            nChoiceNodes++;
            for ( pTemp = pNode; pTemp; pTemp = pTemp->pNextE )
                nChoices++;
        }
    }
    printf( "Maximum level: Original = %d. Reduced due to choices = %d.\n", LevelMax1, LevelMax2 );
    printf( "Choice stats:  Choice nodes = %d. Total choices = %d.\n", nChoiceNodes, nChoices );
}

/**Function*************************************************************

  Synopsis    [Returns 1 if the node is the root of EXOR/NEXOR gate.]

  Description [The node can be complemented.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fraig_NodeIsExorType( Fraig_Node_t * pNode )
{
    Fraig_Node_t * pNode1, * pNode2;
    // make the node regular (it does not matter for EXOR/NEXOR)
    pNode = Fraig_Regular(pNode);
    // if the node or its children are not ANDs or not compl, this cannot be EXOR type
    if ( !Fraig_NodeIsAnd(pNode) )
        return 0;
    if ( !Fraig_NodeIsAnd(pNode->p1) || !Fraig_IsComplement(pNode->p1) )
        return 0;
    if ( !Fraig_NodeIsAnd(pNode->p2) || !Fraig_IsComplement(pNode->p2) )
        return 0;

    // get children
    pNode1 = Fraig_Regular(pNode->p1);
    pNode2 = Fraig_Regular(pNode->p2);
    assert( pNode1->Num < pNode2->Num );

    // compare grandchildren
    return pNode1->p1 == Fraig_Not(pNode2->p1) && pNode1->p2 == Fraig_Not(pNode2->p2);
}

/**Function*************************************************************

  Synopsis    [Returns 1 if the node is the root of MUX or EXOR/NEXOR.]

  Description [The node can be complemented.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fraig_NodeIsMuxType( Fraig_Node_t * pNode )
{
    Fraig_Node_t * pNode1, * pNode2;

    // make the node regular (it does not matter for EXOR/NEXOR)
    pNode = Fraig_Regular(pNode);
    // if the node or its children are not ANDs or not compl, this cannot be EXOR type
    if ( !Fraig_NodeIsAnd(pNode) )
        return 0;
    if ( !Fraig_NodeIsAnd(pNode->p1) || !Fraig_IsComplement(pNode->p1) )
        return 0;
    if ( !Fraig_NodeIsAnd(pNode->p2) || !Fraig_IsComplement(pNode->p2) )
        return 0;

    // get children
    pNode1 = Fraig_Regular(pNode->p1);
    pNode2 = Fraig_Regular(pNode->p2);
    assert( pNode1->Num < pNode2->Num );

    // compare grandchildren
    // node is an EXOR/NEXOR
    if ( pNode1->p1 == Fraig_Not(pNode2->p1) && pNode1->p2 == Fraig_Not(pNode2->p2) )
        return 1; 

    // otherwise the node is MUX iff it has a pair of equal grandchildren
    return pNode1->p1 == Fraig_Not(pNode2->p1) || 
           pNode1->p1 == Fraig_Not(pNode2->p2) ||
           pNode1->p2 == Fraig_Not(pNode2->p1) ||
           pNode1->p2 == Fraig_Not(pNode2->p2);
}

/**Function*************************************************************

  Synopsis    [Returns 1 if the node is EXOR, 0 if it is NEXOR.]

  Description [The node should be EXOR type and not complemented.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fraig_NodeIsExor( Fraig_Node_t * pNode )
{
    Fraig_Node_t * pNode1;
    assert( !Fraig_IsComplement(pNode) );
    assert( Fraig_NodeIsExorType(pNode) );
    assert( Fraig_IsComplement(pNode->p1) );
    // get children
    pNode1 = Fraig_Regular(pNode->p1);
    return Fraig_IsComplement(pNode1->p1) == Fraig_IsComplement(pNode1->p2);
}

/**Function*************************************************************

  Synopsis    [Recognizes what nodes are control and data inputs of a MUX.]

  Description [If the node is a MUX, returns the control variable C.
  Assigns nodes T and E to be the then and else variables of the MUX. 
  Node C is never complemented. Nodes T and E can be complemented.
  This function also recognizes EXOR/NEXOR gates as MUXes.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fraig_Node_t * Fraig_NodeRecognizeMux( Fraig_Node_t * pNode, Fraig_Node_t ** ppNodeT, Fraig_Node_t ** ppNodeE )
{
    Fraig_Node_t * pNode1, * pNode2;
    assert( !Fraig_IsComplement(pNode) );
    assert( Fraig_NodeIsMuxType(pNode) );
    // get children
    pNode1 = Fraig_Regular(pNode->p1);
    pNode2 = Fraig_Regular(pNode->p2);
    // find the control variable
    if ( pNode1->p1 == Fraig_Not(pNode2->p1) )
    {
        if ( Fraig_IsComplement(pNode1->p1) )
        { // pNode2->p1 is positive phase of C
            *ppNodeT = Fraig_Not(pNode2->p2);
            *ppNodeE = Fraig_Not(pNode1->p2);
            return pNode2->p1;
        }
        else
        { // pNode1->p1 is positive phase of C
            *ppNodeT = Fraig_Not(pNode1->p2);
            *ppNodeE = Fraig_Not(pNode2->p2);
            return pNode1->p1;
        }
    }
    else if ( pNode1->p1 == Fraig_Not(pNode2->p2) )
    {
        if ( Fraig_IsComplement(pNode1->p1) )
        { // pNode2->p2 is positive phase of C
            *ppNodeT = Fraig_Not(pNode2->p1);
            *ppNodeE = Fraig_Not(pNode1->p2);
            return pNode2->p2;
        }
        else
        { // pNode1->p1 is positive phase of C
            *ppNodeT = Fraig_Not(pNode1->p2);
            *ppNodeE = Fraig_Not(pNode2->p1);
            return pNode1->p1;
        }
    }
    else if ( pNode1->p2 == Fraig_Not(pNode2->p1) )
    {
        if ( Fraig_IsComplement(pNode1->p2) )
        { // pNode2->p1 is positive phase of C
            *ppNodeT = Fraig_Not(pNode2->p2);
            *ppNodeE = Fraig_Not(pNode1->p1);
            return pNode2->p1;
        }
        else
        { // pNode1->p2 is positive phase of C
            *ppNodeT = Fraig_Not(pNode1->p1);
            *ppNodeE = Fraig_Not(pNode2->p2);
            return pNode1->p2;
        }
    }
    else if ( pNode1->p2 == Fraig_Not(pNode2->p2) )
    {
        if ( Fraig_IsComplement(pNode1->p2) )
        { // pNode2->p2 is positive phase of C
            *ppNodeT = Fraig_Not(pNode2->p1);
            *ppNodeE = Fraig_Not(pNode1->p1);
            return pNode2->p2;
        }
        else
        { // pNode1->p2 is positive phase of C
            *ppNodeT = Fraig_Not(pNode1->p1);
            *ppNodeE = Fraig_Not(pNode2->p1);
            return pNode1->p2;
        }
    }
    assert( 0 ); // this is not MUX
    return NULL;
}

/**Function*************************************************************

  Synopsis    [Counts the number of EXOR type nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fraig_ManCountExors( Fraig_Man_t * pMan )
{
    int i, nExors;
    nExors = 0;
    for ( i = 0; i < pMan->vNodes->nSize; i++ )
        nExors += Fraig_NodeIsExorType( pMan->vNodes->pArray[i] );
    return nExors;

}

/**Function*************************************************************

  Synopsis    [Counts the number of EXOR type nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fraig_ManCountMuxes( Fraig_Man_t * pMan )
{
    int i, nMuxes;
    nMuxes = 0;
    for ( i = 0; i < pMan->vNodes->nSize; i++ )
        nMuxes += Fraig_NodeIsMuxType( pMan->vNodes->pArray[i] );
    return nMuxes;

}

/**Function*************************************************************

  Synopsis    [Returns 1 if siminfo of Node1 is contained in siminfo of Node2.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fraig_NodeSimsContained( Fraig_Man_t * pMan, Fraig_Node_t * pNode1, Fraig_Node_t * pNode2 )
{
    unsigned * pUnsigned1, * pUnsigned2;
    int i;

    // compare random siminfo
    pUnsigned1 = pNode1->puSimR;
    pUnsigned2 = pNode2->puSimR;
    for ( i = 0; i < pMan->nWordsRand; i++ )
        if ( pUnsigned1[i] & ~pUnsigned2[i] )
            return 0;

    // compare systematic siminfo
    pUnsigned1 = pNode1->puSimD;
    pUnsigned2 = pNode2->puSimD;
    for ( i = 0; i < pMan->iWordStart; i++ )
        if ( pUnsigned1[i] & ~pUnsigned2[i] )
            return 0;

    return 1;
}

/**Function*************************************************************

  Synopsis    [Count the number of PI variables.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fraig_CountPis( Fraig_Man_t * p, Msat_IntVec_t * vVarNums )
{
    int * pVars, nVars, i, Counter;

    nVars = Msat_IntVecReadSize(vVarNums);
    pVars = Msat_IntVecReadArray(vVarNums);
    Counter = 0;
    for ( i = 0; i < nVars; i++ )
        Counter += Fraig_NodeIsVar( p->vNodes->pArray[pVars[i]] );
    return Counter;
}



/**Function*************************************************************

  Synopsis    [Counts the number of EXOR type nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fraig_ManPrintRefs( Fraig_Man_t * pMan )
{
    Fraig_NodeVec_t * vPivots;
    Fraig_Node_t * pNode, * pNode2;
    int i, k, Counter, nProved;
    abctime clk;

    vPivots = Fraig_NodeVecAlloc( 1000 );
    for ( i = 0; i < pMan->vNodes->nSize; i++ )
    {
        pNode = pMan->vNodes->pArray[i];

        if ( pNode->nOnes == 0 || pNode->nOnes == (unsigned)pMan->nWordsRand * 32 )
            continue;

        if ( pNode->nRefs > 5 )
        {
            Fraig_NodeVecPush( vPivots, pNode );
//            printf( "Node %6d : nRefs = %2d   Level = %3d.\n", pNode->Num, pNode->nRefs, pNode->Level );
        }
    }
    printf( "Total nodes = %d. Referenced nodes = %d.\n", pMan->vNodes->nSize, vPivots->nSize );

clk = Abc_Clock();
    // count implications
    Counter = nProved = 0;
    for ( i = 0; i < vPivots->nSize; i++ )
        for ( k = i+1; k < vPivots->nSize; k++ )
        {
            pNode  = vPivots->pArray[i];
            pNode2 = vPivots->pArray[k];
            if ( Fraig_NodeSimsContained( pMan, pNode, pNode2 ) )
            {
                if ( Fraig_NodeIsImplication( pMan, pNode, pNode2, -1 ) )
                    nProved++;
                Counter++;
            }
            else if ( Fraig_NodeSimsContained( pMan, pNode2, pNode ) )
            {
                if ( Fraig_NodeIsImplication( pMan, pNode2, pNode, -1 ) )
                    nProved++;
                Counter++;
            }
        }
    printf( "Number of candidate pairs = %d.  Proved = %d.\n", Counter, nProved );
//ABC_PRT( "Time", Abc_Clock() - clk );
    return 0;
}


/**Function*************************************************************

  Synopsis    [Checks if pNew exists among the implication fanins of pOld.]

  Description [If pNew is an implication fanin of pOld, returns 1. 
  If Fraig_Not(pNew) is an implication fanin of pOld, return -1.
  Otherwise returns 0.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fraig_NodeIsInSupergate( Fraig_Node_t * pOld, Fraig_Node_t * pNew )
{
    int RetValue1, RetValue2;
    if ( Fraig_Regular(pOld) == Fraig_Regular(pNew) )
        return (pOld == pNew)? 1 : -1;
    if ( Fraig_IsComplement(pOld) || Fraig_NodeIsVar(pOld) )
        return 0;
    RetValue1 = Fraig_NodeIsInSupergate( pOld->p1, pNew );
    RetValue2 = Fraig_NodeIsInSupergate( pOld->p2, pNew );
    if ( RetValue1 == -1 || RetValue2 == -1 )
        return -1;
    if ( RetValue1 ==  1 || RetValue2 ==  1 )
        return 1;
    return 0;
}


/**Function*************************************************************

  Synopsis    [Returns the array of nodes to be combined into one multi-input AND-gate.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_CollectSupergate_rec( Fraig_Node_t * pNode, Fraig_NodeVec_t * vSuper, int fFirst, int fStopAtMux )
{
    // if the new node is complemented or a PI, another gate begins
//    if ( Fraig_IsComplement(pNode) || Fraig_NodeIsVar(pNode) || Fraig_NodeIsMuxType(pNode) )
    if ( (!fFirst && Fraig_Regular(pNode)->nRefs > 1) || 
          Fraig_IsComplement(pNode) || Fraig_NodeIsVar(pNode) || 
          (fStopAtMux && Fraig_NodeIsMuxType(pNode)) )
    {
        Fraig_NodeVecPushUnique( vSuper, pNode );
        return;
    }
    // go through the branches
    Fraig_CollectSupergate_rec( pNode->p1, vSuper, 0, fStopAtMux );
    Fraig_CollectSupergate_rec( pNode->p2, vSuper, 0, fStopAtMux );
}

/**Function*************************************************************

  Synopsis    [Returns the array of nodes to be combined into one multi-input AND-gate.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fraig_NodeVec_t * Fraig_CollectSupergate( Fraig_Node_t * pNode, int fStopAtMux )
{
    Fraig_NodeVec_t * vSuper;
    vSuper = Fraig_NodeVecAlloc( 8 );
    Fraig_CollectSupergate_rec( pNode, vSuper, 1, fStopAtMux );
    return vSuper;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_ManIncrementTravId( Fraig_Man_t * pMan )
{
    pMan->nTravIds2++;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_NodeSetTravIdCurrent( Fraig_Man_t * pMan, Fraig_Node_t * pNode )
{
    pNode->TravId2 = pMan->nTravIds2;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fraig_NodeIsTravIdCurrent( Fraig_Man_t * pMan, Fraig_Node_t * pNode )
{
    return pNode->TravId2 == pMan->nTravIds2;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fraig_NodeIsTravIdPrevious( Fraig_Man_t * pMan, Fraig_Node_t * pNode )
{
    return pNode->TravId2 == pMan->nTravIds2 - 1;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

