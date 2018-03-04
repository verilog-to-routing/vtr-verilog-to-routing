/**CFile****************************************************************

  FileName    [mvcMan.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Procedures working with the MVC memory manager.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: mvcMan.c,v 1.3 2003/03/19 19:50:26 alanmi Exp $]

***********************************************************************/

#include <string.h>
#include "mvc.h"

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
Mvc_Manager_t * Mvc_ManagerStart()
{
    Mvc_Manager_t * p;
    p = ABC_ALLOC( Mvc_Manager_t, 1 );
    memset( p, 0, sizeof(Mvc_Manager_t) );
    p->pMan1 = Extra_MmFixedStart( sizeof(Mvc_Cube_t)                              );
    p->pMan2 = Extra_MmFixedStart( sizeof(Mvc_Cube_t) +     sizeof(Mvc_CubeWord_t) );
    p->pMan4 = Extra_MmFixedStart( sizeof(Mvc_Cube_t) + 3 * sizeof(Mvc_CubeWord_t) );
    p->pManC = Extra_MmFixedStart( sizeof(Mvc_Cover_t) );
    return p;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mvc_ManagerFree( Mvc_Manager_t * p )
{
    Extra_MmFixedStop( p->pMan1 );
    Extra_MmFixedStop( p->pMan2 );
    Extra_MmFixedStop( p->pMan4 );
    Extra_MmFixedStop( p->pManC );
    ABC_FREE( p );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

