/**CFile****************************************************************

  FileName    [ifTruth.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [FPGA mapping based on priority cuts.]

  Synopsis    [Computation of truth tables of the cuts.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - November 21, 2006.]

  Revision    [$Id: ifTruth.c,v 1.00 2006/11/21 00:00:00 alanmi Exp $]

***********************************************************************/

#include "if.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Computes the stretching phase of the cut w.r.t. the merged cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline unsigned Cut_TruthPhase( If_Cut_t * pCut, If_Cut_t * pCut1 )
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

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void If_CutComputeTruth( If_Man_t * p, If_Cut_t * pCut, If_Cut_t * pCut0, If_Cut_t * pCut1, int fCompl0, int fCompl1 )
{
    extern void Kit_FactorTest( unsigned * pTruth, int nVars );

    // permute the first table
    if ( fCompl0 ^ pCut0->fCompl ) 
        Extra_TruthNot( p->puTemp[0], If_CutTruth(pCut0), pCut->nLimit );
    else
        Extra_TruthCopy( p->puTemp[0], If_CutTruth(pCut0), pCut->nLimit );
    Extra_TruthStretch( p->puTemp[2], p->puTemp[0], pCut0->nLeaves, pCut->nLimit, Cut_TruthPhase(pCut, pCut0) );
    // permute the second table
    if ( fCompl1 ^ pCut1->fCompl ) 
        Extra_TruthNot( p->puTemp[1], If_CutTruth(pCut1), pCut->nLimit );
    else
        Extra_TruthCopy( p->puTemp[1], If_CutTruth(pCut1), pCut->nLimit );
    Extra_TruthStretch( p->puTemp[3], p->puTemp[1], pCut1->nLeaves, pCut->nLimit, Cut_TruthPhase(pCut, pCut1) );
    // produce the resulting table
    assert( pCut->fCompl == 0 );
    if ( pCut->fCompl )
        Extra_TruthNand( If_CutTruth(pCut), p->puTemp[2], p->puTemp[3], pCut->nLimit );
    else
        Extra_TruthAnd( If_CutTruth(pCut), p->puTemp[2], p->puTemp[3], pCut->nLimit );

    // perform 
//    Kit_FactorTest( If_CutTruth(pCut), pCut->nLimit );
//    printf( "%d ", If_CutLeaveNum(pCut) - Kit_TruthSupportSize(If_CutTruth(pCut), If_CutLeaveNum(pCut)) );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


