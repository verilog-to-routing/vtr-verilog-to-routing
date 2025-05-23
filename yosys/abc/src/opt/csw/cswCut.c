/**CFile****************************************************************

  FileName    [cswCut.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Cut sweeping.]

  Synopsis    []

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - July 11, 2007.]

  Revision    [$Id: cswCut.c,v 1.00 2007/07/11 00:00:00 alanmi Exp $]

***********************************************************************/

#include "cswInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Compute the cost of the cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Csw_CutFindCost( Csw_Man_t * p, Csw_Cut_t * pCut )
{
    Aig_Obj_t * pLeaf;
    int i, Cost = 0;
    assert( pCut->nFanins > 0 );
    Csw_CutForEachLeaf( p->pManRes, pCut, pLeaf, i )
    {
//        Cost += pLeaf->nRefs;
        Cost += Csw_ObjRefs( p, pLeaf );
//        printf( "%d ", pLeaf->nRefs );
    }
//printf( "\n" );
    return Cost * 100 / pCut->nFanins;
}

/**Function*************************************************************

  Synopsis    [Compute the cost of the cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline float Csw_CutFindCost2( Csw_Man_t * p, Csw_Cut_t * pCut )
{
    Aig_Obj_t * pLeaf;
    float Cost = 0.0;
    int i;
    assert( pCut->nFanins > 0 );
    Csw_CutForEachLeaf( p->pManRes, pCut, pLeaf, i )
        Cost += (float)1.0/pLeaf->nRefs;
    return 1/Cost;
}

/**Function*************************************************************

  Synopsis    [Returns the next free cut to use.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Csw_Cut_t * Csw_CutFindFree( Csw_Man_t * p, Aig_Obj_t * pObj )
{
    Csw_Cut_t * pCut, * pCutMax;
    int i;
    pCutMax = NULL;
    Csw_ObjForEachCut( p, pObj, pCut, i )
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
static inline unsigned Cut_TruthPhase( Csw_Cut_t * pCut, Csw_Cut_t * pCut1 )
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
unsigned * Csw_CutComputeTruth( Csw_Man_t * p, Csw_Cut_t * pCut, Csw_Cut_t * pCut0, Csw_Cut_t * pCut1, int fCompl0, int fCompl1 )
{
    // permute the first table
    if ( fCompl0 ) 
        Kit_TruthNot( p->puTemp[0], Csw_CutTruth(pCut0), p->nLeafMax );
    else
        Kit_TruthCopy( p->puTemp[0], Csw_CutTruth(pCut0), p->nLeafMax );
    Kit_TruthStretch( p->puTemp[2], p->puTemp[0], pCut0->nFanins, p->nLeafMax, Cut_TruthPhase(pCut, pCut0), 0 );
    // permute the second table
    if ( fCompl1 ) 
        Kit_TruthNot( p->puTemp[1], Csw_CutTruth(pCut1), p->nLeafMax );
    else
        Kit_TruthCopy( p->puTemp[1], Csw_CutTruth(pCut1), p->nLeafMax );
    Kit_TruthStretch( p->puTemp[3], p->puTemp[1], pCut1->nFanins, p->nLeafMax, Cut_TruthPhase(pCut, pCut1), 0 );
    // produce the resulting table
    Kit_TruthAnd( Csw_CutTruth(pCut), p->puTemp[2], p->puTemp[3], p->nLeafMax );
//    assert( pCut->nFanins >= Kit_TruthSupportSize( Csw_CutTruth(pCut), p->nLeafMax ) );
    return Csw_CutTruth(pCut);
}

/**Function*************************************************************

  Synopsis    [Performs support minimization for the truth table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Csw_CutSupportMinimize( Csw_Man_t * p, Csw_Cut_t * pCut )
{
    unsigned * pTruth;
    int uSupp, nFansNew, i, k;
    // get truth table
    pTruth = Csw_CutTruth( pCut );
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
static inline int Csw_CutCheckDominance( Csw_Cut_t * pDom, Csw_Cut_t * pCut )
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
int Csw_CutFilter( Csw_Man_t * p, Aig_Obj_t * pObj, Csw_Cut_t * pCut )
{ 
    Csw_Cut_t * pTemp;
    int i;
    // go through the cuts of the node
    Csw_ObjForEachCut( p, pObj, pTemp, i )
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
            if ( Csw_CutCheckDominance( pCut, pTemp ) )
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
            if ( Csw_CutCheckDominance( pTemp, pCut ) )
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
static inline int Csw_CutMergeOrdered( Csw_Man_t * p, Csw_Cut_t * pC0, Csw_Cut_t * pC1, Csw_Cut_t * pC )
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
int Csw_CutMerge( Csw_Man_t * p, Csw_Cut_t * pCut0, Csw_Cut_t * pCut1, Csw_Cut_t * pCut )
{ 
    assert( p->nLeafMax > 0 );
    // merge the nodes
    if ( pCut0->nFanins < pCut1->nFanins )
    {
        if ( !Csw_CutMergeOrdered( p, pCut1, pCut0, pCut ) )
            return 0;
    }
    else
    {
        if ( !Csw_CutMergeOrdered( p, pCut0, pCut1, pCut ) )
            return 0;
    }
    pCut->uSign = pCut0->uSign | pCut1->uSign;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Consider cut with more than 2 fanins having 2 true variables.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Obj_t * Csw_ObjTwoVarCut( Csw_Man_t * p, Csw_Cut_t * pCut )
{
    Aig_Obj_t * pRes, * pIn0, * pIn1;
    int nVars, uTruth, fCompl = 0;
    assert( pCut->nFanins > 2 );
    // minimize support of this cut
    nVars = Csw_CutSupportMinimize( p, pCut );
    assert( nVars == 2 );
    // get the fanins
    pIn0 = Aig_ManObj( p->pManRes, pCut->pFanins[0] );
    pIn1 = Aig_ManObj( p->pManRes, pCut->pFanins[1] );
    // derive the truth table
    uTruth = 0xF & *Csw_CutTruth(pCut);
    if ( uTruth == 14 || uTruth == 13 || uTruth == 11 || uTruth == 7 )
    {
        uTruth = 0xF & ~uTruth;
        fCompl = 1;
    }
    // compute the result
    pRes = NULL;
    if ( uTruth == 1  )  // 0001  // 1110  14
        pRes = Aig_And( p->pManRes, Aig_Not(pIn0), Aig_Not(pIn1) );
    if ( uTruth == 2  )  // 0010  // 1101  13 
        pRes = Aig_And( p->pManRes,         pIn0 , Aig_Not(pIn1) );
    if ( uTruth == 4  )  // 0100  // 1011  11
        pRes = Aig_And( p->pManRes, Aig_Not(pIn0),         pIn1  );
    if ( uTruth == 8  )  // 1000  // 0111   7
        pRes = Aig_And( p->pManRes,         pIn0 ,         pIn1  );
    if ( pRes )
        pRes = Aig_NotCond( pRes, fCompl );
    return pRes;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Csw_Cut_t * Csw_ObjPrepareCuts( Csw_Man_t * p, Aig_Obj_t * pObj, int fTriv )
{
    Csw_Cut_t * pCutSet, * pCut;
    int i;
    // create the cutset of the node
    pCutSet = (Csw_Cut_t *)Aig_MmFixedEntryFetch( p->pMemCuts );
    Csw_ObjSetCuts( p, pObj, pCutSet );
    Csw_ObjForEachCut( p, pObj, pCut, i )
    {
        pCut->nFanins = 0;
        pCut->iNode = pObj->Id;
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
        memset( Csw_CutTruth(pCut), 0xAA, sizeof(unsigned) * p->nTruthWords );
    }
    return pCutSet;
}

/**Function*************************************************************

  Synopsis    [Derives cuts for one node and sweeps this node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Obj_t * Csw_ObjSweep( Csw_Man_t * p, Aig_Obj_t * pObj, int fTriv )
{
    int fUseResub = 1;
    Csw_Cut_t * pCut0, * pCut1, * pCut, * pCutSet;
    Aig_Obj_t * pFanin0 = Aig_ObjFanin0(pObj);
    Aig_Obj_t * pFanin1 = Aig_ObjFanin1(pObj);
    Aig_Obj_t * pObjNew;
    unsigned * pTruth;
    int i, k, nVars, nFanins, iVar;
    abctime clk;

    assert( !Aig_IsComplement(pObj) );
    if ( !Aig_ObjIsNode(pObj) )
        return pObj;
    if ( Csw_ObjCuts(p, pObj) )
        return pObj;
    // the node is not processed yet
    assert( Csw_ObjCuts(p, pObj) == NULL );
    assert( Aig_ObjIsNode(pObj) );

    // set up the first cut
    pCutSet = Csw_ObjPrepareCuts( p, pObj, fTriv );

    // compute pair-wise cut combinations while checking table
    Csw_ObjForEachCut( p, pFanin0, pCut0, i )
    if ( pCut0->nFanins > 0 )
    Csw_ObjForEachCut( p, pFanin1, pCut1, k )
    if ( pCut1->nFanins > 0 )
    {
        // make sure K-feasible cut exists
        if ( Kit_WordCountOnes(pCut0->uSign | pCut1->uSign) > p->nLeafMax )
            continue;
        // get the next cut of this node
        pCut = Csw_CutFindFree( p, pObj );
clk = Abc_Clock();
        // assemble the new cut
        if ( !Csw_CutMerge( p, pCut0, pCut1, pCut ) )
        {
            assert( pCut->nFanins == 0 );
            continue;
        }
        // check containment
        if ( Csw_CutFilter( p, pObj, pCut ) )
        {
            assert( pCut->nFanins == 0 );
            continue;
        }
        // create its truth table
        pTruth = Csw_CutComputeTruth( p, pCut, pCut0, pCut1, Aig_ObjFaninC0(pObj), Aig_ObjFaninC1(pObj) );
        // support minimize the truth table
        nFanins = pCut->nFanins;
//        nVars = Csw_CutSupportMinimize( p, pCut ); // leads to quality degradation
        nVars = Kit_TruthSupportSize( pTruth, p->nLeafMax );
p->timeCuts += Abc_Clock() - clk;

        // check for trivial truth tables
        if ( nVars == 0 )
        {
            p->nNodesTriv0++;
            return Aig_NotCond( Aig_ManConst1(p->pManRes), !(pTruth[0] & 1) );
        }
        if ( nVars == 1 )
        {
            p->nNodesTriv1++;
            iVar = Kit_WordFindFirstBit( Kit_TruthSupport(pTruth, p->nLeafMax) );
            assert( iVar < pCut->nFanins );
            return Aig_NotCond( Aig_ManObj(p->pManRes, pCut->pFanins[iVar]), (pTruth[0] & 1) );
        }
        if ( nVars == 2 && nFanins > 2 && fUseResub )
        {
            if ( (pObjNew = Csw_ObjTwoVarCut( p, pCut )) )
            {
                p->nNodesTriv2++;
                return pObjNew;
            }
        }

        // check if an equivalent node with the same cut exists
clk = Abc_Clock();
        pObjNew = pCut->nFanins > 2 ? Csw_TableCutLookup( p, pCut ) : NULL;
p->timeHash += Abc_Clock() - clk;
        if ( pObjNew )
        {
            p->nNodesCuts++;
            return pObjNew;
        }

        // assign the cost
        pCut->Cost = Csw_CutFindCost( p, pCut );
        assert( pCut->nFanins > 0 );
        assert( pCut->Cost > 0 );
    }
    p->nNodesTried++;

    // load the resulting cuts into the table
clk = Abc_Clock();
    Csw_ObjForEachCut( p, pObj, pCut, i )
    {
        if ( pCut->nFanins > 2 )
        {
            assert( pCut->Cost > 0 );
            Csw_TableCutInsert( p, pCut );
        }
    }
p->timeHash += Abc_Clock() - clk;

    // return the node if could not replace it
    return pObj;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

