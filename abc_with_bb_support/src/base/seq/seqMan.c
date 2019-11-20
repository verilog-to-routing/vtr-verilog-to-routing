/**CFile****************************************************************

  FileName    [seqMan.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Construction and manipulation of sequential AIGs.]

  Synopsis    [Manager of sequential AIG containing.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: seqMan.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "seqInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Allocates sequential AIG manager.]

  Description [The manager contains all the data structures needed to 
  represent sequential AIG and compute stand-alone retiming as well as 
  the integrated mapping/retiming of the sequential AIG.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Seq_t * Seq_Create( Abc_Ntk_t * pNtk )
{
    Abc_Seq_t * p;
    // start the manager
    p = ALLOC( Abc_Seq_t, 1 );
    memset( p, 0, sizeof(Abc_Seq_t) );
    p->pNtk  = pNtk;
    p->nSize = 1000;
    p->nMaxIters = 15;
    p->pMmInits  = Extra_MmFixedStart( sizeof(Seq_Lat_t) );
    p->fEpsilon  = (float)0.001;
    // create internal data structures
    p->vNums     = Vec_IntStart( 2 * p->nSize ); 
    p->vInits    = Vec_PtrStart( 2 * p->nSize ); 
    p->vLValues  = Vec_IntStart( p->nSize ); 
    p->vLags     = Vec_StrStart( p->nSize ); 
    p->vLValuesN = Vec_IntStart( p->nSize ); 
    p->vAFlows   = Vec_IntStart( p->nSize ); 
    p->vLagsN    = Vec_StrStart( p->nSize ); 
    p->vUses     = Vec_StrStart( p->nSize ); 
    return p;
}

/**Function*************************************************************

  Synopsis    [Deallocates sequential AIG manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Seq_Resize( Abc_Seq_t * p, int nMaxId )
{
    if ( p->nSize > nMaxId )
        return;
    p->nSize = nMaxId + 1;
    Vec_IntFill( p->vNums,     2 * p->nSize, 0 ); 
    Vec_PtrFill( p->vInits,    2 * p->nSize, NULL ); 
    Vec_IntFill( p->vLValues,  p->nSize, 0 ); 
    Vec_StrFill( p->vLags,     p->nSize, 0 ); 
    Vec_IntFill( p->vLValuesN, p->nSize, 0 ); 
    Vec_IntFill( p->vAFlows,   p->nSize, 0 ); 
    Vec_StrFill( p->vLagsN,    p->nSize, 0 ); 
    Vec_StrFill( p->vUses,     p->nSize, 0 ); 
}


/**Function*************************************************************

  Synopsis    [Deallocates sequential AIG manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Seq_Delete( Abc_Seq_t * p )
{
    if ( p->fStandCells && p->vMapAnds )
    {
        void * pVoid; int i;
        Vec_PtrForEachEntry( p->vMapAnds, pVoid, i )
            free( pVoid );
    }
    if ( p->vMapDelays )  Vec_VecFree( p->vMapDelays );  // the nodes used in the mapping
    if ( p->vMapFanins )  Vec_VecFree( p->vMapFanins );  // the cuts used in the mapping
    if ( p->vMapAnds )    Vec_PtrFree( p->vMapAnds );    // the nodes used in the mapping
    if ( p->vMapCuts )    Vec_VecFree( p->vMapCuts );    // the cuts used in the mapping
    if ( p->vLValues )    Vec_IntFree( p->vLValues );    // the arrival times (L-Values of nodes)
    if ( p->vLags )       Vec_StrFree( p->vLags );       // the lags of the mapped nodes
    if ( p->vLValuesN )   Vec_IntFree( p->vLValuesN );   // the arrival times (L-Values of nodes)
    if ( p->vAFlows )     Vec_IntFree( p->vAFlows );     // the arrival times (L-Values of nodes)
    if ( p->vLagsN )      Vec_StrFree( p->vLagsN );      // the lags of the mapped nodes
    if ( p->vUses )       Vec_StrFree( p->vUses );       // the uses of phases
    if ( p->vInits )      Vec_PtrFree( p->vInits );      // the initial values of the latches
    if ( p->vNums )       Vec_IntFree( p->vNums );       // the numbers of latches
    Extra_MmFixedStop( p->pMmInits );
    free( p );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


