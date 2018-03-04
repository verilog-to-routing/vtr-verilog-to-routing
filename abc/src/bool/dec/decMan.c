/**CFile****************************************************************

  FileName    [decMan.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Decomposition manager.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: decMan.c,v 1.1 2003/05/22 19:20:05 alanmi Exp $]

***********************************************************************/

#include "base/abc/abc.h"
#include "misc/mvc/mvc.h"
#include "dec.h"

ABC_NAMESPACE_IMPL_START


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
//    abctime clk = Abc_Clock();
    p = ABC_ALLOC( Dec_Man_t, 1 );
    p->pMvcMem = Mvc_ManagerStart();
    p->vCubes = Vec_IntAlloc( 8 );
    p->vLits = Vec_IntAlloc( 8 );
    // canonical forms, phases, perms
    Extra_Truth4VarNPN( &p->puCanons, &p->pPhases, &p->pPerms, &p->pMap );
//ABC_PRT( "NPN classes precomputation time", Abc_Clock() - clk ); 
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
    Mvc_ManagerFree( (Mvc_Manager_t *)p->pMvcMem );
    Vec_IntFree( p->vCubes );
    Vec_IntFree( p->vLits );
    ABC_FREE( p->puCanons );
    ABC_FREE( p->pPhases );
    ABC_FREE( p->pPerms );
    ABC_FREE( p->pMap );
    ABC_FREE( p );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

