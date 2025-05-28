/**CFile****************************************************************

  FileName    [attr.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Network attributes.]

  Synopsis    [External declarations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: attr.h,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/
 
#ifndef ABC__aig__ivy__attr_h
#define ABC__aig__ivy__attr_h


////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include "misc/extra/extra.h"

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////



ABC_NAMESPACE_HEADER_START


////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Attr_ManStruct_t_     Attr_Man_t;
struct Attr_ManStruct_t_
{
    // attribute info
    int                        nAttrSize;     // the size of each attribute in bytes
    Extra_MmFixed_t *          pManMem;       // memory manager for attributes
    int                        nAttrs;        // the number of attributes allocated
    void **                    pAttrs;        // the array of attributes
    int                        fUseInt;       // uses integer attributes
    // attribute specific info
    void *                     pManAttr;      // the manager for this attribute
    void (*pFuncFreeMan) (void *);            // the procedure to call to free attribute-specific manager
    void (*pFuncFreeObj) (void *, void *);    // the procedure to call to free attribute-specific data
};

// at any time, an attribute of the given ID can be
// - not available (p->nAttrs < Id)
// - available but not allocated (p->nAttrs >= Id && p->pAttrs[Id] == NULL)
// - available and allocated (p->nAttrs >= Id && p->pAttrs[Id] != NULL)

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Allocates the attribute manager.]

  Description [The manager is simple if it does not need memory manager.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Attr_Man_t * Attr_ManAlloc( int nAttrSize, int fManMem )
{
    Attr_Man_t * p;
    p = ALLOC( Attr_Man_t, 1 );
    memset( p, 0, sizeof(Attr_Man_t) );
    p->nAttrSize = nAttrSize;
    if ( fManMem )
        p->pManMem = Extra_MmFixedStart( nAttrSize );
    return p;
}

/**Function*************************************************************

  Synopsis    [Start the attribute manager for integers.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Attr_Man_t * Attr_ManStartInt( int nAttrs )
{
    Attr_Man_t * p;
    p = Attr_ManAlloc( sizeof(int), 0 );
    p->nAttrs  = nAttrs;
    p->pAttrs  = (void **)ALLOC( int, nAttrs );
    memset( (int *)p->pAttrs, 0, sizeof(int) * nAttrs );
    p->fUseInt = 1;
    return p;
}

/**Function*************************************************************

  Synopsis    [Start the attribute manager for pointers.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Attr_Man_t * Attr_ManStartPtr( int nAttrs )
{
    Attr_Man_t * p;
    p = Attr_ManAlloc( sizeof(void *), 0 );
    p->nAttrs  = nAttrs;
    p->pAttrs  = ALLOC( void *, nAttrs );
    memset( p->pAttrs, 0, sizeof(void *) * nAttrs );
    return p;
}

/**Function*************************************************************

  Synopsis    [Start the attribute manager for the fixed entry size.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Attr_Man_t * Attr_ManStartPtrMem( int nAttrs, int nAttrSize )
{
    Attr_Man_t * p;
    int i;
    p = Attr_ManAlloc( nAttrSize, 1 );
    p->nAttrs  = nAttrs;
    p->pAttrs  = ALLOC( void *, nAttrs );
    for ( i = 0; i < p->nAttrs; i++ )
    {
        p->pAttrs[i] = Extra_MmFixedEntryFetch( p->pManMem );
        memset( p->pAttrs[i], 0, nAttrSize );
    }
    return p;
}

/**Function*************************************************************

  Synopsis    [Stop the attribute manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Attr_ManStop( Attr_Man_t * p )
{
    // free the attributes of objects
    if ( p->pFuncFreeObj )
    {
        int i;
        if ( p->fUseInt )
        {
            for ( i = 0; i < p->nAttrs; i++ )
                if ( ((int *)p->pAttrs)[i] )
                    p->pFuncFreeObj( p->pManAttr, (void *)((int *)p->pAttrs)[i] );
        }
        else
        {
            for ( i = 0; i < p->nAttrs; i++ )
                if ( p->pAttrs[i] )
                    p->pFuncFreeObj( p->pManAttr, p->pAttrs[i] );
        }
    }
    // free the attribute manager
    if ( p->pManAttr && p->pFuncFreeMan )
        p->pFuncFreeMan( p->pManAttr );
    // free the memory manager
    if ( p->pManMem )  
        Extra_MmFixedStop( p->pManMem);
    // free the attribute manager
    FREE( p->pAttrs );
    free( p );
}

/**Function*************************************************************

  Synopsis    [Reads the attribute of the given object.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Attr_ManReadAttrInt( Attr_Man_t * p, int Id )
{
    assert( p->fUseInt );
    if ( Id >= p->nAttrs )
        return 0;
    return ((int *)p->pAttrs)[Id]; 
}

/**Function*************************************************************

  Synopsis    [Reads the attribute of the given object.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void * Attr_ManReadAttrPtr( Attr_Man_t * p, int Id )
{
    assert( !p->fUseInt );
    if ( Id >= p->nAttrs )
        return NULL;
    return p->pAttrs[Id]; 
}

/**Function*************************************************************

  Synopsis    [Writes the attribute of the given object.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Attr_ManWriteAttrInt( Attr_Man_t * p, int Id, int Attr )
{
    assert( p->fUseInt );
    ((int *)p->pAttrs)[Id] = Attr; 
}

/**Function*************************************************************

  Synopsis    [Writes the attribute of the given object.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Attr_ManWriteAttrPtr( Attr_Man_t * p, int Id, void * pAttr )
{
    assert( !p->fUseInt );
    assert( p->pManMem == NULL );
    p->pAttrs[Id] = pAttr; 
}

/**Function*************************************************************

  Synopsis    [Returns or creates the pointer to the attribute of the given object.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int * Attr_ManFetchSpotInt( Attr_Man_t * p, int Id )
{
    assert( p->fUseInt );
    if ( Id >= p->nAttrs )
    {
        // save the old size
        int i, nAttrsOld = p->nAttrs;
        // get the new size
        p->nAttrs = p->nAttrs? 2*p->nAttrs : 1024;
        p->pAttrs = realloc( p->pAttrs, sizeof(int) * p->nAttrs ); 
        // fill in the empty spots
        for ( i = nAttrsOld; i < p->nAttrs; i++ )
            ((int *)p->pAttrs)[Id] = 0;
    }
    return ((int *)p->pAttrs) + Id;
}

/**Function*************************************************************

  Synopsis    [Returns or creates the pointer to the attribute of the given object.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void ** Attr_ManFetchSpotPtr( Attr_Man_t * p, int Id )
{
    assert( !p->fUseInt );
    if ( Id >= p->nAttrs )
    {
        // save the old size
        int i, nAttrsOld = p->nAttrs;
        // get the new size
        p->nAttrs = p->nAttrs? 2*p->nAttrs : 1024;
        p->pAttrs = realloc( p->pAttrs, sizeof(void *) * p->nAttrs ); 
        // fill in the empty spots
        for ( i = nAttrsOld; i < p->nAttrs; i++ )
            p->pAttrs[Id] = NULL;
    }
    // if memory manager is available but entry is not created, create it
    if ( p->pManMem && p->pAttrs[Id] != NULL ) 
    {
        p->pAttrs[Id] = Extra_MmFixedEntryFetch( p->pManMem );
        memset( p->pAttrs[Id], 0, p->nAttrSize );
    }
    return p->pAttrs + Id;
}


/**Function*************************************************************

  Synopsis    [Returns or creates the attribute of the given object.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Attr_ManFetchAttrInt( Attr_Man_t * p, int Id )
{
    return *Attr_ManFetchSpotInt( p, Id );
}

/**Function*************************************************************

  Synopsis    [Returns or creates the attribute of the given object.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void * Attr_ManFetchAttrPtr( Attr_Man_t * p, int Id )
{
    return *Attr_ManFetchSpotPtr( p, Id );
}

/**Function*************************************************************

  Synopsis    [Sets the attribute of the given object.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Attr_ManSetAttrInt( Attr_Man_t * p, int Id, int Attr )
{
    *Attr_ManFetchSpotInt( p, Id ) = Attr;
}

/**Function*************************************************************

  Synopsis    [Sets the attribute of the given object.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Attr_ManSetAttrPtr( Attr_Man_t * p, int Id, void * pAttr )
{
    assert( p->pManMem == NULL );
    *Attr_ManFetchSpotPtr( p, Id ) = pAttr;
}





ABC_NAMESPACE_HEADER_END



#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

