/**CFile****************************************************************

  FileName    [hopMem.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Minimalistic And-Inverter Graph package.]

  Synopsis    [Memory management for the AIG nodes.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - May 11, 2006.]

  Revision    [$Id: hopMem.c,v 1.00 2006/05/11 00:00:00 alanmi Exp $]

***********************************************************************/

#include "hop.h"

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
void Hop_ManStartMemory( Hop_Man_t * p )
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
void Hop_ManStopMemory( Hop_Man_t * p )
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
void Hop_ManAddMemory( Hop_Man_t * p )
{
    char * pMemory;
    int i, nBytes;
    assert( sizeof(Hop_Obj_t) <= 64 );
    assert( p->pListFree == NULL );
//    assert( (Hop_ManObjNum(p) & IVY_PAGE_MASK) == 0 );
    // allocate new memory page
    nBytes = sizeof(Hop_Obj_t) * (1<<IVY_PAGE_SIZE) + 64;
    pMemory = ABC_ALLOC( char, nBytes );
    Vec_PtrPush( p->vChunks, pMemory );
    // align memory at the 32-byte boundary
    pMemory = pMemory + 64 - (((int)(ABC_PTRUINT_T)pMemory) & 63);
    // remember the manager in the first entry
    Vec_PtrPush( p->vPages, pMemory );
    // break the memory down into nodes
    p->pListFree = (Hop_Obj_t *)pMemory;
    for ( i = 1; i <= IVY_PAGE_MASK; i++ )
    {
        *((char **)pMemory) = pMemory + sizeof(Hop_Obj_t);
        pMemory += sizeof(Hop_Obj_t);
    }
    *((char **)pMemory) = NULL;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

