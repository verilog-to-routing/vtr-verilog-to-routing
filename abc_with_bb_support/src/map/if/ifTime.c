/**CFile****************************************************************

  FileName    [ifTime.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [FPGA mapping based on priority cuts.]

  Synopsis    [Computation of delay paramters depending on the library.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - November 21, 2006.]

  Revision    [$Id: ifTime.c,v 1.00 2006/11/21 00:00:00 alanmi Exp $]

***********************************************************************/

#include "if.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static void If_CutSortInputPins( If_Man_t * p, If_Cut_t * pCut, int * pPinPerm, float * pPinDelays );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Computes delay.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
float If_CutDelay( If_Man_t * p, If_Cut_t * pCut )
{
    static int pPinPerm[IF_MAX_LUTSIZE];
    static float pPinDelays[IF_MAX_LUTSIZE];
    If_Obj_t * pLeaf;
    float Delay, DelayCur;
    float * pLutDelays;
    int i, Shift;
    assert( p->pPars->fSeqMap || pCut->nLeaves > 1 );
    Delay = -IF_FLOAT_LARGE;
    if ( p->pPars->pLutLib )
    {
        assert( !p->pPars->fLiftLeaves );
        pLutDelays = p->pPars->pLutLib->pLutDelays[pCut->nLeaves];
        if ( p->pPars->pLutLib->fVarPinDelays )
        {
            // compute the delay using sorted pins
            If_CutSortInputPins( p, pCut, pPinPerm, pPinDelays );
            for ( i = 0; i < (int)pCut->nLeaves; i++ )
            {
                DelayCur = pPinDelays[pPinPerm[i]] + pLutDelays[i];
                Delay = IF_MAX( Delay, DelayCur );
            }
        }
        else
        {
            If_CutForEachLeaf( p, pCut, pLeaf, i )
            {
                DelayCur = If_ObjCutBest(pLeaf)->Delay + pLutDelays[0];
                Delay = IF_MAX( Delay, DelayCur );
            }
        }
    }
    else
    {
        if ( pCut->fUser )
        {
            assert( !p->pPars->fLiftLeaves );
            If_CutForEachLeaf( p, pCut, pLeaf, i )
            {
                DelayCur = If_ObjCutBest(pLeaf)->Delay + (float)pCut->pPerm[i];
                Delay = IF_MAX( Delay, DelayCur );
            }
        }
        else
        {
            if ( p->pPars->fLiftLeaves )
            {
                If_CutForEachLeafSeq( p, pCut, pLeaf, Shift, i )
                {
                    DelayCur = If_ObjCutBest(pLeaf)->Delay - Shift * p->Period;
                    Delay = IF_MAX( Delay, DelayCur );
                }
            }
            else
            {
                If_CutForEachLeaf( p, pCut, pLeaf, i )
                {
                    DelayCur = If_ObjCutBest(pLeaf)->Delay;
                    Delay = IF_MAX( Delay, DelayCur );
                }
            }
            Delay += 1.0;
        }
    }
    return Delay;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void If_CutPropagateRequired( If_Man_t * p, If_Cut_t * pCut, float ObjRequired )
{
    static int pPinPerm[IF_MAX_LUTSIZE];
    static float pPinDelays[IF_MAX_LUTSIZE];
    If_Obj_t * pLeaf;
    float * pLutDelays;
    float Required;
    int i;
    assert( !p->pPars->fLiftLeaves );
    // compute the pins
    if ( p->pPars->pLutLib )
    {
        pLutDelays = p->pPars->pLutLib->pLutDelays[pCut->nLeaves];
        if ( p->pPars->pLutLib->fVarPinDelays )
        {
            // compute the delay using sorted pins
            If_CutSortInputPins( p, pCut, pPinPerm, pPinDelays );
            for ( i = 0; i < (int)pCut->nLeaves; i++ )
            {
                Required = ObjRequired - pLutDelays[i];
                pLeaf = If_ManObj( p, pCut->pLeaves[pPinPerm[i]] );
                pLeaf->Required = IF_MIN( pLeaf->Required, Required );
            }
        }
        else
        {
            Required = ObjRequired - pLutDelays[0];
            If_CutForEachLeaf( p, pCut, pLeaf, i )
                pLeaf->Required = IF_MIN( pLeaf->Required, Required );
        }
    }
    else
    {
        if ( pCut->fUser )
        {
            If_CutForEachLeaf( p, pCut, pLeaf, i )
            {
                Required = ObjRequired - (float)pCut->pPerm[i];
                pLeaf->Required = IF_MIN( pLeaf->Required, Required );
            }
        }
        else
        {
            Required = ObjRequired - (float)1.0;
            If_CutForEachLeaf( p, pCut, pLeaf, i )
                pLeaf->Required = IF_MIN( pLeaf->Required, Required );
        }
    }
}

/**Function*************************************************************

  Synopsis    [Sorts the pins in the degreasing order of delays.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void If_CutSortInputPins( If_Man_t * p, If_Cut_t * pCut, int * pPinPerm, float * pPinDelays )
{
    If_Obj_t * pLeaf;
    int i, j, best_i, temp;
    // start the trivial permutation and collect pin delays
    If_CutForEachLeaf( p, pCut, pLeaf, i )
    {
        pPinPerm[i] = i;
        pPinDelays[i] = If_ObjCutBest(pLeaf)->Delay;
    }
    // selection sort the pins in the decreasible order of delays
    // this order will match the increasing order of LUT input pins
    for ( i = 0; i < (int)pCut->nLeaves-1; i++ )
    {
        best_i = i;
        for ( j = i+1; j < (int)pCut->nLeaves; j++ )
            if ( pPinDelays[pPinPerm[j]] > pPinDelays[pPinPerm[best_i]] )
                best_i = j;
        if ( best_i == i )
            continue;
        temp = pPinPerm[i]; 
        pPinPerm[i] = pPinPerm[best_i]; 
        pPinPerm[best_i] = temp;
    }
    // verify
    assert( pPinPerm[0] < (int)pCut->nLeaves );
    for ( i = 1; i < (int)pCut->nLeaves; i++ )
    {
        assert( pPinPerm[i] < (int)pCut->nLeaves );
        assert( pPinDelays[pPinPerm[i-1]] >= pPinDelays[pPinPerm[i]] );
    }
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


