/**CFile****************************************************************

  FileName    [fsimCore.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Fast sequential AIG simulator.]

  Synopsis    [Core procedures.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: fsimCore.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "fsimInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [This procedure sets default parameters.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fsim_ManSetDefaultParamsSim( Fsim_ParSim_t * p )
{
    memset( p, 0, sizeof(Fsim_ParSim_t) );
    // user-controlled parameters
    p->nWords       =   8;    // the number of machine words
    p->nIters       =  32;    // the number of timeframes
    p->TimeLimit    =  60;    // time limit in seconds
    p->fCheckMiter  =   0;    // check if miter outputs are non-zero 
    p->fVerbose     =   1;    // enables verbose output
    // internal parameters
    p->fCompressAig =   0;    // compresses internal data
}

/**Function*************************************************************

  Synopsis    [This procedure sets default parameters.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fsim_ManSetDefaultParamsSwitch( Fsim_ParSwitch_t * p )
{
    memset( p, 0, sizeof(Fsim_ParSwitch_t) );
    // user-controlled parameters
    p->nWords       =   1;    // the number of machine words
    p->nIters       =  48;    // the number of timeframes
    p->nPref        =  16;    // the number of first timeframes to skip
    p->nRandPiNum   =   0;    // PI trans prob (0=1/2; 1=1/4; 2=1/8, etc)
    p->fProbOne     =   1;    // collect probability of one
    p->fProbTrans   =   1;    // collect probatility of switching
    p->fVerbose     =   1;    // enables verbose output
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

