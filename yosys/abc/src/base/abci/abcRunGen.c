/**CFile****************************************************************

  FileName    [abcRunGen.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Network and node package.]

  Synopsis    [Interface to experimental procedures.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: abcRunGen.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "base/abc/abc.h"
#include "base/main/main.h"
#include "base/cmd/cmd.h"
#include "misc/util/utilTruth.h"

#ifdef WIN32
#include <process.h> 
#define unlink _unlink
#else
#include <unistd.h>
#endif

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkRunGenOne( Abc_Ntk_t * p, char * pScript )
{    
    Abc_FrameReplaceCurrentNetwork( Abc_FrameGetGlobalFrame(), p );
    if ( Abc_FrameIsBatchMode() )
    {
        if ( Cmd_CommandExecute(Abc_FrameGetGlobalFrame(), pScript) )
        {
            Abc_Print( 1, "Something did not work out with the command \"%s\".\n", pScript );
            return 0;
        }
    }
    else
    {
        Abc_FrameSetBatchMode( 1 );
        if ( Cmd_CommandExecute(Abc_FrameGetGlobalFrame(), pScript) )
        {
            Abc_Print( 1, "Something did not work out with the command \"%s\".\n", pScript );
            return 0;
        }
        Abc_FrameSetBatchMode( 0 );
    }
    Abc_Ntk_t * pTemp = Abc_FrameReadNtk(Abc_FrameGetGlobalFrame());
    return Abc_NtkNodeNum(pTemp);
}
void Acb_NtkRunGen( int nInputs, int nMints, int nFuncs, int Seed, int fVerbose, char * pScript )
{
    abctime clkStart = Abc_Clock();
    Vec_Int_t * vNodes = Vec_IntAlloc( 1000 );
    int i, k, nNodes, nWords = Abc_TtWordNum(nInputs);
    word * pFun = ABC_ALLOC( word, nWords );
    Abc_Ntk_t * pNtkNew; char * pTtStr, * pSop;
    Abc_Random(1);
    for ( i = 0; i < 10+Seed; i++ )
        Abc_Random(0);
    printf( "Synthesizing %d random %d-variable functions with %d positive minterms using script \"%s\".\n", nFuncs, nInputs, nMints, pScript );
    for ( i = 0; i < nFuncs; i++ )
    {
        if ( nMints == 0 )
            for ( k = 0; k < nWords; k++ )
                pFun[k] = Abc_RandomW(0);
        else {
            Abc_TtClear( pFun, nWords );
            for ( k = 0; k < nMints; k++ ) {
                int iMint = 0;
                do iMint = Abc_Random(0) % (1 << nInputs);
                while ( Abc_TtGetBit(pFun, iMint) );
                Abc_TtSetBit( pFun, iMint );
            }
        }
        pTtStr = ABC_CALLOC( char, nInputs > 2 ? (1 << (nInputs-2)) + 1 : 2 );
        Extra_PrintHexadecimalString( pTtStr, (unsigned *)pFun, nInputs );
        pSop = Abc_SopFromTruthHex( pTtStr );
        pNtkNew = Abc_NtkCreateWithNode( pSop );
        nNodes = Abc_NtkRunGenOne( pNtkNew, pScript );
        if ( nNodes >= Vec_IntSize(vNodes) )
          Vec_IntSetEntry( vNodes, nNodes, 0 );
        Vec_IntAddToEntry( vNodes, nNodes, 1 );

        if ( fVerbose ) {
          printf( "Iteration %3d : ", i );
          printf( "Random function has %d positive minterms ", nMints );
          printf( "and maps into %d nodes.\n", nNodes );
          if ( fVerbose )
              printf( "Truth table : %s\n", pTtStr );
        }
        ABC_FREE( pTtStr );
        ABC_FREE( pSop );
    }
    Vec_IntForEachEntry( vNodes, i, k )
      if ( i )
          printf( "Nodes %3d :   Functions %3d   Ratio %5.2f %%\n", k, i, 100.0*i/nFuncs );
    ABC_FREE( pFun );
    Abc_PrintTime( 0, "Total time", Abc_Clock() - clkStart );  
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

