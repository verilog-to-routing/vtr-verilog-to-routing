/**CFile****************************************************************

  FileName    [covMinMan.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Mapping into network of SOPs/ESOPs.]

  Synopsis    [SOP manipulation.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: covMinMan.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "covInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Starts the minimization manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Min_Man_t * Min_ManAlloc( int nVars )
{
    Min_Man_t * pMan;
    // start the manager
    pMan = ABC_ALLOC( Min_Man_t, 1 );
    memset( pMan, 0, sizeof(Min_Man_t) );
    pMan->nVars   = nVars;
    pMan->nWords  = Abc_BitWordNum( nVars * 2 );
    pMan->pMemMan = Extra_MmFixedStart( sizeof(Min_Cube_t) + sizeof(unsigned) * (pMan->nWords - 1) );
    // allocate storage for the temporary cover
    pMan->ppStore = ABC_ALLOC( Min_Cube_t *, pMan->nVars + 1 );
    // create tautology cubes
    Min_ManClean( pMan, nVars );
    pMan->pOne0  = Min_CubeAlloc( pMan );
    pMan->pOne1  = Min_CubeAlloc( pMan );
    pMan->pTemp  = Min_CubeAlloc( pMan );
    pMan->pBubble = Min_CubeAlloc( pMan );  pMan->pBubble->uData[0] = 0;
    // create trivial cubes
    Min_ManClean( pMan, 1 );
    pMan->pTriv0[0] = Min_CubeAllocVar( pMan, 0, 0 );
    pMan->pTriv0[1] = Min_CubeAllocVar( pMan, 0, 1 );   
    pMan->pTriv1[0] = Min_CubeAllocVar( pMan, 0, 0 );
    pMan->pTriv1[1] = Min_CubeAllocVar( pMan, 0, 1 );   
    Min_ManClean( pMan, nVars );
    return pMan;
}

/**Function*************************************************************

  Synopsis    [Cleans the minimization manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Min_ManClean( Min_Man_t * p, int nSupp )
{
    // set the size of the cube manager
    p->nVars  = nSupp;
    p->nWords = Abc_BitWordNum(2*nSupp);
    // clean the storage
    memset( p->ppStore, 0, sizeof(Min_Cube_t *) * (nSupp + 1) );
    p->nCubes = 0;
}

/**Function*************************************************************

  Synopsis    [Stops the minimization manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Min_ManFree( Min_Man_t * p )
{
    Extra_MmFixedStop( p->pMemMan );
    ABC_FREE( p->ppStore );
    ABC_FREE( p );
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

