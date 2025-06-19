/**CFile****************************************************************

  FileName    [mapperCanon.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Generic technology mapping engine.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 2.0. Started - June 1, 2004.]

  Revision    [$Id: mapperCanon.c,v 1.2 2005/01/23 06:59:42 alanmi Exp $]

***********************************************************************/

#include "mapperInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static unsigned Map_CanonComputePhase( unsigned uTruths[][2], int nVars, unsigned uTruth, unsigned uPhase );
static void     Map_CanonComputePhase6( unsigned uTruths[][2], int nVars, unsigned uTruth[], unsigned uPhase, unsigned uTruthRes[] );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Computes the N-canonical form of the Boolean function.]

  Description [The N-canonical form is defined as the truth table with 
  the minimum integer value. This function exhaustively enumerates 
  through the complete set of 2^N phase assignments.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Map_CanonComputeSlow( unsigned uTruths[][2], int nVarsMax, int nVarsReal, unsigned uTruth[], unsigned char * puPhases, unsigned uTruthRes[] )
{
    unsigned  uTruthPerm[2];
    int nMints, nPhases, m;

    nPhases = 0;
    nMints = (1 << nVarsReal);
    if ( nVarsMax < 6 )
    {
        uTruthRes[0] = MAP_MASK(32);
        for ( m = 0; m < nMints; m++ )
        {
            uTruthPerm[0] = Map_CanonComputePhase( uTruths, nVarsMax, uTruth[0], m );
            if ( uTruthRes[0] > uTruthPerm[0] )
            {
                uTruthRes[0] = uTruthPerm[0];
                nPhases      = 0;
                puPhases[nPhases++] = (unsigned char)m;
            }
            else if ( uTruthRes[0] == uTruthPerm[0] )
            {
                if ( nPhases < 4 ) // the max number of phases in Map_Super_t
                    puPhases[nPhases++] = (unsigned char)m;
            }
        }
        uTruthRes[1] = uTruthRes[0];
    }
    else
    {
        uTruthRes[0] = MAP_MASK(32);
        uTruthRes[1] = MAP_MASK(32);
        for ( m = 0; m < nMints; m++ )
        {
            Map_CanonComputePhase6( uTruths, nVarsMax, uTruth, m, uTruthPerm );
            if ( uTruthRes[1] > uTruthPerm[1] || (uTruthRes[1] == uTruthPerm[1] && uTruthRes[0] > uTruthPerm[0]) )
            {
                uTruthRes[0] = uTruthPerm[0];
                uTruthRes[1] = uTruthPerm[1];
                nPhases      = 0;
                puPhases[nPhases++] = (unsigned char)m;
            }
            else if ( uTruthRes[1] == uTruthPerm[1] && uTruthRes[0] == uTruthPerm[0] )
            {
                if ( nPhases < 4 ) // the max number of phases in Map_Super_t
                    puPhases[nPhases++] = (unsigned char)m;
            }
        }
    }
    assert( nPhases > 0 );
//    printf( "%d ", nPhases );
    return nPhases;
}

/**Function*************************************************************

  Synopsis    [Performs phase transformation for one function of less than 6 variables.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned Map_CanonComputePhase( unsigned uTruths[][2], int nVars, unsigned uTruth, unsigned uPhase )
{
    int v, Shift;
    for ( v = 0, Shift = 1; v < nVars; v++, Shift <<= 1 )
        if ( uPhase & Shift )
            uTruth = (((uTruth & ~uTruths[v][0]) << Shift) | ((uTruth & uTruths[v][0]) >> Shift));
    return uTruth;
}

/**Function*************************************************************

  Synopsis    [Performs phase transformation for one function of 6 variables.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Map_CanonComputePhase6( unsigned uTruths[][2], int nVars, unsigned uTruth[], unsigned uPhase, unsigned uTruthRes[] )
{
    unsigned uTemp;
    int v, Shift;

    // initialize the result
    uTruthRes[0] = uTruth[0];
    uTruthRes[1] = uTruth[1];
    if ( uPhase == 0 )
        return;
    // compute the phase 
    for ( v = 0, Shift = 1; v < nVars; v++, Shift <<= 1 )
        if ( uPhase & Shift )
        {
            if ( Shift < 32 )
            {
                uTruthRes[0] = (((uTruthRes[0] & ~uTruths[v][0]) << Shift) | ((uTruthRes[0] & uTruths[v][0]) >> Shift));
                uTruthRes[1] = (((uTruthRes[1] & ~uTruths[v][1]) << Shift) | ((uTruthRes[1] & uTruths[v][1]) >> Shift));
            }
            else
            {
                uTemp        = uTruthRes[0];
                uTruthRes[0] = uTruthRes[1];
                uTruthRes[1] = uTemp;
            }
        }
}

/**Function*************************************************************

  Synopsis    [Computes the N-canonical form of the Boolean function.]

  Description [The N-canonical form is defined as the truth table with 
  the minimum integer value. This function exhaustively enumerates 
  through the complete set of 2^N phase assignments.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Map_CanonComputeFast( Map_Man_t * p, int nVarsMax, int nVarsReal, unsigned uTruth[], unsigned char * puPhases, unsigned uTruthRes[] )
{
    unsigned uTruth0, uTruth1;
    unsigned uCanon0, uCanon1, uCanonBest;
    unsigned uPhaseBest = 16; // Suppress "might be used uninitialized" (asserts require < 16)
    int i, Limit;

    if ( nVarsMax == 6 )
        return Map_CanonComputeSlow( p->uTruths, nVarsMax, nVarsReal, uTruth, puPhases, uTruthRes );

    if ( nVarsReal < 5 )
    {
//        return Map_CanonComputeSlow( p->uTruths, nVarsMax, nVarsReal, uTruth, puPhases, uTruthRes );

        uTruth0 = uTruth[0] & 0xFFFF;
        assert( p->pCounters[uTruth0] > 0 );
        uTruthRes[0] = (p->uCanons[uTruth0] << 16) | p->uCanons[uTruth0];
        uTruthRes[1] = uTruthRes[0];
        puPhases[0] = p->uPhases[uTruth0][0];
        return 1;
    }

    assert( nVarsMax == 5 );
    assert( nVarsReal == 5 );
    uTruth0 = uTruth[0] & 0xFFFF;
    uTruth1 = (uTruth[0] >> 16);
    if ( uTruth1 == 0 )
    {
        uTruthRes[0] = p->uCanons[uTruth0];
        uTruthRes[1] = uTruthRes[0];
        Limit = (p->pCounters[uTruth0] > 4)? 4 : p->pCounters[uTruth0];
        for ( i = 0; i < Limit; i++ )
            puPhases[i] = p->uPhases[uTruth0][i];
        return Limit;
    }
    else if ( uTruth0 == 0 )
    {
        uTruthRes[0] = p->uCanons[uTruth1];
        uTruthRes[1] = uTruthRes[0];
        Limit = (p->pCounters[uTruth1] > 4)? 4 : p->pCounters[uTruth1];
        for ( i = 0; i < Limit; i++ )
        {
            puPhases[i] = p->uPhases[uTruth1][i];
            puPhases[i] |= (1 << 4);
        }
        return Limit;
    }
    uCanon0 = p->uCanons[uTruth0];
    uCanon1 = p->uCanons[uTruth1];
    if ( uCanon0 >= uCanon1 ) // using nCanon1 as the main one
    {
        assert( p->pCounters[uTruth1] > 0 );
        uCanonBest = 0xFFFFFFFF;
        for ( i = 0; i < p->pCounters[uTruth1]; i++ )
        {
            uCanon0 = Extra_TruthPolarize( uTruth0, p->uPhases[uTruth1][i], 4 );
            if ( uCanonBest > uCanon0 )
            {
                uCanonBest = uCanon0;
                uPhaseBest = p->uPhases[uTruth1][i];
                assert( uPhaseBest < 16 );
            }
        }
        uTruthRes[0] = (uCanon1 << 16) | uCanonBest;
        uTruthRes[1] = uTruthRes[0];
        puPhases[0] = uPhaseBest;
        return 1;
    }
    else if ( uCanon0 < uCanon1 )
    {
        assert( p->pCounters[uTruth0] > 0 );
        uCanonBest = 0xFFFFFFFF;
        for ( i = 0; i < p->pCounters[uTruth0]; i++ )
        {
            uCanon1 = Extra_TruthPolarize( uTruth1, p->uPhases[uTruth0][i], 4 );
            if ( uCanonBest > uCanon1 )
            {
                uCanonBest = uCanon1;
                uPhaseBest = p->uPhases[uTruth0][i];
                assert( uPhaseBest < 16 );
            }
        }
        uTruthRes[0] = (uCanon0 << 16) | uCanonBest;
        uTruthRes[1] = uTruthRes[0];
        puPhases[0] = uPhaseBest | (1 << 4);
        return 1;
    }
    else
    {
        assert( 0 );
        return Map_CanonComputeSlow( p->uTruths, nVarsMax, nVarsReal, uTruth, puPhases, uTruthRes );
    }
}





////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

