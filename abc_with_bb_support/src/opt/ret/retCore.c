/**CFile****************************************************************

  FileName    [retCore.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Retiming package.]

  Synopsis    [The core retiming procedures.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - Oct 31, 2006.]

  Revision    [$Id: retCore.c,v 1.00 2006/10/31 00:00:00 alanmi Exp $]

***********************************************************************/

#include "retInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

int timeRetime = 0;

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Implementation of retiming.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkRetime( Abc_Ntk_t * pNtk, int Mode, int fForwardOnly, int fBackwardOnly, int fVerbose )
{
    int nLatches = Abc_NtkLatchNum(pNtk);
    int nLevels  = Abc_NtkLevel(pNtk);
    int RetValue = 0, clkTotal = clock();
    int nNodesOld, nLatchesOld;
    assert( Mode > 0 && Mode < 7 );
    assert( !fForwardOnly || !fBackwardOnly );

    // cleanup the network
    nNodesOld   = Abc_NtkNodeNum(pNtk);
    nLatchesOld = Abc_NtkLatchNum(pNtk);
    Abc_NtkCleanupSeq(pNtk, 0, 0, 0);
    if ( nNodesOld > Abc_NtkNodeNum(pNtk) || nLatchesOld > Abc_NtkLatchNum(pNtk) )
        printf( "Cleanup before retiming removed %d dangling nodes and %d dangling latches.\n",
            nNodesOld - Abc_NtkNodeNum(pNtk), nLatchesOld - Abc_NtkLatchNum(pNtk) );

    // perform retiming
    switch ( Mode )
    {
    case 1: // forward 
        RetValue = Abc_NtkRetimeIncremental( pNtk, 1, 0, fVerbose );
        break;
    case 2: // backward 
        RetValue = Abc_NtkRetimeIncremental( pNtk, 0, 0, fVerbose );
        break;
    case 3: // min-area 
        RetValue = Abc_NtkRetimeMinArea( pNtk, fForwardOnly, fBackwardOnly, fVerbose );
        break;
    case 4: // min-delay
        if ( !fBackwardOnly )
            RetValue += Abc_NtkRetimeIncremental( pNtk, 1, 1, fVerbose );
        if ( !fForwardOnly )
            RetValue += Abc_NtkRetimeIncremental( pNtk, 0, 1, fVerbose );
        break;
    case 5: // min-area + min-delay
        RetValue  = Abc_NtkRetimeMinArea( pNtk, fForwardOnly, fBackwardOnly, fVerbose );
        if ( !fBackwardOnly )
            RetValue += Abc_NtkRetimeIncremental( pNtk, 1, 1, fVerbose );
        if ( !fForwardOnly )
            RetValue += Abc_NtkRetimeIncremental( pNtk, 0, 1, fVerbose );
        break;
    case 6: // Pan's algorithm
        RetValue = Abc_NtkRetimeLValue( pNtk, 500, fVerbose );
        break;
    default:
        printf( "Unknown retiming option.\n" );
        break;
    }
    if ( fVerbose )
    {
        printf( "Reduction in area = %3d. Reduction in delay = %3d. ", 
            nLatches - Abc_NtkLatchNum(pNtk), nLevels - Abc_NtkLevel(pNtk) );
        PRT( "Total runtime", clock() - clkTotal );
    }
    timeRetime = clock() - clkTotal;
    return RetValue;
}

/**Function*************************************************************

  Synopsis    [Used for automated debugging.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkRetimeDebug( Abc_Ntk_t * pNtk )
{
    extern int Abc_NtkSecFraig( Abc_Ntk_t * pNtk1, Abc_Ntk_t * pNtk2, int nSeconds, int nFrames, int fVerbose );
    Abc_Ntk_t * pNtkRet;
    assert( Abc_NtkIsLogic(pNtk) );
    Abc_NtkToSop( pNtk, 0 );
//    if ( !Abc_NtkCheck( pNtk ) )
//        fprintf( stdout, "Abc_NtkRetimeDebug(): Network check has failed.\n" );
//    Io_WriteBlifLogic( pNtk, "debug_temp.blif", 1 );
    pNtkRet = Abc_NtkDup( pNtk );
    Abc_NtkRetime( pNtkRet, 3, 0, 1, 0 ); // debugging backward flow
    return !Abc_NtkSecFraig( pNtk, pNtkRet, 10000, 3, 0 );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


