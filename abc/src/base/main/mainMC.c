/**CFile****************************************************************

  FileName    [mainMC.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [The main package.]

  Synopsis    [The main file for the model checker.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: main.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "mainInt.h"
#include "aig/aig/aig.h"
#include "aig/saig/saig.h"
#include "aig/fra/fra.h"
#include "aig/ioa/ioa.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [The main() procedure.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int main( int argc, char * argv[] )
{
    int fEnableBmcOnly = 0;  // enable to make it BMC-only

    int fEnableCounter = 1;  // should be 1 in the final version
    int fEnableComment = 0;  // should be 0 in the final version

    Fra_Sec_t SecPar, * pSecPar = &SecPar;
    FILE * pFile;
    Aig_Man_t * pAig;
    int RetValue = -1;
    int Depth    = -1;
    // BMC parameters
    int nFrames  = 50;
    int nSizeMax = 500000;
    int nBTLimit = 10000;
    int fRewrite = 0;
    int fNewAlgo = 1;
    int fVerbose = 0;
    clock_t clkTotal = clock();

    if ( argc != 2 )
    {
        printf( "  Expecting one command-line argument (an input file in AIGER format).\n" );
        printf( "  usage: %s <file.aig>\n", argv[0] );
        return 0;
    }
    pFile = fopen( argv[1], "r" );
    if ( pFile == NULL )
    {
        printf( "  Cannot open input AIGER file \"%s\".\n", argv[1] );
        printf( "  usage: %s <file.aig>\n", argv[0] );
        return 0;
    }
    fclose( pFile );
    pAig = Ioa_ReadAiger( argv[1], 1 );
    if ( pAig == NULL )
    {
        printf( "  Parsing the AIGER file \"%s\" has failed.\n", argv[1] );
        printf( "  usage: %s <file.aig>\n", argv[0] );
        return 0;
    }

    Aig_ManSetRegNum( pAig, pAig->nRegs );
    if ( !fEnableBmcOnly )
    {
        // perform BMC
        if ( pAig->nRegs != 0 )
            RetValue = Saig_ManBmcSimple( pAig, nFrames, nSizeMax, nBTLimit, fRewrite, fVerbose, NULL, 0, 0 );

        // perform full-blown SEC
        if ( RetValue != 0 )
        {
            extern void Dar_LibStart();
            extern void Dar_LibStop();
            extern void Cnf_ManFree();

            Fra_SecSetDefaultParams( pSecPar );
            pSecPar->TimeLimit       = 600;
            pSecPar->nFramesMax      =   4;  // the max number of frames used for induction
            pSecPar->fPhaseAbstract  =   0;  // disable phase-abstraction
            pSecPar->fSilent         =   1;  // disable phase-abstraction

            Dar_LibStart();
            RetValue = Fra_FraigSec( pAig, pSecPar, NULL );
            Dar_LibStop();
            Cnf_ManFree();
        }
    }

    // perform BMC again
    if ( RetValue == -1 && pAig->nRegs != 0 )
    {
        int nFrames  = 200;
        int nSizeMax = 500000;
        int nBTLimit = 10000000;
        int fRewrite = 0;
        RetValue = Saig_ManBmcSimple( pAig, nFrames, nSizeMax, nBTLimit, fRewrite, fVerbose, &Depth, 0, 0 );
        if ( RetValue != 0 )
            RetValue = -1;
    }

    // decide how to report the output
    pFile = stdout;

    // report the result
    if ( RetValue == 0 ) 
    {
//        fprintf(stdout, "s SATIFIABLE\n");
        fprintf( pFile, "1" );
        if ( fEnableCounter  )
        {
        printf( "\n" );
        if ( pAig->pSeqModel )
        Fra_SmlWriteCounterExample( pFile, pAig, pAig->pSeqModel );
        }

        if ( fEnableComment )
        {
        printf( "  # File %10s.  ", argv[1] );
        PRT( "Time", clock() - clkTotal );
        }

        if ( pFile != stdout )
            fclose(pFile);
        Aig_ManStop( pAig );
        exit(10);
    } 
    else if ( RetValue == 1 ) 
    {
//    fprintf(stdout, "s UNSATISFIABLE\n");
        fprintf( pFile, "0" );

        if ( fEnableComment )
        {
        printf( "  # File %10s.  ", argv[1] );
        PRT( "Time", clock() - clkTotal );
        }
        printf( "\n" );

        if ( pFile != stdout )
            fclose(pFile);
        Aig_ManStop( pAig );
        exit(20);
    } 
    else // if ( RetValue == -1 ) 
    {
//    fprintf(stdout, "s UNKNOWN\n");
        fprintf( pFile, "2" );

        if ( fEnableComment )
        {
        printf( "  # File %10s.  ", argv[1] );
        PRT( "Time", clock() - clkTotal );
        }
        printf( "\n" );

        if ( pFile != stdout )
            fclose(pFile);
        Aig_ManStop( pAig );
        exit(0);
    }
    return 0;
}




////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

