/**CFile****************************************************************

  FileName    [ivyMem.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [And-Inverter Graph package.]

  Synopsis    [Memory management for the AIG nodes.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - May 11, 2006.]

  Revision    [$Id: ivyMem.c,v 1.00 2006/05/11 00:00:00 alanmi Exp $]

***********************************************************************/

#include "ivy.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

// memory management
#define IVY_PAGE_SIZE      12        // page size containing 2^IVY_PAGE_SIZE nodes
#define IVY_PAGE_MASK    4095        // page bitmask (2^IVY_PAGE_SIZE)-1

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Starts the internal memory manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_ManStartMemory( Ivy_Man_t * p )
{
    p->vChunks = Vec_PtrAlloc( 128 );
    p->vPages = Vec_PtrAlloc( 128 );
}

/**Function*************************************************************

  Synopsis    [Stops the internal memory manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_ManStopMemory( Ivy_Man_t * p )
{
    void * pMemory;
    int i;
    Vec_PtrForEachEntry( void *, p->vChunks, pMemory, i )
        ABC_FREE( pMemory );
    Vec_PtrFree( p->vChunks );
    Vec_PtrFree( p->vPages );
    p->pListFree = NULL;
}

/**Function*************************************************************

  Synopsis    [Allocates additional memory for the nodes.]

  Description [Allocates IVY_PAGE_SIZE nodes. Aligns memory by 32 bytes. 
  Records the pointer to the AIG manager in the -1 entry.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_ManAddMemory( Ivy_Man_t * p )
{
    char * pMemory;
    int i, nBytes;
    int EntrySizeMax = 128;
    assert( sizeof(Ivy_Obj_t) <= EntrySizeMax );
    assert( p->pListFree == NULL );
//    assert( (Ivy_ManObjNum(p) & IVY_PAGE_MASK) == 0 );
    // allocate new memory page
    nBytes = sizeof(Ivy_Obj_t) * (1<<IVY_PAGE_SIZE) + EntrySizeMax;
    pMemory = ABC_ALLOC( char, nBytes );
    Vec_PtrPush( p->vChunks, pMemory );
    // align memory at the 32-byte boundary
    pMemory = pMemory + EntrySizeMax - (((int)(ABC_PTRUINT_T)pMemory) & (EntrySizeMax-1));
    // remember the manager in the first entry
    Vec_PtrPush( p->vPages, pMemory );
    // break the memory down into nodes
    p->pListFree = (Ivy_Obj_t *)pMemory;
    for ( i = 1; i <= IVY_PAGE_MASK; i++ )
    {
        *((char **)pMemory) = pMemory + sizeof(Ivy_Obj_t);
        pMemory += sizeof(Ivy_Obj_t);
    }
    *((char **)pMemory) = NULL;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

