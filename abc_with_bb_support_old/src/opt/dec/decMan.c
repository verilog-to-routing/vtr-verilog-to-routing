/**CFile****************************************************************

  FileName    [decMan.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Decomposition manager.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: decMan.c,v 1.1 2003/05/22 19:20:05 alanmi Exp $]

***********************************************************************/

#include "abc.h"
#include "mvc.h"
#include "dec.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Start the MVC manager used in the factoring package.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Dec_Man_t * Dec_ManStart()
{
    Dec_Man_t * p;
    int clk = clock();
    p = ALLOC( Dec_Man_t, 1 );
    p->pMvcMem = Mvc_ManagerStart();
    p->vCubes = Vec_IntAlloc( 8 );
    p->vLits = Vec_IntAlloc( 8 );
    // canonical forms, phases, perms
    Extra_Truth4VarNPN( &p->puCanons, &p->pPhases, &p->pPerms, &p->pMap );
//PRT( "NPN classes precomputation time", clock() - clk ); 
    return p;
}

/**Function*************************************************************

  Synopsis    [Stops the MVC maanager used in the factoring package.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Dec_ManStop( Dec_Man_t * p )
{
    Mvc_ManagerFree( p->pMvcMem );
    Vec_IntFree( p->vCubes );
    Vec_IntFree( p->vLits );
    free( p->puCanons );
    free( p->pPhases );
    free( p->pPerms );
    free( p->pMap );
    free( p );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


