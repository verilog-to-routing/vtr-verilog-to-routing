/**CFile****************************************************************

  FileName    [lpkCut.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Fast Boolean matching for LUT structures.]

  Synopsis    []

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - April 28, 2007.]

  Revision    [$Id: lpkCut.c,v 1.00 2007/04/28 00:00:00 alanmi Exp $]

***********************************************************************/

#include "lpkInt.h"
#include "bool/kit/cloud.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////
 
////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Computes the truth table of one cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
CloudNode * Lpk_CutTruthBdd_rec( CloudManager * dd, Hop_Man_t * pMan, Hop_Obj_t * pObj, int nVars )
{
    CloudNode * pTruth, * pTruth0, * pTruth1;
    assert( !Hop_IsComplement(pObj) );
    if ( pObj->pData )
    {
        assert( ((unsigned)(ABC_PTRUINT_T)pObj->pData) & 0xffff0000 );
        return (CloudNode *)pObj->pData;
    }
    // get the plan for a new truth table
    if ( Hop_ObjIsConst1(pObj) )
        pTruth = dd->one;
    else
    {
        assert( Hop_ObjIsAnd(pObj) );
        // compute the truth tables of the fanins
        pTruth0 = Lpk_CutTruthBdd_rec( dd, pMan, Hop_ObjFanin0(pObj), nVars );
        pTruth1 = Lpk_CutTruthBdd_rec( dd, pMan, Hop_ObjFanin1(pObj), nVars );
        pTruth0 = Cloud_NotCond( pTruth0, Hop_ObjFaninC0(pObj) );
        pTruth1 = Cloud_NotCond( pTruth1, Hop_ObjFaninC1(pObj) );
        // creat the truth table of the node
        pTruth = Cloud_bddAnd( dd, pTruth0, pTruth1 );
    }
    pObj->pData = pTruth;
    return pTruth;
}

/**Function*************************************************************

  Synopsis    [Verifies that the factoring is correct.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
CloudNode * Lpk_CutTruthBdd( Lpk_Man_t * p, Lpk_Cut_t * pCut )
{
    CloudManager * dd = p->pDsdMan->dd;
    Hop_Man_t * pManHop = (Hop_Man_t *)p->pNtk->pManFunc;
    Hop_Obj_t * pObjHop;
    Abc_Obj_t * pObj, * pFanin;
    CloudNode * pTruth = NULL; // Suppress "might be used uninitialized"
    int i, k;

//    return NULL;
//    Lpk_NodePrintCut( p, pCut );
    // initialize the leaves
    Lpk_CutForEachLeaf( p->pNtk, pCut, pObj, i )
        pObj->pCopy = (Abc_Obj_t *)dd->vars[pCut->nLeaves-1-i];

    // construct truth table in the topological order
    Lpk_CutForEachNodeReverse( p->pNtk, pCut, pObj, i )
    {
        // get the local AIG
        pObjHop = Hop_Regular((Hop_Obj_t *)pObj->pData);
        // clean the data field of the nodes in the AIG subgraph
        Hop_ObjCleanData_rec( pObjHop );
        // set the initial truth tables at the fanins
        Abc_ObjForEachFanin( pObj, pFanin, k )
        {
            assert( ((unsigned)(ABC_PTRUINT_T)pFanin->pCopy) & 0xffff0000 );
            Hop_ManPi( pManHop, k )->pData = pFanin->pCopy;
        }
        // compute the truth table of internal nodes
        pTruth = Lpk_CutTruthBdd_rec( dd, pManHop, pObjHop, pCut->nLeaves );
        if ( Hop_IsComplement((Hop_Obj_t *)pObj->pData) )
            pTruth = Cloud_Not(pTruth);
        // set the truth table at the node
        pObj->pCopy = (Abc_Obj_t *)pTruth;
        
    }

//    Cloud_bddPrint( dd, pTruth );
//    printf( "Bdd size = %d. Total nodes = %d.\n", Cloud_DagSize( dd, pTruth ), dd->nNodesCur-dd->nVars-1 );
    return pTruth;
}


/**Function*************************************************************

  Synopsis    [Computes the truth table of one cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned * Lpk_CutTruth_rec( Hop_Man_t * pMan, Hop_Obj_t * pObj, int nVars, Vec_Ptr_t * vTtNodes, int * piCount )
{
    unsigned * pTruth, * pTruth0, * pTruth1;
    assert( !Hop_IsComplement(pObj) );
    if ( pObj->pData )
    {
        assert( ((unsigned)(ABC_PTRUINT_T)pObj->pData) & 0xffff0000 );
        return (unsigned *)pObj->pData;
    }
    // get the plan for a new truth table
    pTruth = (unsigned *)Vec_PtrEntry( vTtNodes, (*piCount)++ );
    if ( Hop_ObjIsConst1(pObj) )
        Kit_TruthFill( pTruth, nVars );
    else
    {
        assert( Hop_ObjIsAnd(pObj) );
        // compute the truth tables of the fanins
        pTruth0 = Lpk_CutTruth_rec( pMan, Hop_ObjFanin0(pObj), nVars, vTtNodes, piCount );
        pTruth1 = Lpk_CutTruth_rec( pMan, Hop_ObjFanin1(pObj), nVars, vTtNodes, piCount );
        // creat the truth table of the node
        Kit_TruthAndPhase( pTruth, pTruth0, pTruth1, nVars, Hop_ObjFaninC0(pObj), Hop_ObjFaninC1(pObj) );
    }
    pObj->pData = pTruth;
    return pTruth;
}

/**Function*************************************************************

  Synopsis    [Computes the truth able of one cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned * Lpk_CutTruth( Lpk_Man_t * p, Lpk_Cut_t * pCut, int fInv )
{
    Hop_Man_t * pManHop = (Hop_Man_t *)p->pNtk->pManFunc;
    Hop_Obj_t * pObjHop;
    Abc_Obj_t * pObj = NULL; // Suppress "might be used uninitialized"
    Abc_Obj_t * pFanin;
    unsigned * pTruth = NULL; // Suppress "might be used uninitialized"
    int i, k, iCount = 0;
//    Lpk_NodePrintCut( p, pCut );
    assert( pCut->nNodes > 0 );

    // initialize the leaves
    Lpk_CutForEachLeaf( p->pNtk, pCut, pObj, i )
        pObj->pCopy = (Abc_Obj_t *)Vec_PtrEntry( p->vTtElems, fInv? pCut->nLeaves-1-i : i );

    // construct truth table in the topological order
    Lpk_CutForEachNodeReverse( p->pNtk, pCut, pObj, i )
    {
        // get the local AIG
        pObjHop = Hop_Regular((Hop_Obj_t *)pObj->pData);
        // clean the data field of the nodes in the AIG subgraph
        Hop_ObjCleanData_rec( pObjHop );
        // set the initial truth tables at the fanins
        Abc_ObjForEachFanin( pObj, pFanin, k )
        {
            assert( ((unsigned)(ABC_PTRUINT_T)pFanin->pCopy) & 0xffff0000 );
            Hop_ManPi( pManHop, k )->pData = pFanin->pCopy;
        }
        // compute the truth table of internal nodes
        pTruth = Lpk_CutTruth_rec( pManHop, pObjHop, pCut->nLeaves, p->vTtNodes, &iCount );
        if ( Hop_IsComplement((Hop_Obj_t *)pObj->pData) )
            Kit_TruthNot( pTruth, pTruth, pCut->nLeaves );
        // set the truth table at the node
        pObj->pCopy = (Abc_Obj_t *)pTruth;
    }

    // make sure direct truth table is stored elsewhere (assuming the first call for direct truth!!!)
    if ( fInv == 0 )
    {
        pTruth = (unsigned *)Vec_PtrEntry( p->vTtNodes, iCount++ );
        Kit_TruthCopy( pTruth, (unsigned *)(ABC_PTRUINT_T)pObj->pCopy, pCut->nLeaves );
    }
    assert( iCount <= Vec_PtrSize(p->vTtNodes) );
    return pTruth;
}


/**Function*************************************************************

  Synopsis    [Returns 1 if at least one entry has changed.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Lpk_NodeRecordImpact( Lpk_Man_t * p )
{
    Lpk_Cut_t * pCut;
    Vec_Ptr_t * vNodes = Vec_VecEntry( p->vVisited, p->pObj->Id );
    Abc_Obj_t * pNode, * pNode2;
    int i, k;
    // collect the nodes that impact the given node
    Vec_PtrClear( vNodes );
    for ( i = 0; i < p->nCuts; i++ )
    {
        pCut = p->pCuts + i;
        for ( k = 0; k < (int)pCut->nLeaves; k++ )
        {
            pNode = Abc_NtkObj( p->pNtk, pCut->pLeaves[k] );
            if ( pNode->fMarkC )
                continue;
            pNode->fMarkC = 1;
            Vec_PtrPush( vNodes, (void *)(ABC_PTRUINT_T)pNode->Id );
            Vec_PtrPush( vNodes, (void *)(ABC_PTRUINT_T)Abc_ObjFanoutNum(pNode) );
        }
    }
    // clear the marks
    Vec_PtrForEachEntryDouble( Abc_Obj_t *, Abc_Obj_t *, vNodes, pNode, pNode2, i )
    {
        pNode = Abc_NtkObj( p->pNtk, (int)(ABC_PTRUINT_T)pNode );
        pNode->fMarkC = 0;
//        i++;
    }
//printf( "%d ", Vec_PtrSize(vNodes) );
}

/**Function*************************************************************

  Synopsis    [Returns 1 if the cut has structural DSD.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Lpk_NodeCutsCheckDsd( Lpk_Man_t * p, Lpk_Cut_t * pCut )
{
    Abc_Obj_t * pObj, * pFanin;
    int i, k, nCands, fLeavesOnly, RetValue;
    assert( pCut->nLeaves > 0 );
    // clear ref counters
    memset( p->pRefs, 0, sizeof(int) * pCut->nLeaves );
    // mark cut leaves
    Lpk_CutForEachLeaf( p->pNtk, pCut, pObj, i )
    {
        assert( pObj->fMarkA == 0 );
        pObj->fMarkA = 1;
        pObj->pCopy = (Abc_Obj_t *)(ABC_PTRUINT_T)i;
    }
    // ref leaves pointed from the internal nodes
    nCands = 0;
    Lpk_CutForEachNode( p->pNtk, pCut, pObj, i )
    {
        fLeavesOnly = 1;
        Abc_ObjForEachFanin( pObj, pFanin, k )
            if ( pFanin->fMarkA )
                p->pRefs[(int)(ABC_PTRUINT_T)pFanin->pCopy]++;
            else
                fLeavesOnly = 0;
        if ( fLeavesOnly )
            p->pCands[nCands++] = pObj->Id;
    }
    // look at the nodes that only point to the leaves
    RetValue = 0;
    for ( i = 0; i < nCands; i++ )
    {
        pObj = Abc_NtkObj( p->pNtk, p->pCands[i] );
        Abc_ObjForEachFanin( pObj, pFanin, k )
        {
            assert( pFanin->fMarkA == 1 );
            if ( p->pRefs[(int)(ABC_PTRUINT_T)pFanin->pCopy] > 1 )
                break;
        }
        if ( k == Abc_ObjFaninNum(pObj) )
        {
            RetValue = 1;
            break;
        }
    }
    // unmark cut leaves
    Lpk_CutForEachLeaf( p->pNtk, pCut, pObj, i )
        pObj->fMarkA = 0;
    return RetValue;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if pDom is contained in pCut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Lpk_NodeCutsOneDominance( Lpk_Cut_t * pDom, Lpk_Cut_t * pCut )
{
    int i, k;
    for ( i = 0; i < (int)pDom->nLeaves; i++ )
    {
        for ( k = 0; k < (int)pCut->nLeaves; k++ )
            if ( pDom->pLeaves[i] == pCut->pLeaves[k] )
                break;
        if ( k == (int)pCut->nLeaves ) // node i in pDom is not contained in pCut
            return 0;
    }
    // every node in pDom is contained in pCut
    return 1;
}

/**Function*************************************************************

  Synopsis    [Check if the cut exists.]

  Description [Returns 1 if the cut exists.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Lpk_NodeCutsOneFilter( Lpk_Cut_t * pCuts, int nCuts, Lpk_Cut_t * pCutNew )
{
    Lpk_Cut_t * pCut;
    int i, k;
    assert( pCutNew->uSign[0] || pCutNew->uSign[1] );
    // try to find the cut
    for ( i = 0; i < nCuts; i++ )
    {
        pCut = pCuts + i;
        if ( pCut->nLeaves == 0 )
            continue;
        if ( pCut->nLeaves == pCutNew->nLeaves )
        {
            if ( pCut->uSign[0] == pCutNew->uSign[0] && pCut->uSign[1] == pCutNew->uSign[1] )
            {
                for ( k = 0; k < (int)pCutNew->nLeaves; k++ )
                    if ( pCut->pLeaves[k] != pCutNew->pLeaves[k] )
                        break;
                if ( k == (int)pCutNew->nLeaves )
                    return 1;
            }
            continue;
        }
        if ( pCut->nLeaves < pCutNew->nLeaves )
        {
            // skip the non-contained cuts
            if ( (pCut->uSign[0] & pCutNew->uSign[0]) != pCut->uSign[0] )
                continue;
            if ( (pCut->uSign[1] & pCutNew->uSign[1]) != pCut->uSign[1] )
                continue;
            // check containment seriously
            if ( Lpk_NodeCutsOneDominance( pCut, pCutNew ) )
                return 1;
            continue;
        }
        // check potential containment of other cut

        // skip the non-contained cuts
        if ( (pCut->uSign[0] & pCutNew->uSign[0]) != pCutNew->uSign[0] )
            continue;
        if ( (pCut->uSign[1] & pCutNew->uSign[1]) != pCutNew->uSign[1] )
            continue;
        // check containment seriously
        if ( Lpk_NodeCutsOneDominance( pCutNew, pCut ) )
            pCut->nLeaves = 0; // removed
    }
    return 0;
}

/**Function*************************************************************

  Synopsis    [Prints the given cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Lpk_NodePrintCut( Lpk_Man_t * p, Lpk_Cut_t * pCut, int fLeavesOnly )
{
    Abc_Obj_t * pObj;
    int i;
    if ( !fLeavesOnly )
        printf( "LEAVES:\n" );
    Lpk_CutForEachLeaf( p->pNtk, pCut, pObj, i )
        printf( "%d,", pObj->Id );
    if ( !fLeavesOnly )
    {
        printf( "\nNODES:\n" );
        Lpk_CutForEachNode( p->pNtk, pCut, pObj, i )
        {
            printf( "%d,", pObj->Id );
            assert( Abc_ObjIsNode(pObj) );
        }
        printf( "\n" );
    }
}

/**Function*************************************************************

  Synopsis    [Set the cut signature.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Lpk_NodeCutSignature( Lpk_Cut_t * pCut )
{
    unsigned i;
    pCut->uSign[0] = pCut->uSign[1] = 0;
    for ( i = 0; i < pCut->nLeaves; i++ )
    {
        pCut->uSign[(pCut->pLeaves[i] & 32) > 0] |= (1 << (pCut->pLeaves[i] & 31));
        if ( i != pCut->nLeaves - 1 )
            assert( pCut->pLeaves[i] < pCut->pLeaves[i+1] );
    }
}


/**Function*************************************************************

  Synopsis    [Computes the set of all cuts.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Lpk_NodeCutsOne( Lpk_Man_t * p, Lpk_Cut_t * pCut, int Node )
{
    Lpk_Cut_t * pCutNew;
    Abc_Obj_t * pObj, * pFanin;
    int i, k, j, nLeavesNew;
/*
    printf( "Exploring cut " );
    Lpk_NodePrintCut( p, pCut, 1 );
    printf( "with node %d\n", Node );
*/
    // check if the cut can stand adding one more internal node
    if ( pCut->nNodes == LPK_SIZE_MAX )
        return;

    // if the node is a PI, quit
    pObj = Abc_NtkObj( p->pNtk, Node );
    if ( Abc_ObjIsCi(pObj) )
        return;
    assert( Abc_ObjIsNode(pObj) );
//    assert( Abc_ObjFaninNum(pObj) <= p->pPars->nLutSize );

    // if the node is not in the MFFC, check the limit
    if ( !Abc_NodeIsTravIdCurrent(pObj) )
    {
        if ( (int)pCut->nNodesDup == p->pPars->nLutsOver )
            return;
        assert( (int)pCut->nNodesDup < p->pPars->nLutsOver );
    }

    // check the possibility of adding this node using the signature
    nLeavesNew = pCut->nLeaves - 1;
    Abc_ObjForEachFanin( pObj, pFanin, i )
    {
        if ( (pCut->uSign[(pFanin->Id & 32) > 0] & (1 << (pFanin->Id & 31))) )
            continue;
        if ( ++nLeavesNew > p->pPars->nVarsMax )
            return;
    }

    // initialize the set of leaves to the nodes in the cut
    assert( p->nCuts < LPK_CUTS_MAX );
    pCutNew = p->pCuts + p->nCuts;
    pCutNew->nLeaves = 0;
    for ( i = 0; i < (int)pCut->nLeaves; i++ )
        if ( pCut->pLeaves[i] != Node )
            pCutNew->pLeaves[pCutNew->nLeaves++] = pCut->pLeaves[i];

    // add new nodes
    Abc_ObjForEachFanin( pObj, pFanin, i )
    {
        // find the place where this node belongs
        for ( k = 0; k < (int)pCutNew->nLeaves; k++ )
            if ( pCutNew->pLeaves[k] >= pFanin->Id )
                break;
        if ( k < (int)pCutNew->nLeaves && pCutNew->pLeaves[k] == pFanin->Id )
            continue;
        // check if there is room
        if ( (int)pCutNew->nLeaves == p->pPars->nVarsMax )
            return;
        // move all the nodes
        for ( j = pCutNew->nLeaves; j > k; j-- )
            pCutNew->pLeaves[j] = pCutNew->pLeaves[j-1];
        pCutNew->pLeaves[k] = pFanin->Id;
        pCutNew->nLeaves++;
        assert( pCutNew->nLeaves <= LPK_SIZE_MAX );

    }
    // skip the contained cuts
    Lpk_NodeCutSignature( pCutNew );
    if ( Lpk_NodeCutsOneFilter( p->pCuts, p->nCuts, pCutNew ) )
        return;

    // update the set of internal nodes
    assert( pCut->nNodes < LPK_SIZE_MAX );
    memcpy( pCutNew->pNodes, pCut->pNodes, pCut->nNodes * sizeof(int) );
    pCutNew->nNodes = pCut->nNodes;
    pCutNew->nNodesDup = pCut->nNodesDup;

    // check if the node is already there
    // if so, move the node to be the last
    for ( i = 0; i < (int)pCutNew->nNodes; i++ )
        if ( pCutNew->pNodes[i] == Node )
        {
            for ( k = i; k < (int)pCutNew->nNodes - 1; k++ )
                pCutNew->pNodes[k] = pCutNew->pNodes[k+1];
            pCutNew->pNodes[k] = Node;
            break;
        }
    if ( i == (int)pCutNew->nNodes ) // new node
    {
        pCutNew->pNodes[ pCutNew->nNodes++ ] = Node;
        pCutNew->nNodesDup += !Abc_NodeIsTravIdCurrent(pObj);
    }
    // the number of nodes does not exceed MFFC plus duplications
    assert( pCutNew->nNodes <= p->nMffc + pCutNew->nNodesDup );
    // add the cut to storage
    assert( p->nCuts < LPK_CUTS_MAX );
    p->nCuts++;
}

/**Function*************************************************************

  Synopsis    [Count support.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Lpk_CountSupp( Abc_Ntk_t * p, Vec_Ptr_t * vNodes )
{
    Abc_Obj_t * pObj, * pFanin; 
    int i, k, Count = 0;
    Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pObj, i )
    {
        Abc_ObjForEachFanin( pObj, pFanin, k )
        {
            if ( Abc_NodeIsTravIdCurrent(pFanin) )
                continue;
            Count += !pFanin->fPersist;
            pFanin->fPersist = 1;
        }
    }
    Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pObj, i )
        Abc_ObjForEachFanin( pObj, pFanin, k )
            pFanin->fPersist = 0;
    return Count;
}

/**Function*************************************************************

  Synopsis    [Computes the set of all cuts.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Lpk_NodeCuts( Lpk_Man_t * p )
{
    Lpk_Cut_t * pCut, * pCut2;
    int i, k, Temp, nMffc, fChanges;
    //int nSupp;

    // mark the MFFC of the node with the current trav ID
    Vec_PtrClear( p->vTemp );
    nMffc = p->nMffc = Abc_NodeMffcLabel( p->pObj, p->vTemp );
    assert( nMffc > 0 );
    if ( nMffc == 1 )
        return 0;
/*
    // count the leaves
    nSupp = Lpk_CountSupp( p->pNtk, p->vTemp );
    if ( nMffc > 10 && nSupp <= 10 )
        printf( "Obj = %4d : Supp = %4d. Mffc = %4d.\n", p->pObj->Id, nSupp, nMffc );
*/
    // initialize the first cut
    pCut = p->pCuts; p->nCuts = 1;
    pCut->nNodes = 0; 
    pCut->nNodesDup = 0;
    pCut->nLeaves = 1;
    pCut->pLeaves[0] = p->pObj->Id;
    // assign the signature
    Lpk_NodeCutSignature( pCut );

    // perform the cut computation
    for ( i = 0; i < p->nCuts; i++ )
    {
        pCut = p->pCuts + i;
        if ( pCut->nLeaves == 0 )
            continue;

        // try to expand the fanins of this cut
        for ( k = 0; k < (int)pCut->nLeaves; k++ )
        {
            // create a new cut
            Lpk_NodeCutsOne( p, pCut, pCut->pLeaves[k] );
            // quit if the number of cuts has exceeded the limit
            if ( p->nCuts == LPK_CUTS_MAX )
                break;
        }
        if ( p->nCuts == LPK_CUTS_MAX )
            break;
    }
    if ( p->nCuts == LPK_CUTS_MAX ) 
        p->nNodesOver++;

    // record the impact of this node
    if ( p->pPars->fSatur )
        Lpk_NodeRecordImpact( p );

    // compress the cuts by removing empty ones, those with negative Weight, and decomposable ones
    p->nEvals = 0;
    for ( i = 0; i < p->nCuts; i++ )
    {
        pCut = p->pCuts + i;
        if ( pCut->nLeaves < 2 )
            continue;
        // compute the minimum number of LUTs needed to implement this cut
        // V = N * (K-1) + 1  ~~~~~  N = Ceiling[(V-1)/(K-1)] = (V-1)/(K-1) + [(V-1)%(K-1) > 0]
        pCut->nLuts = Lpk_LutNumLuts( pCut->nLeaves, p->pPars->nLutSize ); 
//        pCut->Weight = (float)1.0 * (pCut->nNodes - pCut->nNodesDup - 1) / pCut->nLuts; //p->pPars->nLutsMax;
        pCut->Weight = (float)1.0 * (pCut->nNodes - pCut->nNodesDup) / pCut->nLuts; //p->pPars->nLutsMax;
        if ( pCut->Weight <= 1.001 )
//        if ( pCut->Weight <= 0.999 )
            continue;
        pCut->fHasDsd = Lpk_NodeCutsCheckDsd( p, pCut );
        if ( pCut->fHasDsd )
            continue;
        p->pEvals[p->nEvals++] = i;

//        if ( pCut->nLeaves <= 9 && pCut->nNodes > 15 )
//        printf( "%5d : Obj = %4d  Leaves = %4d  Nodes = %4d  LUTs = %4d\n", i, p->pObj->Id, pCut->nLeaves, pCut->nNodes, pCut->nLuts );
    }
    if ( p->nEvals == 0 )
        return 0;

    // sort the cuts by Weight
    do {
        fChanges = 0;
        for ( i = 0; i < p->nEvals - 1; i++ )
        {
            pCut = p->pCuts + p->pEvals[i];
            pCut2 = p->pCuts + p->pEvals[i+1];
            if ( pCut->Weight >= pCut2->Weight - 0.001 )
                continue;
            Temp = p->pEvals[i];
            p->pEvals[i] = p->pEvals[i+1];
            p->pEvals[i+1] = Temp;
            fChanges = 1;
        }
    } while ( fChanges );
/*
    for ( i = 0; i < p->nEvals; i++ )
    {
        pCut = p->pCuts + p->pEvals[i];
        printf( "Cut %3d : W = %5.2f.\n", i, pCut->Weight );
    }
    printf( "\n" );
*/
    return 1;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

