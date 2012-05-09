/**CFile****************************************************************

  FileName    [fraSec.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [New FRAIG package.]

  Synopsis    [Performs SEC based on seq sweeping.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 30, 2007.]

  Revision    [$Id: fraSec.c,v 1.00 2007/06/30 00:00:00 alanmi Exp $]

***********************************************************************/

#include "fra.h"

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
int Fra_FraigSec( Aig_Man_t * p, int nFramesFix, int fVerbose, int fVeryVerbose )
{
    Aig_Man_t * pNew;
    int nFrames, RetValue, nIter, clk, clkTotal = clock();
    if ( nFramesFix )
    {
        nFrames = nFramesFix;
        // perform seq sweeping for one frame number
        pNew = Fra_FraigInduction( p, nFrames, 0, fVeryVerbose, &nIter );
    }
    else
    {
        // perform seq sweeping while increasing the number of frames
        for ( nFrames = 1; ; nFrames++ )
        {
clk = clock();
            pNew = Fra_FraigInduction( p, nFrames, 0, fVeryVerbose, &nIter );
            RetValue = Fra_FraigMiterStatus( pNew );
            if ( fVerbose )
            {
                printf( "FRAMES %3d : Iters = %3d. ", nFrames, nIter );
                if ( RetValue == 1 )
                    printf( "UNSAT     " );
                else
                    printf( "UNDECIDED " );
PRT( "Time", clock() - clk );
            }
            if ( RetValue != -1 )
                break;
            Aig_ManStop( pNew );
        }
    }

    // get the miter status
    RetValue = Fra_FraigMiterStatus( pNew );
    Aig_ManStop( pNew );

    // report the miter
    if ( RetValue == 1 )
        printf( "Networks are equivalent after seq sweeping with K=%d frames (%d iters). ", nFrames, nIter );
    else if ( RetValue == 0 )
        printf( "Networks are NOT EQUIVALENT. " );
    else
        printf( "Networks are UNDECIDED after seq sweeping with K=%d frames (%d iters). ", nFrames, nIter );
PRT( "Time", clock() - clkTotal );
    return RetValue;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


