/**CFile****************************************************************

  FileName    [exor.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Exclusive sum-of-product minimization.]

  Synopsis    [Main procedure.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: exor.c,v 1.0 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

////////////////////////////////////////////////////////////////////////
///                                                                  ///
///                  Implementation of EXORCISM - 4                  ///
///              An Exclusive Sum-of-Product Minimizer               ///
///               Alan Mishchenko  <alanmi@ee.pdx.edu>               ///
///                                                                  ///
////////////////////////////////////////////////////////////////////////
///                                                                  ///
///                         Main Module                              ///
///                ESOP Minimization Task Coordinator                ///
///                                                                  ///
///          1) interprets command line                              ///  
///          2) calls the approapriate reading procedure             ///
///          3) calls the minimization module                        ///
///                                                                  ///
///  Ver. 1.0. Started - July 18, 2000. Last update - July 20, 2000  ///
///  Ver. 1.1. Started - July 24, 2000. Last update - July 29, 2000  ///
///  Ver. 1.4. Started -  Aug 10, 2000. Last update -  Aug 26, 2000  ///
///  Ver. 1.6. Started -  Sep 11, 2000. Last update -  Sep 15, 2000  ///
///  Ver. 1.7. Started -  Sep 20, 2000. Last update -  Sep 23, 2000  ///
///                                                                  ///
////////////////////////////////////////////////////////////////////////
///   This software was tested with the BDD package "CUDD", v.2.3.0  ///
///                          by Fabio Somenzi                        ///
///                  http://vlsi.colorado.edu/~fabio/                ///
////////////////////////////////////////////////////////////////////////

#include "exor.h"

ABC_NAMESPACE_IMPL_START

///////////////////////////////////////////////////////////////////////
///                      GLOBAL VARIABLES                            ///
////////////////////////////////////////////////////////////////////////

// information about the cube cover
cinfo g_CoverInfo;

extern int s_fDecreaseLiterals;

////////////////////////////////////////////////////////////////////////
///                       EXTERNAL FUNCTIONS                         ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                        FUNCTION main()                           ///
////////////////////////////////////////////////////////////////////////


/**Function*************************************************************

  Synopsis    [Number of negative literals.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
/*
static int QCost[16][16] = 
{
    {  1}, // 0
    {  1,   2}, // 1
    {  5,   5,   6}, // 2
    { 14,  14,  16,  18}, // 3
    { 20,  20,  20,  22,  24}, // 4
    { 32,  32,  32,  34,  36,  38}, // 5
    { 44,  44,  44,  44,  46,  48,  50}, // 6
    { 56,  56,  56,  56,  58,  60,  62,  64}, // 7
    { 0 }
};
*/
int GetQCost( int nVars, int nNegs )
{
    int Extra;
    assert( nVars >= nNegs );
    if ( nVars == 0 )  
        return 1;
    if ( nVars == 1 )  
    {
        if ( nNegs == 0 )  return 1;
        if ( nNegs == 1 )  return 2;
    }
    if ( nVars == 2 )
    {
        if ( nNegs <= 1 )  return 5;
        if ( nNegs == 2 )  return 6;
    }
    if ( nVars == 3 )
    {
        if ( nNegs <= 1 )  return 14;
        if ( nNegs == 2 )  return 16;
        if ( nNegs == 3 )  return 18;
    }
    Extra = nNegs - nVars/2;
    return 20 + 12 * (nVars - 4) + (Extra > 0 ? 2 * Extra : 0);

}
void GetQCostTest()
{
    int i, k, Limit = 10;
    for ( i = 0; i < Limit; i++ )
    {
        for ( k = 0; k <= i; k++ )
            printf( "%4d ", GetQCost(i, k) );
        printf( "\n" );
    }
}
int ComputeQCost( Vec_Int_t * vCube )
{
    int i, Entry, nLitsN = 0;
    Vec_IntForEachEntry( vCube, Entry, i )
        nLitsN += Abc_LitIsCompl(Entry);
    return GetQCost( Vec_IntSize(vCube), nLitsN );
}
int ComputeQCostBits( Cube * p )
{
    extern varvalue GetVar( Cube* pC, int Var );
    int v, nLits = 0, nLitsN = 0;
    for ( v = 0; v < g_CoverInfo.nVarsIn; v++ )
    {
        int Value = GetVar( p, v );
        if ( Value == VAR_NEG )
            nLitsN++;
        else if ( Value == VAR_POS )
            nLits++;
    }
    nLits += nLitsN;
    return GetQCost( nLits, nLitsN );
}
int ToffoliGateCount( int controls, int lines )
{
    switch ( controls )
    {
    case 0u:
    case 1u:
        return 0;
        break;
    case 2u:
        return 1;
        break;
    case 3u:
        return 4;
        break;
    case 4u:
        return ( ( ( lines + 1 ) / 2 ) >= controls ) ? 8 : 10;
        break;
    default:
        return ( ( ( lines + 1 ) / 2 ) >= controls ) ? 4 * ( controls - 2 ) : 8 * ( controls - 3 );
    }
}
int ComputeQCostTcount( Vec_Int_t * vCube )
{
    return 7 * ToffoliGateCount( Vec_IntSize( vCube ), g_CoverInfo.nVarsIn + 1 );
}
int ComputeQCostTcountBits( Cube * p )
{
    extern varvalue GetVar( Cube* pC, int Var );
    int v, nLits = 0;
    for ( v = 0; v < g_CoverInfo.nVarsIn; v++ )
        if ( GetVar( p, v ) != VAR_ABS )
            nLits++;
    return 7 * ToffoliGateCount( nLits, g_CoverInfo.nVarsIn + 1 );

    /* maybe just: 7 * ToffoliGateCount( p->a, g_CoverInfo.nVarsIn + 1 ); */
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int ReduceEsopCover()
{
    ///////////////////////////////////////////////////////////////
    // SIMPLIFICATION
    ////////////////////////////////////////////////////////////////////

    int nIterWithoutImprovement = 0;
    int nIterCount = 0;
    int GainTotal;
    int z;

    do
    {
//START:
        if ( g_CoverInfo.Verbosity == 2 )
            printf( "\nITERATION #%d\n\n", ++nIterCount );
        else if ( g_CoverInfo.Verbosity == 1 )
            printf( "." );

        GainTotal  = 0;
        GainTotal += IterativelyApplyExorLink2( 1|2|0 );
        GainTotal += IterativelyApplyExorLink3( 1|2|0 );

        GainTotal += IterativelyApplyExorLink2( 1|2|0 );
        GainTotal += IterativelyApplyExorLink3( 1|2|0 );

        GainTotal += IterativelyApplyExorLink2( 1|2|0 );
        GainTotal += IterativelyApplyExorLink3( 1|2|0 );

        GainTotal += IterativelyApplyExorLink2( 1|2|0 );
        GainTotal += IterativelyApplyExorLink3( 1|2|0 );

        GainTotal += IterativelyApplyExorLink2( 1|2|0 );
        GainTotal += IterativelyApplyExorLink3( 1|2|0 );

        GainTotal += IterativelyApplyExorLink2( 1|2|0 );
        GainTotal += IterativelyApplyExorLink3( 1|2|0 );

        if ( nIterWithoutImprovement > (int)(g_CoverInfo.Quality>0) )
        {
            GainTotal += IterativelyApplyExorLink2( 1|2|0 );
            GainTotal += IterativelyApplyExorLink3( 1|2|0 );
            GainTotal += IterativelyApplyExorLink2( 1|2|4 );
            GainTotal += IterativelyApplyExorLink3( 1|2|4 );
            GainTotal += IterativelyApplyExorLink2( 1|2|4 );
            GainTotal += IterativelyApplyExorLink4( 1|2|4 );
            GainTotal += IterativelyApplyExorLink2( 1|2|4 );
            GainTotal += IterativelyApplyExorLink4( 1|2|0 );

            GainTotal += IterativelyApplyExorLink2( 1|2|0 );
            GainTotal += IterativelyApplyExorLink3( 1|2|0 );
            GainTotal += IterativelyApplyExorLink2( 1|2|4 );
            GainTotal += IterativelyApplyExorLink3( 1|2|4 );
            GainTotal += IterativelyApplyExorLink2( 1|2|4 );
            GainTotal += IterativelyApplyExorLink4( 1|2|4 );
            GainTotal += IterativelyApplyExorLink2( 1|2|4 );
            GainTotal += IterativelyApplyExorLink4( 1|2|0 );
        }

        if ( GainTotal )
            nIterWithoutImprovement = 0;
        else
            nIterWithoutImprovement++;

//      if ( g_CoverInfo.Quality >= 2 && nIterWithoutImprovement == 2 )
//          s_fDecreaseLiterals = 1;
    }
    while ( nIterWithoutImprovement < 1 + g_CoverInfo.Quality );


    // improve the literal count
    s_fDecreaseLiterals = 1;
    for ( z = 0; z < 1; z++ )
    {
        if ( g_CoverInfo.Verbosity == 2 )
            printf( "\nITERATION #%d\n\n", ++nIterCount );
        else if ( g_CoverInfo.Verbosity == 1 )
            printf( "." );

        GainTotal  = 0;
        GainTotal += IterativelyApplyExorLink2( 1|2|0 );
        GainTotal += IterativelyApplyExorLink3( 1|2|0 );

        GainTotal += IterativelyApplyExorLink2( 1|2|0 );
        GainTotal += IterativelyApplyExorLink3( 1|2|0 );

        GainTotal += IterativelyApplyExorLink2( 1|2|0 );
        GainTotal += IterativelyApplyExorLink3( 1|2|0 );

        GainTotal += IterativelyApplyExorLink2( 1|2|0 );
        GainTotal += IterativelyApplyExorLink3( 1|2|0 );

        GainTotal += IterativelyApplyExorLink2( 1|2|0 );
        GainTotal += IterativelyApplyExorLink3( 1|2|0 );

//      if ( GainTotal )
//      {
//          nIterWithoutImprovement = 0;
//          goto START;
//      }           
    }


/*  ////////////////////////////////////////////////////////////////////
    // Print statistics
    printf( "\nShallow simplification time is ";
    cout << (float)(clk2 - clk1)/(float)(CLOCKS_PER_SEC) << " sec\n" );
    printf( "Deep simplification time is ";
    cout << (float)(Abc_Clock() - clk2)/(float)(CLOCKS_PER_SEC) << " sec\n" );
    printf( "Cover after iterative simplification = " << s_nCubesInUse << endl;
    printf( "Reduced by initial cube writing      = " << g_CoverInfo.nCubesBefore-nCubesAfterWriting << endl;
    printf( "Reduced by shallow simplification    = " << nCubesAfterWriting-nCubesAfterShallow << endl;
    printf( "Reduced by deep simplification       = " << nCubesAfterWriting-s_nCubesInUse << endl;

//  printf( "\nThe total number of cubes created = " << g_CoverInfo.cIDs << endl;
//  printf( "Total number of places in a queque = " << s_nPosAlloc << endl;
//  printf( "Minimum free places in queque-2 = " << s_nPosMax[0] << endl;
//  printf( "Minimum free places in queque-3 = " << s_nPosMax[1] << endl;
//  printf( "Minimum free places in queque-4 = " << s_nPosMax[2] << endl;
*/  ////////////////////////////////////////////////////////////////////

    // write the number of cubes into cover information 
    assert ( g_CoverInfo.nCubesInUse + g_CoverInfo.nCubesFree == g_CoverInfo.nCubesAlloc );

//  printf( "\nThe output cover is\n" );
//  PrintCoverDebug( cout );

    return 0;
}

//////////////////////////////////////////////////////////////////
// quite a good script
//////////////////////////////////////////////////////////////////
/*
    long clk1 = Abc_Clock();
    int nIterWithoutImprovement = 0;
    do
    {
        PrintQuequeStats();
        GainTotal  = 0;
        GainTotal += IterativelyApplyExorLink( DIST2, 0|2|0 );
        GainTotal += IterativelyApplyExorLink( DIST3, 0|2|0 );
        GainTotal += IterativelyApplyExorLink( DIST3, 1|2|0 );
        GainTotal += IterativelyApplyExorLink( DIST3, 1|2|0 );

        if ( nIterWithoutImprovement > 2 )
        {
            GainTotal += IterativelyApplyExorLink( DIST2, 0|0|4 );
            GainTotal += IterativelyApplyExorLink( DIST4, 0|2|4 );
            GainTotal += IterativelyApplyExorLink( DIST3, 1|2|4 );
            GainTotal += IterativelyApplyExorLink( DIST4, 1|2|0 );
        }

        if ( nIterWithoutImprovement > 6 )
        {
            GainTotal += IterativelyApplyExorLink( DIST2, 0|0|4 );
            GainTotal += IterativelyApplyExorLink( DIST4, 0|0|4 );
            GainTotal += IterativelyApplyExorLink( DIST4, 0|0|4 );
            GainTotal += IterativelyApplyExorLink( DIST4, 1|2|4 );
            GainTotal += IterativelyApplyExorLink( DIST4, 1|2|4 );
            GainTotal += IterativelyApplyExorLink( DIST4, 1|2|0 );
        }

        if ( GainTotal )
            nIterWithoutImprovement = 0;
        else
            nIterWithoutImprovement++;
    }
    while ( nIterWithoutImprovement < 12 );

    nCubesAfterShallow = s_nCubesInUse;

*/

/*
    // alu4 - 439
    long clk1 = Abc_Clock();
    int nIterWithoutImprovement = 0;
    do
    {
        PrintQuequeStats();
        GainTotal  = 0;
        GainTotal += IterativelyApplyExorLink( DIST2, 1|0|0 );
        GainTotal += IterativelyApplyExorLink( DIST2, 1|2|0 );
        GainTotal += IterativelyApplyExorLink( DIST2, 1|2|0 );
        GainTotal += IterativelyApplyExorLink( DIST3, 1|2|0 );
        GainTotal += IterativelyApplyExorLink( DIST3, 1|2|0 );
        GainTotal += IterativelyApplyExorLink( DIST3, 1|2|0 );

        if ( nIterWithoutImprovement > 2 )
        {
            GainTotal += IterativelyApplyExorLink( DIST2, 0|0|4 );
            GainTotal += IterativelyApplyExorLink( DIST4, 0|2|4 );
            GainTotal += IterativelyApplyExorLink( DIST3, 1|2|4 );
            GainTotal += IterativelyApplyExorLink( DIST4, 1|2|0 );
        }

        if ( nIterWithoutImprovement > 6 )
        {
            GainTotal += IterativelyApplyExorLink( DIST2, 0|0|4 );
            GainTotal += IterativelyApplyExorLink( DIST4, 0|0|4 );
            GainTotal += IterativelyApplyExorLink( DIST4, 0|0|4 );
            GainTotal += IterativelyApplyExorLink( DIST4, 1|2|4 );
            GainTotal += IterativelyApplyExorLink( DIST4, 1|2|4 );
            GainTotal += IterativelyApplyExorLink( DIST4, 1|2|0 );
        }

        if ( GainTotal )
            nIterWithoutImprovement = 0;
        else
            nIterWithoutImprovement++;
    }
    while ( nIterWithoutImprovement < 12 );
*/

/*
// alu4 - 412 cubes, 700 sec

    long clk1 = Abc_Clock();
    int nIterWithoutImprovement = 0;
    int nIterCount = 0;
    do
    {
        printf( "\nITERATION #" << ++nIterCount << endl << endl;

        GainTotal  = 0;
        GainTotal += IterativelyApplyExorLink2( 1|2|0 );
        GainTotal += IterativelyApplyExorLink2( 1|2|0 );
        GainTotal += IterativelyApplyExorLink3( 1|2|0 );
        GainTotal += IterativelyApplyExorLink3( 1|2|0 );
        GainTotal += IterativelyApplyExorLink3( 1|2|0 );
        GainTotal += IterativelyApplyExorLink3( 1|2|0 );

        if ( nIterWithoutImprovement > 3 )
        {
            GainTotal += IterativelyApplyExorLink2( 1|2|0 );
            GainTotal += IterativelyApplyExorLink3( 1|2|0 );
            GainTotal += IterativelyApplyExorLink3( 1|2|4 );
            GainTotal += IterativelyApplyExorLink2( 1|2|4 );
            GainTotal += IterativelyApplyExorLink3( 1|2|4 );
            GainTotal += IterativelyApplyExorLink4( 1|2|4 );
            GainTotal += IterativelyApplyExorLink3( 1|2|4 );
            GainTotal += IterativelyApplyExorLink4( 1|2|4 );
        }

        if ( nIterWithoutImprovement > 7 )
        {
            GainTotal += IterativelyApplyExorLink2( 1|2|0 );
            GainTotal += IterativelyApplyExorLink3( 1|2|0 );
            GainTotal += IterativelyApplyExorLink2( 1|2|4 );
            GainTotal += IterativelyApplyExorLink3( 1|2|4 );
            GainTotal += IterativelyApplyExorLink3( 1|2|4 );
            GainTotal += IterativelyApplyExorLink4( 1|2|4 );
            GainTotal += IterativelyApplyExorLink4( 1|2|4 );
            GainTotal += IterativelyApplyExorLink4( 1|2|4 );
            GainTotal += IterativelyApplyExorLink3( 1|2|4 );
            GainTotal += IterativelyApplyExorLink4( 1|2|4 );
            GainTotal += IterativelyApplyExorLink4( 1|2|4 );
            GainTotal += IterativelyApplyExorLink4( 1|2|4 );
        }

        if ( GainTotal )
            nIterWithoutImprovement = 0;
        else
            nIterWithoutImprovement++;
    }
    while ( nIterWithoutImprovement < 12 );
*/

/*
// pretty good script
// alu4 = 424   in 250 sec

    long clk1 = Abc_Clock();
    int nIterWithoutImprovement = 0;
    int nIterCount = 0;
    do
    {
        printf( "\nITERATION #" << ++nIterCount << "   |";
        for ( int k = 0; k < nIterWithoutImprovement; k++ )
            printf( "*";
        for ( ; k < 11; k++ )
            printf( "_";
        printf( "|" << endl << endl;

        GainTotal  = 0;
        GainTotal += IterativelyApplyExorLink( DIST2, 1|2|0 );
        GainTotal += IterativelyApplyExorLink( DIST2, 1|2|0 );
        GainTotal += IterativelyApplyExorLink( DIST3, 1|2|0 );
        GainTotal += IterativelyApplyExorLink( DIST3, 1|2|0 );
        GainTotal += IterativelyApplyExorLink( DIST3, 1|2|0 );

        if ( nIterWithoutImprovement > 2 )
        {
            GainTotal += IterativelyApplyExorLink( DIST2, 1|2|0 );
            GainTotal += IterativelyApplyExorLink( DIST3, 1|2|0 );
            GainTotal += IterativelyApplyExorLink( DIST2, 1|2|4 );
            GainTotal += IterativelyApplyExorLink( DIST3, 1|2|4 );
            GainTotal += IterativelyApplyExorLink( DIST4, 1|2|4 );
            GainTotal += IterativelyApplyExorLink( DIST3, 1|2|4 );
            GainTotal += IterativelyApplyExorLink( DIST4, 1|2|4 );
        }

        if ( nIterWithoutImprovement > 4 )
        {
            GainTotal += IterativelyApplyExorLink( DIST2, 1|2|0 );
            GainTotal += IterativelyApplyExorLink( DIST3, 1|2|0 );
            GainTotal += IterativelyApplyExorLink( DIST2, 1|2|4 );
            GainTotal += IterativelyApplyExorLink( DIST3, 1|2|4 );
            GainTotal += IterativelyApplyExorLink( DIST4, 1|2|4 );
            GainTotal += IterativelyApplyExorLink( DIST4, 1|2|4 );
            GainTotal += IterativelyApplyExorLink( DIST2, 1|2|4 );
            GainTotal += IterativelyApplyExorLink( DIST3, 1|2|4 );
            GainTotal += IterativelyApplyExorLink( DIST4, 1|2|4 );
            GainTotal += IterativelyApplyExorLink( DIST4, 1|2|4 );
        }

        if ( GainTotal )
            nIterWithoutImprovement = 0;
        else
            nIterWithoutImprovement++;
    }
    while ( nIterWithoutImprovement < 7 );
*/

/*
alu4 = 435   70 secs

    long clk1 = Abc_Clock();
    int nIterWithoutImprovement = 0;
    int nIterCount = 0;

    do
    {
        printf( "\nITERATION #" << ++nIterCount << endl << endl;

        GainTotal  = 0;
        GainTotal += IterativelyApplyExorLink( DIST2, 1|2|0 );
        GainTotal += IterativelyApplyExorLink( DIST3, 1|2|0 );

        GainTotal += IterativelyApplyExorLink( DIST2, 1|2|0 );
        GainTotal += IterativelyApplyExorLink( DIST3, 1|2|0 );

        GainTotal += IterativelyApplyExorLink( DIST2, 1|2|0 );
        GainTotal += IterativelyApplyExorLink( DIST3, 1|2|0 );

        GainTotal += IterativelyApplyExorLink( DIST2, 1|2|0 );
        GainTotal += IterativelyApplyExorLink( DIST3, 1|2|0 );

        GainTotal += IterativelyApplyExorLink( DIST2, 1|2|0 );
        GainTotal += IterativelyApplyExorLink( DIST3, 1|2|0 );

        GainTotal += IterativelyApplyExorLink( DIST2, 1|2|0 );
        GainTotal += IterativelyApplyExorLink( DIST3, 1|2|0 );


        if ( GainTotal )
            nIterWithoutImprovement = 0;
        else
            nIterWithoutImprovement++;
    }
    while ( nIterWithoutImprovement < 4 );
*/

/*
  // the best previous
  
    long clk1 = Abc_Clock();
    int nIterWithoutImprovement = 0;
    int nIterCount = 0;
    int GainTotal;
    do
    {
        if ( g_CoverInfo.Verbosity == 2 )
        printf( "\nITERATION #" << ++nIterCount << endl << endl;
        else if ( g_CoverInfo.Verbosity == 1 )
        cout << '.';

        GainTotal  = 0;
        GainTotal += IterativelyApplyExorLink( DIST2, 1|2|0 );
        GainTotal += IterativelyApplyExorLink( DIST3, 1|2|0 );

        GainTotal += IterativelyApplyExorLink( DIST2, 1|2|0 );
        GainTotal += IterativelyApplyExorLink( DIST3, 1|2|0 );

        GainTotal += IterativelyApplyExorLink( DIST2, 1|2|0 );
        GainTotal += IterativelyApplyExorLink( DIST3, 1|2|0 );

        GainTotal += IterativelyApplyExorLink( DIST2, 1|2|0 );
        GainTotal += IterativelyApplyExorLink( DIST3, 1|2|0 );

        GainTotal += IterativelyApplyExorLink( DIST2, 1|2|0 );
        GainTotal += IterativelyApplyExorLink( DIST3, 1|2|0 );

        GainTotal += IterativelyApplyExorLink( DIST2, 1|2|0 );
        GainTotal += IterativelyApplyExorLink( DIST3, 1|2|0 );

        if ( nIterWithoutImprovement > 1 )
        {
            GainTotal += IterativelyApplyExorLink( DIST2, 1|2|0 );
            GainTotal += IterativelyApplyExorLink( DIST3, 1|2|0 );
            GainTotal += IterativelyApplyExorLink( DIST2, 1|2|0 );
            GainTotal += IterativelyApplyExorLink( DIST3, 1|2|4 );
            GainTotal += IterativelyApplyExorLink( DIST2, 1|2|4 );
            GainTotal += IterativelyApplyExorLink( DIST4, 1|2|4 );
            GainTotal += IterativelyApplyExorLink( DIST2, 1|2|4 );
            GainTotal += IterativelyApplyExorLink( DIST4, 1|2|0 );

            GainTotal += IterativelyApplyExorLink( DIST2, 1|2|0 );
            GainTotal += IterativelyApplyExorLink( DIST3, 1|2|0 );
            GainTotal += IterativelyApplyExorLink( DIST2, 1|2|0 );
            GainTotal += IterativelyApplyExorLink( DIST3, 1|2|4 );
            GainTotal += IterativelyApplyExorLink( DIST2, 1|2|4 );
            GainTotal += IterativelyApplyExorLink( DIST4, 1|2|4 );
            GainTotal += IterativelyApplyExorLink( DIST2, 1|2|4 );
            GainTotal += IterativelyApplyExorLink( DIST4, 1|2|0 );
        }

        if ( GainTotal )
            nIterWithoutImprovement = 0;
        else
            nIterWithoutImprovement++;
    }
//  while ( nIterWithoutImprovement < 20 );
//  while ( nIterWithoutImprovement < 7 );
    while ( nIterWithoutImprovement < 1 + g_CoverInfo.Quality );

*/

/*
// the last tried

    long clk1 = Abc_Clock();
    int nIterWithoutImprovement = 0;
    int nIterCount = 0;
    int GainTotal;
    do
    {
        if ( g_CoverInfo.Verbosity == 2 )
        printf( "\nITERATION #" << ++nIterCount << endl << endl;
        else if ( g_CoverInfo.Verbosity == 1 )
        cout << '.';

        GainTotal  = 0;
        GainTotal += IterativelyApplyExorLink2( 1|2|0 );
        GainTotal += IterativelyApplyExorLink3( 1|2|0 );

        GainTotal += IterativelyApplyExorLink2( 1|2|0 );
        GainTotal += IterativelyApplyExorLink3( 1|2|0 );

        GainTotal += IterativelyApplyExorLink2( 1|2|0 );
        GainTotal += IterativelyApplyExorLink3( 1|2|0 );

        GainTotal += IterativelyApplyExorLink2( 1|2|0 );
        GainTotal += IterativelyApplyExorLink3( 1|2|0 );

        GainTotal += IterativelyApplyExorLink2( 1|2|0 );
        GainTotal += IterativelyApplyExorLink3( 1|2|0 );

        if ( nIterWithoutImprovement > (int)(g_CoverInfo.Quality>0) )
        {
            GainTotal += IterativelyApplyExorLink2( 1|2|0 );
            GainTotal += IterativelyApplyExorLink3( 1|2|0 );
            GainTotal += IterativelyApplyExorLink2( 1|2|0 );
            GainTotal += IterativelyApplyExorLink3( 1|2|4 );
            GainTotal += IterativelyApplyExorLink2( 1|2|4 );
            GainTotal += IterativelyApplyExorLink4( 1|2|4 );
            GainTotal += IterativelyApplyExorLink2( 1|2|4 );
            GainTotal += IterativelyApplyExorLink4( 1|2|0 );

            GainTotal += IterativelyApplyExorLink2( 1|2|0 );
            GainTotal += IterativelyApplyExorLink3( 1|2|0 );
            GainTotal += IterativelyApplyExorLink2( 1|2|0 );
            GainTotal += IterativelyApplyExorLink3( 1|2|4 );
            GainTotal += IterativelyApplyExorLink2( 1|2|4 );
            GainTotal += IterativelyApplyExorLink4( 1|2|4 );
            GainTotal += IterativelyApplyExorLink2( 1|2|4 );
            GainTotal += IterativelyApplyExorLink4( 1|2|0 );
        }

        if ( GainTotal )
            nIterWithoutImprovement = 0;
        else
            nIterWithoutImprovement++;
    }
    while ( nIterWithoutImprovement < 1 + g_CoverInfo.Quality );
*/

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void AddCubesToStartingCover( Vec_Wec_t * vEsop )
{
    Vec_Int_t * vCube;
    Cube * pNew;
    int * s_Level2Var;
    int * s_LevelValues;
    int c, i, k, Lit, Out;

    s_Level2Var = ABC_ALLOC( int, g_CoverInfo.nVarsIn );
    s_LevelValues = ABC_ALLOC( int, g_CoverInfo.nVarsIn );

    for ( i = 0; i < g_CoverInfo.nVarsIn; i++ )
        s_Level2Var[i] = i;

    g_CoverInfo.nLiteralsBefore = 0;
    g_CoverInfo.QCostBefore = 0;
    Vec_WecForEachLevel( vEsop, vCube, c )
    {
        // get the output of this cube
        Out = -Vec_IntPop(vCube) - 1;

        // fill in the cube with blanks
        for ( i = 0; i < g_CoverInfo.nVarsIn; i++ )
            s_LevelValues[i] = VAR_ABS;
        Vec_IntForEachEntry( vCube, Lit, k )
        {
            if ( Abc_LitIsCompl(Lit) )
                s_LevelValues[Abc_Lit2Var(Lit)] = VAR_NEG;
            else
                s_LevelValues[Abc_Lit2Var(Lit)] = VAR_POS;
        }

        // get the new cube
        pNew = GetFreeCube();
        // consider the need to clear the cube
        if ( pNew->pCubeDataIn[0] ) // this is a recycled cube
        {
            for ( i = 0; i < g_CoverInfo.nWordsIn; i++ )
                pNew->pCubeDataIn[i] = 0;
            for ( i = 0; i < g_CoverInfo.nWordsOut; i++ )
                pNew->pCubeDataOut[i] = 0;
        }

        InsertVarsWithoutClearing( pNew, s_Level2Var, g_CoverInfo.nVarsIn, s_LevelValues, Out );
        // set literal counts
        pNew->a = Vec_IntSize(vCube);
        pNew->z = 1;
        pNew->q = ComputeQCost(vCube);
        // set the ID
        pNew->ID = g_CoverInfo.cIDs++;
        // skip through zero-ID
        if ( g_CoverInfo.cIDs == 256 )
            g_CoverInfo.cIDs = 1;

        // add this cube to storage
        CheckForCloseCubes( pNew, 1 );

        g_CoverInfo.nLiteralsBefore += Vec_IntSize(vCube);
        g_CoverInfo.QCostBefore += ComputeQCost(vCube);
    }
    ABC_FREE( s_Level2Var );
    ABC_FREE( s_LevelValues );

    assert ( g_CoverInfo.nCubesInUse + g_CoverInfo.nCubesFree == g_CoverInfo.nCubesAlloc );
}

/**Function*************************************************************

  Synopsis    [Performs heuristic minimization of ESOPs.]

  Description [Returns 1 on success, 0 on failure.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Exorcism( Vec_Wec_t * vEsop, int nIns, int nOuts, char * pFileNameOut )
{
    abctime clk1;
    int RemainderBits;
    int TotalWords;
    int MemTemp, MemTotal;

    ///////////////////////////////////////////////////////////////////////
    // STEPS of HEURISTIC ESOP MINIMIZATION
    ///////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////
    // STEP 1: determine the size of the starting cover
    ///////////////////////////////////////////////////////////////////////
    assert( nIns > 0 );
    // inputs
    RemainderBits = (nIns*2)%(sizeof(unsigned)*8);
    TotalWords    = (nIns*2)/(sizeof(unsigned)*8) + (RemainderBits > 0);
    g_CoverInfo.nVarsIn  = nIns;
    g_CoverInfo.nWordsIn = TotalWords;
    // outputs
    RemainderBits = (nOuts)%(sizeof(unsigned)*8);
    TotalWords    = (nOuts)/(sizeof(unsigned)*8) + (RemainderBits > 0);
    g_CoverInfo.nVarsOut  = nOuts;
    g_CoverInfo.nWordsOut = TotalWords;
    g_CoverInfo.cIDs = 1;

    // cubes
    clk1 = Abc_Clock();
//  g_CoverInfo.nCubesBefore = CountTermsInPseudoKroneckerCover( g_Func.dd, nOuts );
    g_CoverInfo.nCubesBefore = Vec_WecSize(vEsop);
    g_CoverInfo.TimeStart = Abc_Clock() - clk1;

    if ( g_CoverInfo.Verbosity )
    {
    printf( "Starting cover generation time is %.2f sec\n", TICKS_TO_SECONDS(g_CoverInfo.TimeStart) );
    printf( "The number of cubes in the starting cover is %d\n", g_CoverInfo.nCubesBefore );
    }

    if ( g_CoverInfo.nCubesBefore > g_CoverInfo.nCubesMax )
    {
        printf( "\nThe size of the starting cover is more than %d cubes. Quitting...\n", g_CoverInfo.nCubesMax );
        return 0;
    }

    ///////////////////////////////////////////////////////////////////////
    // STEP 2: prepare internal data structures
    ///////////////////////////////////////////////////////////////////////
    g_CoverInfo.nCubesAlloc = g_CoverInfo.nCubesBefore + ADDITIONAL_CUBES; 

    // allocate cube cover
    MemTotal = 0;
    MemTemp = AllocateCover( g_CoverInfo.nCubesAlloc, g_CoverInfo.nWordsIn, g_CoverInfo.nWordsOut );
    if ( MemTemp == 0 )
    {
        printf( "Unexpected memory allocation problem. Quitting...\n" );
        return 0;
    }
    else 
        MemTotal += MemTemp;

    // allocate cube sets
    MemTemp = AllocateCubeSets( g_CoverInfo.nVarsIn, g_CoverInfo.nVarsOut );
    if ( MemTemp == 0 )
    {
        printf( "Unexpected memory allocation problem. Quitting...\n" );
        return 0;
    }
    else 
        MemTotal += MemTemp;

    // allocate adjacency queques
    MemTemp = AllocateQueques( g_CoverInfo.nCubesAlloc*g_CoverInfo.nCubesAlloc/CUBE_PAIR_FACTOR );
    if ( MemTemp == 0 )
    {
        printf( "Unexpected memory allocation problem. Quitting...\n" );
        return 0;
    }
    else 
        MemTotal += MemTemp;

    if ( g_CoverInfo.Verbosity )
    printf( "Dynamically allocated memory is %dK\n",  MemTotal/1000 );

    ///////////////////////////////////////////////////////////////////////
    // STEP 3: write the cube cover into the allocated storage
    ///////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////
    clk1 = Abc_Clock();
    if ( g_CoverInfo.Verbosity )
    printf( "Generating the starting cover...\n" );
    AddCubesToStartingCover( vEsop );
    ///////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////
    // STEP 4: iteratively improve the cover 
    ///////////////////////////////////////////////////////////////////////
    if ( g_CoverInfo.Verbosity )
    printf( "Performing minimization...\n" );
    clk1 = Abc_Clock();
    ReduceEsopCover();
    g_CoverInfo.TimeMin = Abc_Clock() - clk1;
//  g_Func.TimeMin = (float)(Abc_Clock() - clk1)/(float)(CLOCKS_PER_SEC);
    if ( g_CoverInfo.Verbosity )
    {
    printf( "\nMinimization time is %.2f sec\n", TICKS_TO_SECONDS(g_CoverInfo.TimeMin) );
    printf( "\nThe number of cubes after minimization is %d\n", g_CoverInfo.nCubesInUse );
    }

    ///////////////////////////////////////////////////////////////////////
    // STEP 5: save the cover into file 
    ///////////////////////////////////////////////////////////////////////
    // if option is MULTI_OUTPUT, the output is written into the output file;
    // if option is SINGLE_NODE, the output is added to the input file
    // and written into the output file; in this case, the minimized nodes is
    // also stored in the temporary file "temp.blif" for verification

    // create the file name and write the output
    {
        char Buffer[1000];
        sprintf( Buffer, "%s", pFileNameOut ? pFileNameOut : "temp.esop" );
        WriteResultIntoFile( Buffer );
        if ( g_CoverInfo.Verbosity )
            printf( "Minimized cover has been written into file <%s>\n", Buffer );
    }

    ///////////////////////////////////////////////////////////////////////
    // STEP 6: delocate memory
    ///////////////////////////////////////////////////////////////////////
    DelocateCubeSets();
    DelocateCover();
    DelocateQueques();

    // return success
    return 1;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_ExorcismMain( Vec_Wec_t * vEsop, int nIns, int nOuts, char * pFileNameOut, int Quality, int Verbosity, int nCubesMax, int fUseQCost )
{
    memset( &g_CoverInfo, 0, sizeof(cinfo) );
    g_CoverInfo.Quality = Quality;
    g_CoverInfo.Verbosity = Verbosity;
    g_CoverInfo.nCubesMax = nCubesMax;
    g_CoverInfo.fUseQCost = fUseQCost;
    if ( fUseQCost )
        s_fDecreaseLiterals = 1;
    if ( g_CoverInfo.Verbosity )
    {
        printf( "\nEXORCISM, Ver.4.7: Exclusive Sum-of-Product Minimizer\n" );
        printf( "by Alan Mishchenko, Portland State University, July-September 2000\n\n" );
        printf( "Incoming ESOP has %d inputs, %d outputs, and %d cubes.\n", nIns, nOuts, Vec_WecSize(vEsop) );
    }
    PrepareBitSetModule();
    if ( Exorcism( vEsop, nIns, nOuts, pFileNameOut ) == 0 )
    {
        printf( "Something went wrong when minimizing the cover\n" );
        return 0;
    }
    return 1;
}



///////////////////////////////////////////////////////////////////
////////////              End of File             /////////////////
///////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

