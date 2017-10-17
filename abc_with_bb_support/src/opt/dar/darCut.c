/**CFile****************************************************************

  FileName    [darCut.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [DAG-aware AIG rewriting.]

  Synopsis    [Computation of 4-input cuts.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - April 28, 2007.]

  Revision    [$Id: darCut.c,v 1.00 2007/04/28 00:00:00 alanmi Exp $]

***********************************************************************/

#include "darInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Prints one cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Dar_CutPrint( Dar_Cut_t * pCut )
{
    unsigned i;
    printf( "{" );
    for ( i = 0; i < pCut->nLeaves; i++ )
        printf( " %d", pCut->pLeaves[i] );
    printf( " }\n" );
}

/**Function*************************************************************

  Synopsis    [Prints one cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Dar_ObjCutPrint( Aig_Man_t * p, Aig_Obj_t * pObj )
{
    Dar_Cut_t * pCut;
    int i;
    printf( "Cuts for node %d:\n", pObj->Id );
    Dar_ObjForEachCut( pObj, pCut, i )
        Dar_CutPrint( pCut );
//    printf( "\n" );
}

/**Function*************************************************************

  Synopsis    [Returns the number of 1s in the machine word.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Dar_WordCountOnes( unsigned uWord )
{
    uWord = (uWord & 0x55555555) + ((uWord>>1) & 0x55555555);
    uWord = (uWord & 0x33333333) + ((uWord>>2) & 0x33333333);
    uWord = (uWord & 0x0F0F0F0F) + ((uWord>>4) & 0x0F0F0F0F);
    uWord = (uWord & 0x00FF00FF) + ((uWord>>8) & 0x00FF00FF);
    return  (uWord & 0x0000FFFF) + (uWord>>16);
}

/**Function*************************************************************

  Synopsis    [Compute the cost of the cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Dar_CutFindValue( Dar_Man_t * p, Dar_Cut_t * pCut )
{
    Aig_Obj_t * pLeaf;
    int i, Value, nOnes;
    assert( pCut->fUsed );
    Value = 0;
    nOnes = 0;
    Dar_CutForEachLeaf( p->pAig, pCut, pLeaf, i )
    {
        if ( pLeaf == NULL )
            return 0;
        assert( pLeaf != NULL );
        Value += pLeaf->nRefs;
        nOnes += (pLeaf->nRefs == 1);
    }
    if ( pCut->nLeaves < 2 )
        return 1001;
//    Value = Value * 100 / pCut->nLeaves;
    if ( Value > 1000 )
        Value = 1000;
    if ( nOnes > 3 )
        Value = 5 - nOnes;
    return Value;
}

/**Function*************************************************************

  Synopsis    [Returns the next free cut to use.]

  Description [Uses the cut with the smallest value.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Dar_Cut_t * Dar_CutFindFree( Dar_Man_t * p, Aig_Obj_t * pObj )
{
    Dar_Cut_t * pCut, * pCutMax;
    int i;
    pCutMax = NULL;
    Dar_ObjForEachCutAll( pObj, pCut, i )
    {
        if ( pCut->fUsed == 0 )
            return pCut;
        if ( pCut->nLeaves < 3 )
            continue;
        if ( pCutMax == NULL || pCutMax->Value > pCut->Value )
            pCutMax = pCut;
    }
    if ( pCutMax == NULL )
    {
        Dar_ObjForEachCutAll( pObj, pCut, i )
        {
            if ( pCut->nLeaves < 2 )
                continue;
            if ( pCutMax == NULL || pCutMax->Value > pCut->Value )
                pCutMax = pCut;
        }
    }
    if ( pCutMax == NULL )
    {
        Dar_ObjForEachCutAll( pObj, pCut, i )
        {
            if ( pCutMax == NULL || pCutMax->Value > pCut->Value )
                pCutMax = pCut;
        }
    }
    assert( pCutMax != NULL );
    pCutMax->fUsed = 0;
    return pCutMax;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if pDom is contained in pCut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Dar_CutCheckDominance( Dar_Cut_t * pDom, Dar_Cut_t * pCut )
{
    int i, k;
    assert( pDom->fUsed && pCut->fUsed );
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

  Synopsis    [Returns 1 if the cut is contained.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Dar_CutFilter( Aig_Obj_t * pObj, Dar_Cut_t * pCut )
{ 
    Dar_Cut_t * pTemp;
    int i;
    assert( pCut->fUsed );
    // go through the cuts of the node
    Dar_ObjForEachCut( pObj, pTemp, i )
    {
        if ( pTemp == pCut )
            continue;
        if ( pTemp->nLeaves > pCut->nLeaves )
        {
            // skip the non-contained cuts
            if ( (pTemp->uSign & pCut->uSign) != pCut->uSign )
                continue;
            // check containment seriously
            if ( Dar_CutCheckDominance( pCut, pTemp ) )
            {
                // remove contained cut
                pTemp->fUsed = 0;
            }
         }
        else
        {
            // skip the non-contained cuts
            if ( (pTemp->uSign & pCut->uSign) != pTemp->uSign )
                continue;
            // check containment seriously
            if ( Dar_CutCheckDominance( pTemp, pCut ) )
            {
                // remove the given cut
                pCut->fUsed = 0;
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
static inline int Dar_CutMergeOrdered( Dar_Cut_t * pC, Dar_Cut_t * pC0, Dar_Cut_t * pC1 )
{ 
    int i, k, c;
    assert( pC0->nLeaves >= pC1->nLeaves );

    // the case of the largest cut sizes
    if ( pC0->nLeaves == 4 && pC1->nLeaves == 4 )
    {
        if ( pC0->uSign != pC1->uSign )
            return 0;
        for ( i = 0; i < (int)pC0->nLeaves; i++ )
            if ( pC0->pLeaves[i] != pC1->pLeaves[i] )
                return 0;
        for ( i = 0; i < (int)pC0->nLeaves; i++ )
            pC->pLeaves[i] = pC0->pLeaves[i];
        pC->nLeaves = pC0->nLeaves;
        return 1;
    }

    // the case when one of the cuts is the largest
    if ( pC0->nLeaves == 4 )
    {
        if ( (pC0->uSign & pC1->uSign) != pC1->uSign )
            return 0;
        for ( i = 0; i < (int)pC1->nLeaves; i++ )
        {
            for ( k = (int)pC0->nLeaves - 1; k >= 0; k-- )
                if ( pC0->pLeaves[k] == pC1->pLeaves[i] )
                    break;
            if ( k == -1 ) // did not find
                return 0;
        }
        for ( i = 0; i < (int)pC0->nLeaves; i++ )
            pC->pLeaves[i] = pC0->pLeaves[i];
        pC->nLeaves = pC0->nLeaves;
        return 1;
    }

    // compare two cuts with different numbers
    i = k = 0;
    for ( c = 0; c < 4; c++ )
    {
        if ( k == (int)pC1->nLeaves )
        {
            if ( i == (int)pC0->nLeaves )
            {
                pC->nLeaves = c;
                return 1;
            }
            pC->pLeaves[c] = pC0->pLeaves[i++];
            continue;
        }
        if ( i == (int)pC0->nLeaves )
        {
            if ( k == (int)pC1->nLeaves )
            {
                pC->nLeaves = c;
                return 1;
            }
            pC->pLeaves[c] = pC1->pLeaves[k++];
            continue;
        }
        if ( pC0->pLeaves[i] < pC1->pLeaves[k] )
        {
            pC->pLeaves[c] = pC0->pLeaves[i++];
            continue;
        }
        if ( pC0->pLeaves[i] > pC1->pLeaves[k] )
        {
            pC->pLeaves[c] = pC1->pLeaves[k++];
            continue;
        }
        pC->pLeaves[c] = pC0->pLeaves[i++]; 
        k++;
    }
    if ( i < (int)pC0->nLeaves || k < (int)pC1->nLeaves )
        return 0;
    pC->nLeaves = c;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Prepares the object for FPGA mapping.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Dar_CutMerge( Dar_Cut_t * pCut, Dar_Cut_t * pCut0, Dar_Cut_t * pCut1 )
{ 
    assert( !pCut->fUsed );
    // merge the nodes
    if ( pCut0->nLeaves <= pCut1->nLeaves )
    {
        if ( !Dar_CutMergeOrdered( pCut, pCut1, pCut0 ) )
            return 0;
    }
    else
    {
        if ( !Dar_CutMergeOrdered( pCut, pCut0, pCut1 ) )
            return 0;
    }
    pCut->uSign = pCut0->uSign | pCut1->uSign;
    pCut->fUsed = 1;
    return 1;
}


/**Function*************************************************************

  Synopsis    [Computes the stretching phase of the cut w.r.t. the merged cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline unsigned Dar_CutTruthPhase( Dar_Cut_t * pCut, Dar_Cut_t * pCut1 )
{
    unsigned uPhase = 0;
    int i, k;
    for ( i = k = 0; i < (int)pCut->nLeaves; i++ )
    {
        if ( k == (int)pCut1->nLeaves )
            break;
        if ( pCut->pLeaves[i] < pCut1->pLeaves[k] )
            continue;
        assert( pCut->pLeaves[i] == pCut1->pLeaves[k] );
        uPhase |= (1 << i);
        k++;
    }
    return uPhase;
}

/**Function*************************************************************

  Synopsis    [Swaps two advancent variables of the truth table.]

  Description [Swaps variable iVar and iVar+1.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline unsigned Dar_CutTruthSwapAdjacentVars( unsigned uTruth, int iVar )
{
    assert( iVar >= 0 && iVar <= 2 );
    if ( iVar == 0 )
        return (uTruth & 0x99999999) | ((uTruth & 0x22222222) << 1) | ((uTruth & 0x44444444) >> 1);
    if ( iVar == 1 )
        return (uTruth & 0xC3C3C3C3) | ((uTruth & 0x0C0C0C0C) << 2) | ((uTruth & 0x30303030) >> 2);
    if ( iVar == 2 )
        return (uTruth & 0xF00FF00F) | ((uTruth & 0x00F000F0) << 4) | ((uTruth & 0x0F000F00) >> 4);
    assert( 0 );
    return 0;
}

/**Function*************************************************************

  Synopsis    [Swaps polarity of the variable.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline unsigned Dar_CutTruthSwapPolarity( unsigned uTruth, int iVar )
{
    assert( iVar >= 0 && iVar <= 3 );
    if ( iVar == 0 )
        return ((uTruth & 0xAAAA) >> 1) | ((uTruth & 0x5555) << 1);
    if ( iVar == 1 )
        return ((uTruth & 0xCCCC) >> 2) | ((uTruth & 0x3333) << 2);
    if ( iVar == 2 )
        return ((uTruth & 0xF0F0) >> 4) | ((uTruth & 0x0F0F) << 4);
    if ( iVar == 3 )
        return ((uTruth & 0xFF00) >> 8) | ((uTruth & 0x00FF) << 8);
    assert( 0 );
    return 0;
}

/**Function*************************************************************

  Synopsis    [Expands the truth table according to the phase.]

  Description [The input and output truth tables are in pIn/pOut. The current number
  of variables is nVars. The total number of variables in nVarsAll. The last argument
  (Phase) contains shows where the variables should go.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline unsigned Dar_CutTruthStretch( unsigned uTruth, int nVars, unsigned Phase )
{
    int i, k, Var = nVars - 1;
    for ( i = 3; i >= 0; i-- )
        if ( Phase & (1 << i) )
        {
            for ( k = Var; k < i; k++ )
                uTruth = Dar_CutTruthSwapAdjacentVars( uTruth, k );
            Var--;
        }
    assert( Var == -1 );
    return uTruth;
}

/**Function*************************************************************

  Synopsis    [Shrinks the truth table according to the phase.]

  Description [The input and output truth tables are in pIn/pOut. The current number
  of variables is nVars. The total number of variables in nVarsAll. The last argument
  (Phase) contains shows what variables should remain.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline unsigned Dar_CutTruthShrink( unsigned uTruth, int nVars, unsigned Phase )
{
    int i, k, Var = 0;
    for ( i = 0; i < 4; i++ )
        if ( Phase & (1 << i) )
        {
            for ( k = i-1; k >= Var; k-- )
                uTruth = Dar_CutTruthSwapAdjacentVars( uTruth, k );
            Var++;
        }
    return uTruth;
}

/**Function*************************************************************

  Synopsis    [Sort variables by their ID.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned Dar_CutSortVars( unsigned uTruth, int * pVars )
{
    int i, Temp, fChange, Counter = 0;
    // replace -1 by large number
    for ( i = 0; i < 4; i++ )
    {
        if ( pVars[i] == -1 )
            pVars[i] = 0x3FFFFFFF;
        else
            if ( Abc_LitIsCompl(pVars[i]) )
        {
            pVars[i] = Abc_LitNot( pVars[i] );
            uTruth = Dar_CutTruthSwapPolarity( uTruth, i );
        }
    }

    // permute variables
    do {
        fChange = 0;
        for ( i = 0; i < 3; i++ )
        {
            if ( pVars[i] <= pVars[i+1] )
                continue;
            Counter++;
            fChange = 1;

            Temp = pVars[i];
            pVars[i] = pVars[i+1];
            pVars[i+1] = Temp;

            uTruth = Dar_CutTruthSwapAdjacentVars( uTruth, i );
        }
    } while ( fChange );

    // replace large number by -1
    for ( i = 0; i < 4; i++ )
    {
        if ( pVars[i] == 0x3FFFFFFF )
            pVars[i] = -1;
//        printf( "%d ", pVars[i] );
    }
//    printf( "\n" );

    return uTruth;
}



/**Function*************************************************************

  Synopsis    [Performs truth table computation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline unsigned Dar_CutTruth( Dar_Cut_t * pCut, Dar_Cut_t * pCut0, Dar_Cut_t * pCut1, int fCompl0, int fCompl1 )
{
    unsigned uTruth0 = fCompl0 ? ~pCut0->uTruth : pCut0->uTruth;
    unsigned uTruth1 = fCompl1 ? ~pCut1->uTruth : pCut1->uTruth;
    uTruth0 = Dar_CutTruthStretch( uTruth0, pCut0->nLeaves, Dar_CutTruthPhase(pCut, pCut0) );
    uTruth1 = Dar_CutTruthStretch( uTruth1, pCut1->nLeaves, Dar_CutTruthPhase(pCut, pCut1) );
    return uTruth0 & uTruth1;
}

/**Function*************************************************************

  Synopsis    [Minimize support of the cut.]

  Description [Returns 1 if the node's support has changed]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Dar_CutSuppMinimize( Dar_Cut_t * pCut )
{
    unsigned uMasks[4][2] = {
        { 0x5555, 0xAAAA },
        { 0x3333, 0xCCCC },
        { 0x0F0F, 0xF0F0 },
        { 0x00FF, 0xFF00 }
    };
    unsigned uPhase = 0, uTruth = 0xFFFF & pCut->uTruth;
    int i, k, nLeaves;
    assert( pCut->fUsed );
    // compute the support of the cut's function
    nLeaves = pCut->nLeaves;
    for ( i = 0; i < (int)pCut->nLeaves; i++ )
        if ( (uTruth & uMasks[i][0]) == ((uTruth & uMasks[i][1]) >> (1 << i)) )
            nLeaves--;
        else
            uPhase |= (1 << i);
    if ( nLeaves == (int)pCut->nLeaves )
        return 0;
    // shrink the truth table
    uTruth = Dar_CutTruthShrink( uTruth, pCut->nLeaves, uPhase );
    pCut->uTruth = 0xFFFF & uTruth;
    // update leaves and signature
    pCut->uSign = 0;
    for ( i = k = 0; i < (int)pCut->nLeaves; i++ )
    {
        if ( !(uPhase & (1 << i)) )
            continue;    
        pCut->pLeaves[k++] = pCut->pLeaves[i];
        pCut->uSign |= Aig_ObjCutSign( pCut->pLeaves[i] );
    }
    assert( k == nLeaves );
    pCut->nLeaves = nLeaves;
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Dar_ManCutsFree( Dar_Man_t * p )
{
    if ( p->pMemCuts == NULL )
        return;
    Aig_MmFixedStop( p->pMemCuts, 0 );
    p->pMemCuts = NULL;
//    Aig_ManCleanData( p );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Dar_Cut_t * Dar_ObjPrepareCuts( Dar_Man_t * p, Aig_Obj_t * pObj )
{
    Dar_Cut_t * pCutSet, * pCut;
    int i;
    assert( Dar_ObjCuts(pObj) == NULL );
    pObj->nCuts = p->pPars->nCutsMax;
    // create the cutset of the node
    pCutSet = (Dar_Cut_t *)Aig_MmFixedEntryFetch( p->pMemCuts );
    memset( pCutSet, 0, p->pPars->nCutsMax * sizeof(Dar_Cut_t) );
    Dar_ObjSetCuts( pObj, pCutSet );
    Dar_ObjForEachCutAll( pObj, pCut, i )
        pCut->fUsed = 0;
    Vec_PtrPush( p->vCutNodes, pObj );
    // add unit cut if needed
    pCut = pCutSet;
    pCut->fUsed = 1;
    if ( Aig_ObjIsConst1(pObj) )
    {
        pCut->nLeaves = 0;
        pCut->uSign = 0;
        pCut->uTruth = 0xFFFF;
    }
    else
    {
        pCut->nLeaves = 1;
        pCut->pLeaves[0] = pObj->Id;
        pCut->uSign = Aig_ObjCutSign( pObj->Id );
        pCut->uTruth = 0xAAAA;
    }
    pCut->Value = Dar_CutFindValue( p, pCut );
    if ( p->nCutMemUsed < Aig_MmFixedReadMemUsage(p->pMemCuts)/(1<<20) )
        p->nCutMemUsed = Aig_MmFixedReadMemUsage(p->pMemCuts)/(1<<20);
    return pCutSet;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Dar_ManCutsRestart( Dar_Man_t * p, Aig_Obj_t * pRoot )
{
    Aig_Obj_t * pObj;
    int i;
    Dar_ObjSetCuts( Aig_ManConst1(p->pAig), NULL );
    Vec_PtrForEachEntry( Aig_Obj_t *, p->vCutNodes, pObj, i )
        if ( !Aig_ObjIsNone(pObj) )
            Dar_ObjSetCuts( pObj, NULL );
    Vec_PtrClear( p->vCutNodes );
    Aig_MmFixedRestart( p->pMemCuts );
    Dar_ObjPrepareCuts( p, Aig_ManConst1(p->pAig) );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Dar_Cut_t * Dar_ObjComputeCuts( Dar_Man_t * p, Aig_Obj_t * pObj, int fSkipTtMin )
{
    Aig_Obj_t * pFanin0 = Aig_ObjReal_rec( Aig_ObjChild0(pObj) );
    Aig_Obj_t * pFanin1 = Aig_ObjReal_rec( Aig_ObjChild1(pObj) );
    Aig_Obj_t * pFaninR0 = Aig_Regular(pFanin0);
    Aig_Obj_t * pFaninR1 = Aig_Regular(pFanin1);
    Dar_Cut_t * pCutSet, * pCut0, * pCut1, * pCut;
    int i, k; 

    assert( !Aig_IsComplement(pObj) );
    assert( Aig_ObjIsNode(pObj) );
    assert( Dar_ObjCuts(pObj) == NULL );
    assert( Dar_ObjCuts(pFaninR0) != NULL );
    assert( Dar_ObjCuts(pFaninR1) != NULL );

    // set up the first cut
    pCutSet = Dar_ObjPrepareCuts( p, pObj );
    // make sure fanins cuts are computed
    Dar_ObjForEachCut( pFaninR0, pCut0, i )
    Dar_ObjForEachCut( pFaninR1, pCut1, k )
    {
        p->nCutsAll++;
        // make sure K-feasible cut exists
        if ( Dar_WordCountOnes(pCut0->uSign | pCut1->uSign) > 4 )
            continue;
        // get the next cut of this node
        pCut = Dar_CutFindFree( p, pObj );
        // create the new cut
        if ( !Dar_CutMerge( pCut, pCut0, pCut1 ) )
        {
            assert( !pCut->fUsed );
            continue;
        }
        p->nCutsTried++;
        // check dominance
        if ( Dar_CutFilter( pObj, pCut ) )
        {
            assert( !pCut->fUsed );
            continue;
        }
        // compute truth table
        pCut->uTruth = 0xFFFF & Dar_CutTruth( pCut, pCut0, pCut1, Aig_IsComplement(pFanin0), Aig_IsComplement(pFanin1) );

        // minimize support of the cut
        if ( !fSkipTtMin && Dar_CutSuppMinimize( pCut ) )
        {
            int RetValue = Dar_CutFilter( pObj, pCut );
            assert( !RetValue );
        }

        // assign the value of the cut
        pCut->Value = Dar_CutFindValue( p, pCut );
        // if the cut contains removed node, do not use it
        if ( pCut->Value == 0 )
        {
            p->nCutsSkipped++;
            pCut->fUsed = 0;
        }
        else if ( pCut->nLeaves < 2 )
            return pCutSet;
    }
    // count the number of nontrivial cuts cuts
    Dar_ObjForEachCut( pObj, pCut, i )
        p->nCutsUsed += pCut->fUsed;
    // discount trivial cut
    p->nCutsUsed--;
    return pCutSet;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Dar_Cut_t * Dar_ObjComputeCuts_rec( Dar_Man_t * p, Aig_Obj_t * pObj )
{
    if ( Dar_ObjCuts(pObj) )
        return Dar_ObjCuts(pObj);
    if ( Aig_ObjIsCi(pObj) )
        return Dar_ObjPrepareCuts( p, pObj );
    if ( Aig_ObjIsBuf(pObj) )
        return Dar_ObjComputeCuts_rec( p, Aig_ObjFanin0(pObj) );
    Dar_ObjComputeCuts_rec( p, Aig_ObjFanin0(pObj) );
    Dar_ObjComputeCuts_rec( p, Aig_ObjFanin1(pObj) );
    return Dar_ObjComputeCuts( p, pObj, 0 );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

