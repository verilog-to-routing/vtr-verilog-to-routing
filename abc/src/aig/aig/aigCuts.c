/**CFile****************************************************************

  FileName    [aigCuts.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [AIG package.]

  Synopsis    [Computation of K-feasible priority cuts.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - April 28, 2007.]

  Revision    [$Id: aigCuts.c,v 1.00 2007/04/28 00:00:00 alanmi Exp $]

***********************************************************************/

#include "aig.h"
#include "bool/kit/kit.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Starts the cut sweeping manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_ManCut_t * Aig_ManCutStart( Aig_Man_t * pMan, int nCutsMax, int nLeafMax, int fTruth, int fVerbose )
{
    Aig_ManCut_t * p;
    assert( nCutsMax >= 2  );
    assert( nLeafMax <= 16 );
    // allocate the fraiging manager
    p = ABC_ALLOC( Aig_ManCut_t, 1 );
    memset( p, 0, sizeof(Aig_ManCut_t) );
    p->nCutsMax = nCutsMax;
    p->nLeafMax = nLeafMax;
    p->fTruth   = fTruth;
    p->fVerbose = fVerbose;
    p->pAig     = pMan;
    p->pCuts    = ABC_CALLOC( Aig_Cut_t *, Aig_ManObjNumMax(pMan) );
    // allocate memory manager
    p->nTruthWords = Abc_TruthWordNum(nLeafMax);
    p->nCutSize = sizeof(Aig_Cut_t) + sizeof(int) * nLeafMax + fTruth * sizeof(unsigned) * p->nTruthWords;
    p->pMemCuts = Aig_MmFixedStart( p->nCutSize * p->nCutsMax, 512 );
    // room for temporary truth tables
    if ( fTruth )
    {
        p->puTemp[0] = ABC_ALLOC( unsigned, 4 * p->nTruthWords );
        p->puTemp[1] = p->puTemp[0] + p->nTruthWords;
        p->puTemp[2] = p->puTemp[1] + p->nTruthWords;
        p->puTemp[3] = p->puTemp[2] + p->nTruthWords;
    }
    return p;
}

/**Function*************************************************************

  Synopsis    [Stops the fraiging manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ManCutStop( Aig_ManCut_t * p )
{
    Aig_MmFixedStop( p->pMemCuts, 0 );
    ABC_FREE( p->puTemp[0] );
    ABC_FREE( p->pCuts );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    [Prints one cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_CutPrint( Aig_Cut_t * pCut )
{
    int i;
    printf( "{" );
    for ( i = 0; i < pCut->nFanins; i++ )
        printf( " %d", pCut->pFanins[i] );
    printf( " }\n" );
}

/**Function*************************************************************

  Synopsis    [Prints one cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ObjCutPrint( Aig_ManCut_t * p, Aig_Obj_t * pObj )
{
    Aig_Cut_t * pCut;
    int i;
    printf( "Cuts for node %d:\n", pObj->Id );
    Aig_ObjForEachCut( p, pObj, pCut, i )
        if ( pCut->nFanins )
            Aig_CutPrint( pCut );
//    printf( "\n" );
}

/**Function*************************************************************

  Synopsis    [Computes the total number of cuts.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aig_ManCutCount( Aig_ManCut_t * p, int * pnCutsK )
{
    Aig_Cut_t * pCut;
    Aig_Obj_t * pObj;
    int i, k, nCuts = 0, nCutsK = 0;
    Aig_ManForEachNode( p->pAig, pObj, i )
        Aig_ObjForEachCut( p, pObj, pCut, k )
        {
            if ( pCut->nFanins == 0 )
                continue;
            nCuts++;
            if ( pCut->nFanins == p->nLeafMax )
                nCutsK++;
        }
    if ( pnCutsK )
        *pnCutsK = nCutsK;
    return nCuts;
}

/**Function*************************************************************

  Synopsis    [Compute the cost of the cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Aig_CutFindCost( Aig_ManCut_t * p, Aig_Cut_t * pCut )
{
    Aig_Obj_t * pLeaf;
    int i, Cost = 0;
    assert( pCut->nFanins > 0 );
    Aig_CutForEachLeaf( p->pAig, pCut, pLeaf, i )
        Cost += pLeaf->nRefs;
    return Cost * 1000 / pCut->nFanins;
}

/**Function*************************************************************

  Synopsis    [Compute the cost of the cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline float Aig_CutFindCost2( Aig_ManCut_t * p, Aig_Cut_t * pCut )
{
    Aig_Obj_t * pLeaf;
    float Cost = 0.0;
    int i;
    assert( pCut->nFanins > 0 );
    Aig_CutForEachLeaf( p->pAig, pCut, pLeaf, i )
        Cost += (float)1.0/pLeaf->nRefs;
    return 1/Cost;
}

/**Function*************************************************************

  Synopsis    [Returns the next free cut to use.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Aig_Cut_t * Aig_CutFindFree( Aig_ManCut_t * p, Aig_Obj_t * pObj )
{
    Aig_Cut_t * pCut, * pCutMax;
    int i;
    pCutMax = NULL;
    Aig_ObjForEachCut( p, pObj, pCut, i )
    {
        if ( pCut->nFanins == 0 )
            return pCut;
        if ( pCutMax == NULL || pCutMax->Cost < pCut->Cost )
            pCutMax = pCut;
    }
    assert( pCutMax != NULL );
    pCutMax->nFanins = 0;
    return pCutMax;
}

/**Function*************************************************************

  Synopsis    [Computes the stretching phase of the cut w.r.t. the merged cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline unsigned Aig_CutTruthPhase( Aig_Cut_t * pCut, Aig_Cut_t * pCut1 )
{
    unsigned uPhase = 0;
    int i, k;
    for ( i = k = 0; i < pCut->nFanins; i++ )
    {
        if ( k == pCut1->nFanins )
            break;
        if ( pCut->pFanins[i] < pCut1->pFanins[k] )
            continue;
        assert( pCut->pFanins[i] == pCut1->pFanins[k] );
        uPhase |= (1 << i);
        k++;
    }
    return uPhase;
}

/**Function*************************************************************

  Synopsis    [Performs truth table computation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned * Aig_CutComputeTruth( Aig_ManCut_t * p, Aig_Cut_t * pCut, Aig_Cut_t * pCut0, Aig_Cut_t * pCut1, int fCompl0, int fCompl1 )
{
    // permute the first table
    if ( fCompl0 ) 
        Kit_TruthNot( p->puTemp[0], Aig_CutTruth(pCut0), p->nLeafMax );
    else
        Kit_TruthCopy( p->puTemp[0], Aig_CutTruth(pCut0), p->nLeafMax );
    Kit_TruthStretch( p->puTemp[2], p->puTemp[0], pCut0->nFanins, p->nLeafMax, Aig_CutTruthPhase(pCut, pCut0), 0 );
    // permute the second table
    if ( fCompl1 ) 
        Kit_TruthNot( p->puTemp[1], Aig_CutTruth(pCut1), p->nLeafMax );
    else
        Kit_TruthCopy( p->puTemp[1], Aig_CutTruth(pCut1), p->nLeafMax );
    Kit_TruthStretch( p->puTemp[3], p->puTemp[1], pCut1->nFanins, p->nLeafMax, Aig_CutTruthPhase(pCut, pCut1), 0 );
    // produce the resulting table
    Kit_TruthAnd( Aig_CutTruth(pCut), p->puTemp[2], p->puTemp[3], p->nLeafMax );
//    assert( pCut->nFanins >= Kit_TruthSupportSize( Aig_CutTruth(pCut), p->nLeafMax ) );
    return Aig_CutTruth(pCut);
}

/**Function*************************************************************

  Synopsis    [Performs support minimization for the truth table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aig_CutSupportMinimize( Aig_ManCut_t * p, Aig_Cut_t * pCut )
{
    unsigned * pTruth;
    int uSupp, nFansNew, i, k;
    // get truth table
    pTruth = Aig_CutTruth( pCut );
    // get support 
    uSupp = Kit_TruthSupport( pTruth, p->nLeafMax );
    // get the new support size
    nFansNew = Kit_WordCountOnes( uSupp );
    // check if there are redundant variables
    if ( nFansNew == pCut->nFanins )
        return nFansNew;
    assert( nFansNew < pCut->nFanins );
    // minimize support
    Kit_TruthShrink( p->puTemp[0], pTruth, nFansNew, p->nLeafMax, uSupp, 1 );
    for ( i = k = 0; i < pCut->nFanins; i++ )
        if ( uSupp & (1 << i) )
            pCut->pFanins[k++] = pCut->pFanins[i];
    assert( k == nFansNew );
    pCut->nFanins = nFansNew;
//    assert( nFansNew == Kit_TruthSupportSize( pTruth, p->nLeafMax ) );
//Extra_PrintBinary( stdout, pTruth, (1<<p->nLeafMax) ); printf( "\n" );
    return nFansNew;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if pDom is contained in pCut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Aig_CutCheckDominance( Aig_Cut_t * pDom, Aig_Cut_t * pCut )
{
    int i, k;
    for ( i = 0; i < (int)pDom->nFanins; i++ )
    {
        for ( k = 0; k < (int)pCut->nFanins; k++ )
            if ( pDom->pFanins[i] == pCut->pFanins[k] )
                break;
        if ( k == (int)pCut->nFanins ) // node i in pDom is not contained in pCut
            return 0;
    }
    // every node in pDom is contained in pCut
    return 1;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if the cut is contained.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aig_CutFilter( Aig_ManCut_t * p, Aig_Obj_t * pObj, Aig_Cut_t * pCut )
{ 
    Aig_Cut_t * pTemp;
    int i;
    // go through the cuts of the node
    Aig_ObjForEachCut( p, pObj, pTemp, i )
    {
        if ( pTemp->nFanins < 2 )
            continue;
        if ( pTemp == pCut )
            continue;
        if ( pTemp->nFanins > pCut->nFanins )
        {
            // skip the non-contained cuts
            if ( (pTemp->uSign & pCut->uSign) != pCut->uSign )
                continue;
            // check containment seriously
            if ( Aig_CutCheckDominance( pCut, pTemp ) )
            {
                // remove contained cut
                pTemp->nFanins = 0;
            }
         }
        else
        {
            // skip the non-contained cuts
            if ( (pTemp->uSign & pCut->uSign) != pTemp->uSign )
                continue;
            // check containment seriously
            if ( Aig_CutCheckDominance( pTemp, pCut ) )
            {
                // remove the given
                pCut->nFanins = 0;
                return 1;
            }
        }
    }
    return 0;
}

/**Function*************************************************************

  Synopsis    [Merges two cuts.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Aig_CutMergeOrdered( Aig_ManCut_t * p, Aig_Cut_t * pC0, Aig_Cut_t * pC1, Aig_Cut_t * pC )
{ 
    int i, k, c;
    assert( pC0->nFanins >= pC1->nFanins );
    // the case of the largest cut sizes
    if ( pC0->nFanins == p->nLeafMax && pC1->nFanins == p->nLeafMax )
    {
        for ( i = 0; i < pC0->nFanins; i++ )
            if ( pC0->pFanins[i] != pC1->pFanins[i] )
                return 0;
        for ( i = 0; i < pC0->nFanins; i++ )
            pC->pFanins[i] = pC0->pFanins[i];
        pC->nFanins = pC0->nFanins;
        return 1;
    }
    // the case when one of the cuts is the largest
    if ( pC0->nFanins == p->nLeafMax )
    {
        for ( i = 0; i < pC1->nFanins; i++ )
        {
            for ( k = pC0->nFanins - 1; k >= 0; k-- )
                if ( pC0->pFanins[k] == pC1->pFanins[i] )
                    break;
            if ( k == -1 ) // did not find
                return 0;
        }
        for ( i = 0; i < pC0->nFanins; i++ )
            pC->pFanins[i] = pC0->pFanins[i];
        pC->nFanins = pC0->nFanins;
        return 1;
    }

    // compare two cuts with different numbers
    i = k = 0;
    for ( c = 0; c < p->nLeafMax; c++ )
    {
        if ( k == pC1->nFanins )
        {
            if ( i == pC0->nFanins )
            {
                pC->nFanins = c;
                return 1;
            }
            pC->pFanins[c] = pC0->pFanins[i++];
            continue;
        }
        if ( i == pC0->nFanins )
        {
            if ( k == pC1->nFanins )
            {
                pC->nFanins = c;
                return 1;
            }
            pC->pFanins[c] = pC1->pFanins[k++];
            continue;
        }
        if ( pC0->pFanins[i] < pC1->pFanins[k] )
        {
            pC->pFanins[c] = pC0->pFanins[i++];
            continue;
        }
        if ( pC0->pFanins[i] > pC1->pFanins[k] )
        {
            pC->pFanins[c] = pC1->pFanins[k++];
            continue;
        }
        pC->pFanins[c] = pC0->pFanins[i++]; 
        k++;
    }
    if ( i < pC0->nFanins || k < pC1->nFanins )
        return 0;
    pC->nFanins = c;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Prepares the object for FPGA mapping.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aig_CutMerge( Aig_ManCut_t * p, Aig_Cut_t * pCut0, Aig_Cut_t * pCut1, Aig_Cut_t * pCut )
{ 
    assert( p->nLeafMax > 0 );
    // merge the nodes
    if ( pCut0->nFanins < pCut1->nFanins )
    {
        if ( !Aig_CutMergeOrdered( p, pCut1, pCut0, pCut ) )
            return 0;
    }
    else
    {
        if ( !Aig_CutMergeOrdered( p, pCut0, pCut1, pCut ) )
            return 0;
    }
    pCut->uSign = pCut0->uSign | pCut1->uSign;
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Cut_t * Aig_ObjPrepareCuts( Aig_ManCut_t * p, Aig_Obj_t * pObj, int fTriv )
{
    Aig_Cut_t * pCutSet, * pCut;
    int i;
    // create the cutset of the node
    pCutSet = (Aig_Cut_t *)Aig_MmFixedEntryFetch( p->pMemCuts );
    Aig_ObjSetCuts( p, pObj, pCutSet );
    Aig_ObjForEachCut( p, pObj, pCut, i )
    {
        pCut->nFanins  = 0;
        pCut->iNode    = pObj->Id;
        pCut->nCutSize = p->nCutSize;
        pCut->nLeafMax = p->nLeafMax;
    }
    // add unit cut if needed
    if ( fTriv )
    {
        pCut = pCutSet;
        pCut->Cost = 0;
        pCut->iNode = pObj->Id;
        pCut->nFanins = 1;
        pCut->pFanins[0] = pObj->Id;
        pCut->uSign = Aig_ObjCutSign( pObj->Id );
        if ( p->fTruth )
        memset( Aig_CutTruth(pCut), 0xAA, sizeof(unsigned) * p->nTruthWords );
    }
    return pCutSet;
}

/**Function*************************************************************

  Synopsis    [Derives cuts for one node and sweeps this node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ObjComputeCuts( Aig_ManCut_t * p, Aig_Obj_t * pObj, int fTriv )
{
    Aig_Cut_t * pCut0, * pCut1, * pCut, * pCutSet;
    Aig_Obj_t * pFanin0 = Aig_ObjFanin0(pObj);
    Aig_Obj_t * pFanin1 = Aig_ObjFanin1(pObj);
    int i, k;
    // the node is not processed yet
    assert( Aig_ObjIsNode(pObj) );
    assert( Aig_ObjCuts(p, pObj) == NULL );
    // set up the first cut
    pCutSet = Aig_ObjPrepareCuts( p, pObj, fTriv );
    // compute pair-wise cut combinations while checking table
    Aig_ObjForEachCut( p, pFanin0, pCut0, i )
    if ( pCut0->nFanins > 0 )
    Aig_ObjForEachCut( p, pFanin1, pCut1, k )
    if ( pCut1->nFanins > 0 )
    {
        // make sure K-feasible cut exists
        if ( Kit_WordCountOnes(pCut0->uSign | pCut1->uSign) > p->nLeafMax )
            continue;
        // get the next cut of this node
        pCut = Aig_CutFindFree( p, pObj );
        // assemble the new cut
        if ( !Aig_CutMerge( p, pCut0, pCut1, pCut ) )
        {
            assert( pCut->nFanins == 0 );
            continue;
        }
        // check containment
        if ( Aig_CutFilter( p, pObj, pCut ) )
        {
            assert( pCut->nFanins == 0 );
            continue;
        }
        // create its truth table
        if ( p->fTruth )
            Aig_CutComputeTruth( p, pCut, pCut0, pCut1, Aig_ObjFaninC0(pObj), Aig_ObjFaninC1(pObj) );
        // assign the cost
        pCut->Cost = Aig_CutFindCost( p, pCut );
        assert( pCut->nFanins > 0 );
        assert( pCut->Cost > 0 );
    }
}

/**Function*************************************************************

  Synopsis    [Computes the cuts for all nodes in the static AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_ManCut_t * Aig_ComputeCuts( Aig_Man_t * pAig, int nCutsMax, int nLeafMax, int fTruth, int fVerbose )
{
    Aig_ManCut_t * p;
    Aig_Obj_t * pObj;
    int i;
    abctime clk = Abc_Clock();
    assert( pAig->pManCuts == NULL );
    // start the manager
    p = Aig_ManCutStart( pAig, nCutsMax, nLeafMax, fTruth, fVerbose );
    // set elementary cuts at the PIs
    Aig_ManForEachCi( pAig, pObj, i )
        Aig_ObjPrepareCuts( p, pObj, 1 );
    // process the nodes
    Aig_ManForEachNode( pAig, pObj, i )
        Aig_ObjComputeCuts( p, pObj, 1 );
    // print stats
    if ( fVerbose )
    {
        int nCuts, nCutsK;
        nCuts = Aig_ManCutCount( p, &nCutsK );
        printf( "Nodes = %6d. Total cuts = %6d. %d-input cuts = %6d.\n",
            Aig_ManObjNum(pAig), nCuts, nLeafMax, nCutsK );
        printf( "Cut size = %2d. Truth size = %2d. Total mem = %5.2f MB  ",
            p->nCutSize, 4*p->nTruthWords, 1.0*Aig_MmFixedReadMemUsage(p->pMemCuts)/(1<<20) );
        ABC_PRT( "Runtime", Abc_Clock() - clk );
/*
        Aig_ManForEachNode( pAig, pObj, i )
            if ( i % 300 == 0 )
                Aig_ObjCutPrint( p, pObj );
*/
    }
    // remember the cut manager
    pAig->pManCuts = p;
    return p;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

