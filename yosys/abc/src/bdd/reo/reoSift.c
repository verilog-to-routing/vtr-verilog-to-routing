/**CFile****************************************************************

  FileName    [reoSift.c]

  PackageName [REO: A specialized DD reordering engine.]

  Synopsis    [Implementation of the sifting algorihtm.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - October 15, 2002.]

  Revision    [$Id: reoSift.c,v 1.0 2002/15/10 03:00:00 alanmi Exp $]

***********************************************************************/

#include "reo.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DEFINITIONS                          ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Implements the variable sifting algorithm.]

  Description [Performs a sequence of adjacent variable swaps known as "sifting".
  Uses the cost functions determined by the flag.]

  SideEffects []

  SeeAlso     []

***********************************************************************/
void reoReorderSift( reo_man * p )
{
    double CostCurrent;  // the cost of the current permutation
    double CostLimit;    // the maximum increase in cost that can be tolerated
    double CostBest;     // the best cost
    int BestQ;           // the best position
    int VarCurrent;      // the current variable to move   
    int q;               // denotes the current position of the variable
    int c;               // performs the loops over variables until all of them are sifted
    int v;               // used for other purposes

    assert( p->nSupp > 0 );

    // set the current cost depending on the minimization criteria
    if ( p->fMinWidth )
        CostCurrent = p->nWidthCur;
    else if ( p->fMinApl )
        CostCurrent = p->nAplCur;
    else
        CostCurrent = p->nNodesCur;

    // find the upper bound on tbe cost growth
    CostLimit = 1 + (int)(REO_REORDER_LIMIT * CostCurrent);

    // perform sifting for each of p->nSupp variables
    for ( c = 0; c < p->nSupp; c++ )
    {
        // select the current variable to be the one with the largest number of nodes that is not sifted yet
        VarCurrent = -1;
        CostBest   = -1.0;
        for ( v = 0; v < p->nSupp; v++ )
        {
            p->pVarCosts[v] = REO_HIGH_VALUE;
            if ( !p->pPlanes[v].fSifted )
            {
//                VarCurrent = v;
//                if ( CostBest < p->pPlanes[v].statsCost )
                if ( CostBest < p->pPlanes[v].statsNodes )
                {
//                    CostBest   = p->pPlanes[v].statsCost;
                    CostBest   = p->pPlanes[v].statsNodes;
                    VarCurrent = v;
                }

            }
        }
        assert( VarCurrent != -1 );
        // mark this variable as sifted
        p->pPlanes[VarCurrent].fSifted = 1;

        // set the current value
        p->pVarCosts[VarCurrent] = CostCurrent;

        // set the best cost
        CostBest = CostCurrent;
        BestQ    = VarCurrent; 

        // determine which way to move the variable first (up or down)
        // the rationale is that if we move the shorter way first
        // it is more likely that the best position will be found on the longer way
        // and the reverse movement (to take the best position) will be faster
        if ( VarCurrent < p->nSupp/2 ) // move up first, then down
        {
            // set the total cost on all levels above the current level
            p->pPlanes[0].statsCostAbove = 0;
            for ( v = 1; v <= VarCurrent; v++ )
                p->pPlanes[v].statsCostAbove = p->pPlanes[v-1].statsCostAbove + p->pPlanes[v-1].statsCost;
            // set the total cost on all levels below the current level
            p->pPlanes[p->nSupp].statsCostBelow = 0;
            for ( v = p->nSupp - 1; v >= VarCurrent; v-- )
                p->pPlanes[v].statsCostBelow = p->pPlanes[v+1].statsCostBelow + p->pPlanes[v+1].statsCost;

            assert( CostCurrent == p->pPlanes[VarCurrent].statsCostAbove + 
                                    p->pPlanes[VarCurrent].statsCost +
                                    p->pPlanes[VarCurrent].statsCostBelow );

            // move up
            for ( q = VarCurrent-1; q >= 0; q-- )
            {
                CostCurrent -= reoReorderSwapAdjacentVars( p, q, 1 );
                // now q points to the position of this var in the order
                p->pVarCosts[q] = CostCurrent;
                // update the lower bound (assuming that for level q+1 it is set correctly)
                p->pPlanes[q].statsCostBelow = p->pPlanes[q+1].statsCostBelow + p->pPlanes[q+1].statsCost;
                // check the upper bound
                if ( CostCurrent >= CostLimit )
                    break;
                // check the lower bound
                if ( p->pPlanes[q].statsCostBelow + (REO_QUAL_PAR-1)*p->pPlanes[q].statsCostAbove/REO_QUAL_PAR >= CostBest )
                    break;
                // update the best cost
                if ( CostBest > CostCurrent )
                {
                    CostBest = CostCurrent;
                    BestQ    = q;
                    // adjust node limit
                    CostLimit = ddMin( CostLimit, 1 + (int)(REO_REORDER_LIMIT * CostCurrent) );
                }

                // when we are reordering for width or APL, it may happen that
                // the number of nodes has grown above certain limit,
                // in which case we have to resize the data structures
                if ( p->fMinWidth || p->fMinApl )
                {
                    if ( p->nNodesCur >= 2 * p->nNodesMaxAlloc )
                    {
//                        printf( "Resizing data structures. Old size = %6d. New size = %6d.\n",  p->nNodesMaxAlloc, p->nNodesCur );
                        reoResizeStructures( p, 0, p->nNodesCur, 0 );
                    }
                }
            }
            // fix the plane index
            if ( q == -1 )
                q++;
            // now p points to the position of this var in the order

            // move down
            for ( ; q < p->nSupp-1; )
            {
                CostCurrent -= reoReorderSwapAdjacentVars( p, q, 0 );
                q++;    // change q to point to the position of this var in the order
                // sanity check: the number of nodes on the back pass should be the same
                if ( p->pVarCosts[q] != REO_HIGH_VALUE && fabs( p->pVarCosts[q] - CostCurrent ) > REO_COST_EPSILON )
                    printf("reoReorderSift(): Error! On the backward move, the costs are different.\n");
                p->pVarCosts[q] = CostCurrent;
                // update the lower bound (assuming that for level q-1 it is set correctly)
                p->pPlanes[q].statsCostAbove = p->pPlanes[q-1].statsCostAbove + p->pPlanes[q-1].statsCost;
                // check the bounds only if the variable already reached its previous position
                if ( q >= BestQ )
                {
                    // check the upper bound
                    if ( CostCurrent >= CostLimit )
                        break;
                    // check the lower bound
                    if ( p->pPlanes[q].statsCostAbove + (REO_QUAL_PAR-1)*p->pPlanes[q].statsCostBelow/REO_QUAL_PAR >= CostBest )
                        break;
                }
                // update the best cost
                if ( CostBest >= CostCurrent )
                {
                    CostBest = CostCurrent;
                    BestQ    = q;
                    // adjust node limit
                    CostLimit = ddMin( CostLimit, 1 + (int)(REO_REORDER_LIMIT * CostCurrent) );
                }

                // when we are reordering for width or APL, it may happen that
                // the number of nodes has grown above certain limit,
                // in which case we have to resize the data structures
                if ( p->fMinWidth || p->fMinApl )
                {
                    if ( p->nNodesCur >= 2 * p->nNodesMaxAlloc )
                    {
//                        printf( "Resizing data structures. Old size = %6d. New size = %6d.\n",  p->nNodesMaxAlloc, p->nNodesCur );
                        reoResizeStructures( p, 0, p->nNodesCur, 0 );
                    }
                }
            }
            // move the variable up from the given position (q) to the best position (BestQ)
            assert( q >= BestQ );
            for ( ; q > BestQ; q-- )
            {
                CostCurrent -= reoReorderSwapAdjacentVars( p, q-1, 1 );
                // sanity check: the number of nodes on the back pass should be the same
                if ( fabs( p->pVarCosts[q-1] - CostCurrent ) > REO_COST_EPSILON )
                {
                    printf("reoReorderSift():  Error! On the return move, the costs are different.\n" );
                    fflush(stdout);
                }
            }
        }
        else // move down first, then up
        {
            // set the current number of nodes on all levels above the given level
            p->pPlanes[0].statsCostAbove = 0;
            for ( v = 1; v <= VarCurrent; v++ )
                p->pPlanes[v].statsCostAbove = p->pPlanes[v-1].statsCostAbove + p->pPlanes[v-1].statsCost;
            // set the current number of nodes on all levels below the given level
            p->pPlanes[p->nSupp].statsCostBelow = 0;
            for ( v = p->nSupp - 1; v >= VarCurrent; v-- )
                p->pPlanes[v].statsCostBelow = p->pPlanes[v+1].statsCostBelow + p->pPlanes[v+1].statsCost;
            
            assert( CostCurrent == p->pPlanes[VarCurrent].statsCostAbove + 
                                    p->pPlanes[VarCurrent].statsCost +
                                    p->pPlanes[VarCurrent].statsCostBelow );

            // move down
            for ( q = VarCurrent; q < p->nSupp-1; )
            {
                CostCurrent -= reoReorderSwapAdjacentVars( p, q, 0 );
                q++;    // change q to point to the position of this var in the order
                p->pVarCosts[q] = CostCurrent;
                // update the lower bound (assuming that for level q-1 it is set correctly)
                p->pPlanes[q].statsCostAbove = p->pPlanes[q-1].statsCostAbove + p->pPlanes[q-1].statsCost;
                // check the upper bound
                if ( CostCurrent >= CostLimit )
                    break;
                // check the lower bound
                if ( p->pPlanes[q].statsCostAbove + (REO_QUAL_PAR-1)*p->pPlanes[q].statsCostBelow/REO_QUAL_PAR >= CostBest )
                    break;
                // update the best cost
                if ( CostBest > CostCurrent )
                {
                    CostBest = CostCurrent;
                    BestQ    = q;
                    // adjust node limit
                    CostLimit = ddMin( CostLimit, 1 + (int)(REO_REORDER_LIMIT * CostCurrent) );
                }

                // when we are reordering for width or APL, it may happen that
                // the number of nodes has grown above certain limit,
                // in which case we have to resize the data structures
                if ( p->fMinWidth || p->fMinApl )
                {
                    if ( p->nNodesCur >= 2 * p->nNodesMaxAlloc )
                    {
//                        printf( "Resizing data structures. Old size = %6d. New size = %6d.\n",  p->nNodesMaxAlloc, p->nNodesCur );
                        reoResizeStructures( p, 0, p->nNodesCur, 0 );
                    }
                }
            }

            // move up
            for ( --q; q >= 0; q-- )
            {
                CostCurrent -= reoReorderSwapAdjacentVars( p, q, 1 );
                // now q points to the position of this var in the order
                // sanity check: the number of nodes on the back pass should be the same
                if ( p->pVarCosts[q] != REO_HIGH_VALUE && fabs( p->pVarCosts[q] - CostCurrent ) > REO_COST_EPSILON )
                    printf("reoReorderSift(): Error! On the backward move, the costs are different.\n");
                p->pVarCosts[q] = CostCurrent;
                // update the lower bound (assuming that for level q+1 it is set correctly)
                p->pPlanes[q].statsCostBelow = p->pPlanes[q+1].statsCostBelow + p->pPlanes[q+1].statsCost;
                // check the bounds only if the variable already reached its previous position
                if ( q <= BestQ )
                {
                    // check the upper bound
                    if ( CostCurrent >= CostLimit )
                        break;
                    // check the lower bound
                    if ( p->pPlanes[q].statsCostBelow + (REO_QUAL_PAR-1)*p->pPlanes[q].statsCostAbove/REO_QUAL_PAR >= CostBest )
                        break;
                }
                // update the best cost
                if ( CostBest >= CostCurrent )
                {
                    CostBest = CostCurrent;
                    BestQ    = q;
                    // adjust node limit
                    CostLimit = ddMin( CostLimit, 1 + (int)(REO_REORDER_LIMIT * CostCurrent) );
                }

                // when we are reordering for width or APL, it may happen that
                // the number of nodes has grown above certain limit,
                // in which case we have to resize the data structures
                if ( p->fMinWidth || p->fMinApl )
                {
                    if ( p->nNodesCur >= 2 * p->nNodesMaxAlloc )
                    {
//                        printf( "Resizing data structures. Old size = %6d. New size = %6d.\n",  p->nNodesMaxAlloc, p->nNodesCur );
                        reoResizeStructures( p, 0, p->nNodesCur, 0 );
                    }
                }
            }
            // fix the plane index
            if ( q == -1 )
                q++;
            // now q points to the position of this var in the order
            // move the variable down from the given position (q) to the best position (BestQ)
            assert( q <= BestQ );
            for ( ; q < BestQ; q++ )
            {
                CostCurrent -= reoReorderSwapAdjacentVars( p, q, 0 );
                // sanity check: the number of nodes on the back pass should be the same
                if ( fabs( p->pVarCosts[q+1] - CostCurrent ) > REO_COST_EPSILON )
                {
                    printf("reoReorderSift(): Error! On the return move, the costs are different.\n" );
                    fflush(stdout);
                }
            }
        }
        assert( fabs( CostBest - CostCurrent ) < REO_COST_EPSILON );

        // update the cost 
        if ( p->fMinWidth )
            p->nWidthCur = (int)CostBest;
        else if ( p->fMinApl )
            p->nAplCur = CostCurrent;
        else
            p->nNodesCur = (int)CostBest;
    }

    // remove the sifted attributes if any
    for ( v = 0; v < p->nSupp; v++ )
        p->pPlanes[v].fSifted = 0;
}

////////////////////////////////////////////////////////////////////////
///                         END OF FILE                              ///
////////////////////////////////////////////////////////////////////////

ABC_NAMESPACE_IMPL_END

