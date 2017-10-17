/**CFile****************************************************************

  FileName    [cutTruth.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [K-feasible cut computation package.]

  Synopsis    [Incremental truth table computation.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: cutTruth.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "cutInt.h"

ABC_NAMESPACE_IMPL_START


/* 
    Truth tables computed in this package are represented as bit-strings
    stored in the cut data structure. Cuts of any number of inputs have 
    the truth table with 2^k bits, where k is the max number of cut inputs.
*/

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

// used in abcCut.c
int nTotal = 0;
int nGood  = 0;
int nEqual = 0;

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Computes the stretching phase of the cut w.r.t. the merged cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline unsigned Cut_TruthPhase( Cut_Cut_t * pCut, Cut_Cut_t * pCut1 )
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

  Synopsis    [Performs truth table computation.]

  Description [This procedure cannot be used while recording oracle
  because it will overwrite Num0 and Num1.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cut_TruthNCanonicize( Cut_Cut_t * pCut )
{
    unsigned uTruth;
    unsigned * uCanon2;
    char * pPhases2;
    assert( pCut->nVarsMax < 6 );

    // get the direct truth table
    uTruth = *Cut_CutReadTruth(pCut);

    // compute the direct truth table
    Extra_TruthCanonFastN( pCut->nVarsMax, pCut->nLeaves, &uTruth, &uCanon2, &pPhases2 );
//    uCanon[0] = uCanon2[0];
//    uCanon[1] = (p->nVarsMax == 6)? uCanon2[1] : uCanon2[0];
//    uPhases[0] = pPhases2[0];
    pCut->uCanon0 = uCanon2[0];
    pCut->Num0    = pPhases2[0];

    // get the complemented truth table
    uTruth = ~*Cut_CutReadTruth(pCut);

    // compute the direct truth table
    Extra_TruthCanonFastN( pCut->nVarsMax, pCut->nLeaves, &uTruth, &uCanon2, &pPhases2 );
//    uCanon[0] = uCanon2[0];
//    uCanon[1] = (p->nVarsMax == 6)? uCanon2[1] : uCanon2[0];
//    uPhases[0] = pPhases2[0];
    pCut->uCanon1 = uCanon2[0];
    pCut->Num1    = pPhases2[0];
}

/**Function*************************************************************

  Synopsis    [Performs truth table computation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cut_TruthComputeOld( Cut_Cut_t * pCut, Cut_Cut_t * pCut0, Cut_Cut_t * pCut1, int fCompl0, int fCompl1 )
{
    static unsigned uTruth0[8], uTruth1[8];
    int nTruthWords = Cut_TruthWords( pCut->nVarsMax );
    unsigned * pTruthRes;
    int i, uPhase;

    // permute the first table
    uPhase = Cut_TruthPhase( pCut, pCut0 );
    Extra_TruthExpand( pCut->nVarsMax, nTruthWords, Cut_CutReadTruth(pCut0), uPhase, uTruth0 );
    if ( fCompl0 ) 
    {
        for ( i = 0; i < nTruthWords; i++ )
            uTruth0[i] = ~uTruth0[i];
    }

    // permute the second table
    uPhase = Cut_TruthPhase( pCut, pCut1 );
    Extra_TruthExpand( pCut->nVarsMax, nTruthWords, Cut_CutReadTruth(pCut1), uPhase, uTruth1 );
    if ( fCompl1 ) 
    {
        for ( i = 0; i < nTruthWords; i++ )
            uTruth1[i] = ~uTruth1[i];
    }

    // write the resulting table
    pTruthRes = Cut_CutReadTruth(pCut);

    if ( pCut->fCompl )
    {
        for ( i = 0; i < nTruthWords; i++ )
            pTruthRes[i] = ~(uTruth0[i] & uTruth1[i]);
    }
    else
    {
        for ( i = 0; i < nTruthWords; i++ )
            pTruthRes[i] = uTruth0[i] & uTruth1[i];
    }
}

/**Function*************************************************************

  Synopsis    [Performs truth table computation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cut_TruthCompute( Cut_Man_t * p, Cut_Cut_t * pCut, Cut_Cut_t * pCut0, Cut_Cut_t * pCut1, int fCompl0, int fCompl1 )
{
    // permute the first table
    if ( fCompl0 ) 
        Extra_TruthNot( p->puTemp[0], Cut_CutReadTruth(pCut0), pCut->nVarsMax );
    else
        Extra_TruthCopy( p->puTemp[0], Cut_CutReadTruth(pCut0), pCut->nVarsMax );
    Extra_TruthStretch( p->puTemp[2], p->puTemp[0], pCut0->nLeaves, pCut->nVarsMax, Cut_TruthPhase(pCut, pCut0) );
    // permute the second table
    if ( fCompl1 ) 
        Extra_TruthNot( p->puTemp[1], Cut_CutReadTruth(pCut1), pCut->nVarsMax );
    else
        Extra_TruthCopy( p->puTemp[1], Cut_CutReadTruth(pCut1), pCut->nVarsMax );
    Extra_TruthStretch( p->puTemp[3], p->puTemp[1], pCut1->nLeaves, pCut->nVarsMax, Cut_TruthPhase(pCut, pCut1) );
    // produce the resulting table
    if ( pCut->fCompl )
        Extra_TruthNand( Cut_CutReadTruth(pCut), p->puTemp[2], p->puTemp[3], pCut->nVarsMax );
    else
        Extra_TruthAnd( Cut_CutReadTruth(pCut), p->puTemp[2], p->puTemp[3], pCut->nVarsMax );

//    Ivy_TruthTestOne( *Cut_CutReadTruth(pCut) );

    // quit if no fancy computation is needed
    if ( !p->pParams->fFancy )
        return;

    if ( pCut->nLeaves != 7 )
        return;

    // count the total number of truth tables computed
    nTotal++;

    // MAPPING INTO ALTERA 6-2 LOGIC BLOCKS
    // call this procedure to find the minimum number of common variables in the cofactors
    // if this number is less or equal than 3, the cut can be implemented using the 6-2 logic block
    if ( Extra_TruthMinCofSuppOverlap( Cut_CutReadTruth(pCut), pCut->nVarsMax, NULL ) <= 4 )
        nGood++;

    // MAPPING INTO ACTEL 2x2 CELLS
    // call this procedure to see if a semi-canonical form can be found in the lookup table 
    // (if it exists, then a two-level 3-input LUT implementation of the cut exists)
    // Before this procedure is called, cell manager should be defined by calling
    // Cut_CellLoad (make sure file "cells22_daomap_iwls.txt" is available in the working dir)
//    if ( Cut_CellIsRunning() && pCut->nVarsMax <= 9 )
//        nGood += Cut_CellTruthLookup( Cut_CutReadTruth(pCut), pCut->nVarsMax );
}



////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

