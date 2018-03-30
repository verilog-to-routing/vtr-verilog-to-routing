/**CFile****************************************************************

  FileName    [exorUtil.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Exclusive sum-of-product minimization.]

  Synopsis    [Utilities.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: exorUtil.c,v 1.0 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

////////////////////////////////////////////////////////////////////////
///                                                                  ///
///                  Implementation of EXORCISM - 4                  ///
///              An Exclusive Sum-of-Product Minimizer               ///
///               Alan Mishchenko  <alanmi@ee.pdx.edu>               ///
///                                                                  ///
////////////////////////////////////////////////////////////////////////
///                                                                  ///
///                        Utility Functions                         ///
///                                                                  ///
///       1) allocating memory for and creating the ESOP cover       ///
///       2) writing the resultant cover into an ESOP PLA file       ///
///                                                                  ///
///  Ver. 1.0. Started - July 15, 2000. Last update - July 20, 2000  ///
///  Ver. 1.4. Started -  Aug 10, 2000. Last update -  Aug 10, 2000  ///
///  Ver. 1.5. Started -  Aug 19, 2000. Last update -  Aug 19, 2000  ///
///  Ver. 1.7. Started -  Sep 20, 2000. Last update -  Sep 23, 2000  ///
///                                                                  ///
////////////////////////////////////////////////////////////////////////
///   This software was tested with the BDD package "CUDD", v.2.3.0  ///
///                          by Fabio Somenzi                        ///
///                  http://vlsi.colorado.edu/~fabio/                ///
////////////////////////////////////////////////////////////////////////

#include "exor.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                      EXTERNAL VARIABLES                         ////
////////////////////////////////////////////////////////////////////////

// information about the options, the function, and the cover
extern cinfo g_CoverInfo;

////////////////////////////////////////////////////////////////////////
///                        EXTERNAL FUNCTIONS                        ///
////////////////////////////////////////////////////////////////////////

// Cube Cover Iterator
// starts an iterator that traverses all the cubes in the ring
extern Cube* IterCubeSetStart();
// returns the next cube in the ring
extern Cube* IterCubeSetNext();

// retrieves the variable from the cube
extern varvalue GetVar( Cube* pC, int Var );

////////////////////////////////////////////////////////////////////////
///                      FUNCTION DECLARATIONS                       ///
////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////
////////////       Cover Service Procedures       /////////////////
///////////////////////////////////////////////////////////////////

int CountLiterals()
{
    Cube* p;
    int LitCounter = 0;
    for ( p = IterCubeSetStart( ); p; p = IterCubeSetNext() )
        LitCounter += p->a;
    return LitCounter;
}

int CountLiteralsCheck()
{
    Cube* p;
    int Value, v;
    int LitCounter = 0;
    int LitCounterControl = 0;

    for ( p = IterCubeSetStart( ); p; p = IterCubeSetNext() )
    {
        LitCounterControl += p->a;

        assert( p->fMark == 0 );

        // write the input variables
        for ( v = 0; v < g_CoverInfo.nVarsIn; v++ )
        {
            Value = GetVar( p, v );
            if ( Value == VAR_NEG )
                LitCounter++;
            else if ( Value == VAR_POS )
                LitCounter++;
            else if ( Value != VAR_ABS )
            {
                assert(0);
            }
        }
    }

    if ( LitCounterControl != LitCounter )
        printf( "Warning! The recorded number of literals (%d) differs from the actual number (%d)\n", LitCounterControl, LitCounter );
    return LitCounter;
}

int CountQCost()
{
    Cube* p;
    int QCost = 0;
    int QCostControl = 0;
    for ( p = IterCubeSetStart( ); p; p = IterCubeSetNext() )
    {
        QCostControl += p->q;
        QCost += ComputeQCostBits( p );
    }
//    if ( QCostControl != QCost )
//        printf( "Warning! The recorded number of literals (%d) differs from the actual number (%d)\n", QCostControl, QCost );
    return QCost;
}


void WriteTableIntoFile( FILE * pFile )
// nCubesAlloc is the number of allocated cubes 
{
    int v, w;
    Cube * p;
    int cOutputs;
    int nOutput;
    int WordSize;

    for ( p = IterCubeSetStart( ); p; p = IterCubeSetNext() )
    {
        assert( p->fMark == 0 );

        // write the input variables
        for ( v = 0; v < g_CoverInfo.nVarsIn; v++ )
        {
            int Value = GetVar( p, v );
            if ( Value == VAR_NEG )
                fprintf( pFile, "0" );
            else if ( Value == VAR_POS )
                fprintf( pFile, "1" );
            else if ( Value == VAR_ABS )
                fprintf( pFile, "-" );
            else
                assert(0);
        }
        fprintf( pFile, " " );

        // write the output variables
        cOutputs = 0;
        nOutput = g_CoverInfo.nVarsOut;
        WordSize = 8*sizeof( unsigned );
        for ( w = 0; w < g_CoverInfo.nWordsOut; w++ )
            for ( v = 0; v < WordSize; v++ )
            {
                if ( p->pCubeDataOut[w] & (1<<v) )
                    fprintf( pFile, "1" );
                else
                    fprintf( pFile, "0" );
                if ( ++cOutputs == nOutput )
                    break;
            }
        fprintf( pFile, "\n" );
    }
}


int WriteResultIntoFile( char * pFileName )
// write the ESOP cover into the PLA file <NewFileName>
{
    FILE * pFile;
    time_t ltime;
    char * TimeStr;

    pFile = fopen( pFileName, "w" );
    if ( pFile == NULL )
    {
        fprintf( pFile, "\n\nCannot open the output file\n" );
        return 1;
    }

    // get current time
    time( &ltime );
    TimeStr = asctime( localtime( &ltime ) );
    // get the number of literals
    g_CoverInfo.nLiteralsAfter = CountLiteralsCheck();
    g_CoverInfo.QCostAfter = CountQCost();
    fprintf( pFile, "# EXORCISM-4 output for command line arguments: " );
    fprintf( pFile, "\"-Q %d -V %d\"\n", g_CoverInfo.Quality, g_CoverInfo.Verbosity );
    fprintf( pFile, "# Minimization performed %s", TimeStr );
    fprintf( pFile, "# Initial statistics: " );
    fprintf( pFile, "Cubes = %d  Literals = %d  QCost = %d\n", g_CoverInfo.nCubesBefore, g_CoverInfo.nLiteralsBefore, g_CoverInfo.QCostBefore );
    fprintf( pFile, "# Final   statistics: " );
    fprintf( pFile, "Cubes = %d  Literals = %d  QCost = %d\n", g_CoverInfo.nCubesInUse, g_CoverInfo.nLiteralsAfter, g_CoverInfo.QCostAfter );
    fprintf( pFile, "# File reading and reordering time = %.2f sec\n", TICKS_TO_SECONDS(g_CoverInfo.TimeRead) );
    fprintf( pFile, "# Starting cover generation time   = %.2f sec\n", TICKS_TO_SECONDS(g_CoverInfo.TimeStart) );
    fprintf( pFile, "# Pure ESOP minimization time      = %.2f sec\n", TICKS_TO_SECONDS(g_CoverInfo.TimeMin) );
    fprintf( pFile, ".i %d\n", g_CoverInfo.nVarsIn );
    fprintf( pFile, ".o %d\n", g_CoverInfo.nVarsOut );
    fprintf( pFile, ".p %d\n", g_CoverInfo.nCubesInUse );
    fprintf( pFile, ".type esop\n" );
    WriteTableIntoFile( pFile );
    fprintf( pFile, ".e\n" );
    fclose( pFile );
    return 0;
}

///////////////////////////////////////////////////////////////////
////////////              End of File             /////////////////
///////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

