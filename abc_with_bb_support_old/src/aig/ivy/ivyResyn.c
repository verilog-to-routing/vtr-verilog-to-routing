/**CFile****************************************************************

  FileName    [ivyResyn.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [And-Inverter Graph package.]

  Synopsis    [AIG rewriting script.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - May 11, 2006.]

  Revision    [$Id: ivyResyn.c,v 1.00 2006/05/11 00:00:00 alanmi Exp $]

***********************************************************************/

#include "ivy.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Performs several passes of rewriting on the AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ivy_Man_t * Ivy_ManResyn0( Ivy_Man_t * pMan, int fUpdateLevel, int fVerbose )
{
    int clk;
    Ivy_Man_t * pTemp;

if ( fVerbose ) { printf( "Original:\n" ); }
if ( fVerbose ) Ivy_ManPrintStats( pMan );

clk = clock();
    pMan = Ivy_ManBalance( pMan, fUpdateLevel );
if ( fVerbose ) { printf( "\n" ); }
if ( fVerbose ) { PRT( "Balance", clock() - clk ); }
if ( fVerbose ) Ivy_ManPrintStats( pMan );

//    Ivy_ManRewriteAlg( pMan, fUpdateLevel, 0 );
clk = clock();
    Ivy_ManRewritePre( pMan, fUpdateLevel, 0, 0 );
if ( fVerbose ) { printf( "\n" ); }
if ( fVerbose ) { PRT( "Rewrite", clock() - clk ); }
if ( fVerbose ) Ivy_ManPrintStats( pMan );

clk = clock();
    pMan = Ivy_ManBalance( pTemp = pMan, fUpdateLevel );
    Ivy_ManStop( pTemp );
if ( fVerbose ) { printf( "\n" ); }
if ( fVerbose ) { PRT( "Balance", clock() - clk ); }
if ( fVerbose ) Ivy_ManPrintStats( pMan );
    return pMan;
}

/**Function*************************************************************

  Synopsis    [Performs several passes of rewriting on the AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ivy_Man_t * Ivy_ManResyn( Ivy_Man_t * pMan, int fUpdateLevel, int fVerbose )
{
    int clk;
    Ivy_Man_t * pTemp;

if ( fVerbose ) { printf( "Original:\n" ); }
if ( fVerbose ) Ivy_ManPrintStats( pMan );

clk = clock();
    pMan = Ivy_ManBalance( pMan, fUpdateLevel );
if ( fVerbose ) { printf( "\n" ); }
if ( fVerbose ) { PRT( "Balance", clock() - clk ); }
if ( fVerbose ) Ivy_ManPrintStats( pMan );

//    Ivy_ManRewriteAlg( pMan, fUpdateLevel, 0 );
clk = clock();
    Ivy_ManRewritePre( pMan, fUpdateLevel, 0, 0 );
if ( fVerbose ) { printf( "\n" ); }
if ( fVerbose ) { PRT( "Rewrite", clock() - clk ); }
if ( fVerbose ) Ivy_ManPrintStats( pMan );

clk = clock();
    pMan = Ivy_ManBalance( pTemp = pMan, fUpdateLevel );
    Ivy_ManStop( pTemp );
if ( fVerbose ) { printf( "\n" ); }
if ( fVerbose ) { PRT( "Balance", clock() - clk ); }
if ( fVerbose ) Ivy_ManPrintStats( pMan );

//    Ivy_ManRewriteAlg( pMan, fUpdateLevel, 1 );
clk = clock();
    Ivy_ManRewritePre( pMan, fUpdateLevel, 1, 0 );
if ( fVerbose ) { printf( "\n" ); }
if ( fVerbose ) { PRT( "Rewrite", clock() - clk ); }
if ( fVerbose ) Ivy_ManPrintStats( pMan );

clk = clock();
    pMan = Ivy_ManBalance( pTemp = pMan, fUpdateLevel );
    Ivy_ManStop( pTemp );
if ( fVerbose ) { printf( "\n" ); }
if ( fVerbose ) { PRT( "Balance", clock() - clk ); }
if ( fVerbose ) Ivy_ManPrintStats( pMan );

//    Ivy_ManRewriteAlg( pMan, fUpdateLevel, 1 );
clk = clock();
    Ivy_ManRewritePre( pMan, fUpdateLevel, 1, 0 );
if ( fVerbose ) { printf( "\n" ); }
if ( fVerbose ) { PRT( "Rewrite", clock() - clk ); }
if ( fVerbose ) Ivy_ManPrintStats( pMan );

clk = clock();
    pMan = Ivy_ManBalance( pTemp = pMan, fUpdateLevel );
    Ivy_ManStop( pTemp );
if ( fVerbose ) { printf( "\n" ); }
if ( fVerbose ) { PRT( "Balance", clock() - clk ); }
if ( fVerbose ) Ivy_ManPrintStats( pMan );
    return pMan;
}

/**Function*************************************************************

  Synopsis    [Performs several passes of rewriting on the AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ivy_Man_t * Ivy_ManRwsat( Ivy_Man_t * pMan, int fVerbose )
{
    int clk;
    Ivy_Man_t * pTemp;

if ( fVerbose ) { printf( "Original:\n" ); }
if ( fVerbose ) Ivy_ManPrintStats( pMan );

clk = clock();
    Ivy_ManRewritePre( pMan, 0, 0, 0 );
if ( fVerbose ) { printf( "\n" ); }
if ( fVerbose ) { PRT( "Rewrite", clock() - clk ); }
if ( fVerbose ) Ivy_ManPrintStats( pMan );

clk = clock();
    pMan = Ivy_ManBalance( pTemp = pMan, 0 );
//    pMan = Ivy_ManDup( pTemp = pMan );
    Ivy_ManStop( pTemp );
if ( fVerbose ) { printf( "\n" ); }
if ( fVerbose ) { PRT( "Balance", clock() - clk ); }
if ( fVerbose ) Ivy_ManPrintStats( pMan );

/*
clk = clock();
    Ivy_ManRewritePre( pMan, 0, 0, 0 );
if ( fVerbose ) { printf( "\n" ); }
if ( fVerbose ) { PRT( "Rewrite", clock() - clk ); }
if ( fVerbose ) Ivy_ManPrintStats( pMan );

clk = clock();
    pMan = Ivy_ManBalance( pTemp = pMan, 0 );
    Ivy_ManStop( pTemp );
if ( fVerbose ) { printf( "\n" ); }
if ( fVerbose ) { PRT( "Balance", clock() - clk ); }
if ( fVerbose ) Ivy_ManPrintStats( pMan );
*/
    return pMan;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


