/**CFile****************************************************************

  FileName    [reoProfile.c]

  PackageName [REO: A specialized DD reordering engine.]

  Synopsis    [Procudures that compute variables profiles (nodes, width, APL).]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - October 15, 2002.]

  Revision    [$Id: reoProfile.c,v 1.0 2002/15/10 03:00:00 alanmi Exp $]

***********************************************************************/

#include "reo.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////
///                    FUNCTION DEFINITIONS                          ///
////////////////////////////////////////////////////////////////////////


/**Function********************************************************************

  Synopsis    [Start the profile for the BDD nodes.]

  Description [TopRef is the first level, on this the given node counts towards 
  the width of the BDDs. (In other words, it is the level of the referencing node plus 1.)]

  SideEffects []

  SeeAlso     []

******************************************************************************/
void reoProfileNodesStart( reo_man * p )
{
    int Total, i;
    Total = 0;
    for ( i = 0; i <= p->nSupp; i++ )
    {
        p->pPlanes[i].statsCost = p->pPlanes[i].statsNodes;
        Total += p->pPlanes[i].statsNodes;
    }
    assert( Total == p->nNodesCur );
    p->nNodesBeg = p->nNodesCur;
}

/**Function*************************************************************

  Synopsis    [Start the profile for the APL.]

  Description [Computes the total path length. The path length is normalized
  by dividing it by 2^|supp(f)|. To get the "real" APL, multiply by 2^|supp(f)|.
  This procedure assumes that Weight field of all nodes has been set to 0.0 
  before the call, except for the weight of the topmost node, which is set to 1.0 
  (1.0 is the probability of traversing the topmost node). This procedure 
  assigns the edge weights. Because of the equal probability of selecting 0 and 1 
  assignment at a node, the edge weights are the same for the node. 
  Instead of storing them, we store the weight of the node, which is the probability 
  of traversing the node (pUnit->Weight) during the top down evalation of the BDD. ]

  SideEffects []

  SeeAlso     []

***********************************************************************/
void reoProfileAplStart( reo_man * p )
{
    reo_unit * pER, * pTR;
    reo_unit * pUnit;
    double Res, Half;
    int i;

    // clean the weights of all nodes
    for ( i = 0; i < p->nSupp; i++ )
        for ( pUnit = p->pPlanes[i].pHead; pUnit; pUnit = pUnit->Next )
            pUnit->Weight = 0.0;
    // to assign the node weights (the probability of visiting each node)
    // we visit the node after visiting its predecessors

    // set the probability of visits to the top nodes
    for ( i = 0; i < p->nTops; i++ )
        Unit_Regular(p->pTops[i])->Weight += 1.0;

    // to compute the path length (the sum of products of edge weight by edge length)
    // we visit the nodes in any order (the above order will do)
    Res = 0.0;
    for ( i = 0; i < p->nSupp; i++ )
    {
        p->pPlanes[i].statsCost = 0.0;
        for ( pUnit = p->pPlanes[i].pHead; pUnit; pUnit = pUnit->Next )
        {
            pER  = Unit_Regular(pUnit->pE);
            pTR  = Unit_Regular(pUnit->pT);
            Half = 0.5 * pUnit->Weight;
            pER->Weight += Half;
            pTR->Weight += Half;
            // add to the path length
            p->pPlanes[i].statsCost += pUnit->Weight;
        }
        Res += p->pPlanes[i].statsCost;
    }
    p->pPlanes[p->nSupp].statsCost = 0.0;
    p->nAplBeg = p->nAplCur = Res;
}

/**Function********************************************************************

  Synopsis    [Start the profile for the BDD width. Complexity of the algorithm is O(N + n).]

  Description [TopRef is the first level, on which the given node counts towards 
  the width of the BDDs. (In other words, it is the level of the referencing node plus 1.)]

  SideEffects []

  SeeAlso     []

******************************************************************************/
void reoProfileWidthStart( reo_man * p )
{
    reo_unit * pUnit;
    int * pWidthStart;
    int * pWidthStop;
    int v;

    // allocate and clean the storage for starting and stopping levels
    pWidthStart = ABC_ALLOC( int, p->nSupp + 1 );
    pWidthStop  = ABC_ALLOC( int, p->nSupp + 1 );
    memset( pWidthStart, 0, sizeof(int) * (p->nSupp + 1) );
    memset( pWidthStop, 0, sizeof(int) * (p->nSupp + 1) );

    // go through the non-constant nodes and set the topmost level of their cofactors
    for ( v = 0; v <= p->nSupp; v++ )
    for ( pUnit = p->pPlanes[v].pHead; pUnit; pUnit = pUnit->Next )
    {
        pUnit->TopRef = REO_TOPREF_UNDEF;
        pUnit->Sign   = 0;
    }

    // add the topmost level of the width profile
    for ( v = 0; v < p->nTops; v++ )
    {
        pUnit = Unit_Regular(p->pTops[v]);
        if ( pUnit->TopRef == REO_TOPREF_UNDEF )
        {
            // set the starting level
            pUnit->TopRef = 0;
            pWidthStart[pUnit->TopRef]++;
            // set the stopping level
            if ( pUnit->lev != REO_CONST_LEVEL )
                pWidthStop[pUnit->lev+1]++;
        }
    }

    for ( v = 0; v < p->nSupp; v++ )
    for ( pUnit = p->pPlanes[v].pHead; pUnit; pUnit = pUnit->Next )
    {
        if ( pUnit->pE->TopRef == REO_TOPREF_UNDEF )
        {
            // set the starting level
            pUnit->pE->TopRef = pUnit->lev + 1;
            pWidthStart[pUnit->pE->TopRef]++;
            // set the stopping level
            if ( pUnit->pE->lev != REO_CONST_LEVEL )
                pWidthStop[pUnit->pE->lev+1]++;
        }
        if ( pUnit->pT->TopRef == REO_TOPREF_UNDEF )
        {
            // set the starting level
            pUnit->pT->TopRef = pUnit->lev + 1;
            pWidthStart[pUnit->pT->TopRef]++;
            // set the stopping level
            if ( pUnit->pT->lev != REO_CONST_LEVEL )
                pWidthStop[pUnit->pT->lev+1]++;
        }
    }

    // verify the top reference
    for ( v = 0; v < p->nSupp; v++ )
        reoProfileWidthVerifyLevel( p->pPlanes + v, v );

    // derive the profile
    p->nWidthCur = 0;
    for ( v = 0; v <= p->nSupp; v++ )
    {
        if ( v == 0 )
            p->pPlanes[v].statsWidth = pWidthStart[v] - pWidthStop[v];
        else
            p->pPlanes[v].statsWidth = p->pPlanes[v-1].statsWidth + pWidthStart[v] - pWidthStop[v];
        p->pPlanes[v].statsCost = p->pPlanes[v].statsWidth;
        p->nWidthCur += p->pPlanes[v].statsWidth;
        printf( "Level %2d: Width = %5d.\n", v, p->pPlanes[v].statsWidth );
    }
    p->nWidthBeg = p->nWidthCur;
    ABC_FREE( pWidthStart );
    ABC_FREE( pWidthStop );
}

/**Function********************************************************************

  Synopsis    [Start the profile for the BDD width. Complexity of the algorithm is O(N * n).]

  Description [TopRef is the first level, on which the given node counts towards 
  the width of the BDDs. (In other words, it is the level of the referencing node plus 1.)]

  SideEffects []

  SeeAlso     []

******************************************************************************/
void reoProfileWidthStart2( reo_man * p )
{
    reo_unit * pUnit;
    int i, v;

    // clean the profile
    for ( i = 0; i <= p->nSupp; i++ )
        p->pPlanes[i].statsWidth = 0;
    
    // clean the node structures
    for ( v = 0; v <= p->nSupp; v++ )
    for ( pUnit = p->pPlanes[v].pHead; pUnit; pUnit = pUnit->Next )
    {
        pUnit->TopRef = REO_TOPREF_UNDEF;
        pUnit->Sign   = 0;
    }

    // set the topref to the topmost nodes
    for ( i = 0; i < p->nTops; i++ )
        Unit_Regular(p->pTops[i])->TopRef = 0;

    // go through the non-constant nodes and set the topmost level of their cofactors
    for ( i = 0; i < p->nSupp; i++ )
        for ( pUnit = p->pPlanes[i].pHead; pUnit; pUnit = pUnit->Next )
        {
            if ( pUnit->pE->TopRef > i+1 )
                 pUnit->pE->TopRef = i+1;
            if ( pUnit->pT->TopRef > i+1 )
                 pUnit->pT->TopRef = i+1;
        }

    // verify the top reference
    for ( i = 0; i < p->nSupp; i++ )
        reoProfileWidthVerifyLevel( p->pPlanes + i, i );

    // compute the profile for the internal nodes
    for ( i = 0; i < p->nSupp; i++ )
        for ( pUnit = p->pPlanes[i].pHead; pUnit; pUnit = pUnit->Next )
            for ( v = pUnit->TopRef; v <= pUnit->lev; v++ )
                p->pPlanes[v].statsWidth++;

    // compute the profile for the constant nodes
    for ( pUnit = p->pPlanes[p->nSupp].pHead; pUnit; pUnit = pUnit->Next )
        for ( v = pUnit->TopRef; v <= p->nSupp; v++ )
            p->pPlanes[v].statsWidth++;

    // get the width cost
    p->nWidthCur = 0;
    for ( i = 0; i <= p->nSupp; i++ )
    {
        p->pPlanes[i].statsCost = p->pPlanes[i].statsWidth;
        p->nWidthCur           += p->pPlanes[i].statsWidth;
    }
    p->nWidthBeg = p->nWidthCur;
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
void reoProfileNodesPrint( reo_man * p )
{
    printf( "NODES: Total = %6d. Average = %6.2f.\n", p->nNodesCur, p->nNodesCur / (float)p->nSupp );
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
void reoProfileAplPrint( reo_man * p )
{
    printf( "APL: Total = %8.2f. Average =%6.2f.\n", p->nAplCur, p->nAplCur / (float)p->nSupp );
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
void reoProfileWidthPrint( reo_man * p )
{
    int WidthMax;
    int TotalWidth;
    int i;

    WidthMax   = 0;
    TotalWidth = 0;
    for ( i = 0; i <= p->nSupp; i++ )
    {
        printf( "Level = %2d. Width = %3d.\n", i, p->pPlanes[i].statsWidth );
        if ( WidthMax < p->pPlanes[i].statsWidth )
             WidthMax = p->pPlanes[i].statsWidth;
        TotalWidth += p->pPlanes[i].statsWidth;
    }
    assert( p->nWidthCur == TotalWidth );
    printf( "WIDTH: " );
    printf( "Maximum = %5d.  ", WidthMax );
    printf( "Total = %7d.  ", p->nWidthCur );
    printf( "Average = %6.2f.\n", TotalWidth / (float)p->nSupp );
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
void reoProfileWidthVerifyLevel( reo_plane * pPlane, int Level )
{
    reo_unit * pUnit;
    for ( pUnit = pPlane->pHead; pUnit; pUnit = pUnit->Next )
    {
        assert( pUnit->TopRef     <= Level );
        assert( pUnit->pE->TopRef <= Level + 1 );
        assert( pUnit->pT->TopRef <= Level + 1 );
    }
}

////////////////////////////////////////////////////////////////////////
///                         END OF FILE                              ///
////////////////////////////////////////////////////////////////////////

ABC_NAMESPACE_IMPL_END

