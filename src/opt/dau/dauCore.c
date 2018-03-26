/**CFile****************************************************************

  FileName    [dauCore.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [DAG-aware unmapping.]

  Synopsis    [Disjoint-support decomposition.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: dauCore.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "dauInt.h"
#include "aig/aig/aig.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

// parameter structure 
typedef struct Xyz_ParTry_t_ Xyz_ParTry_t;
struct Xyz_ParTry_t_
{
    int                Par;
};
 
// operation manager
typedef struct Xyz_ManTry_t_ Xyz_ManTry_t;
struct Xyz_ManTry_t_
{
    Xyz_ParTry_t *     pPar;           // parameters
    Aig_Man_t *        pAig;           // user's AIG 
};

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////
 
/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Xyz_ManTry_t * Xyz_ManTryAlloc( Aig_Man_t * pAig, Xyz_ParTry_t * pPar )
{
    Xyz_ManTry_t * p;
    p = ABC_CALLOC( Xyz_ManTry_t, 1 );
    p->pAig    = pAig;
    p->pPar    = pPar;
    return p;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Xyz_ManTryFree( Xyz_ManTry_t * p )
{
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Xyz_ManPerform( Aig_Man_t * pAig, Xyz_ParTry_t * pPar )
{
    Xyz_ManTry_t * p;
    int RetValue;
    p = Xyz_ManTryAlloc( pAig, pPar );
    RetValue = 1;
    Xyz_ManTryFree( p );
    return RetValue;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

