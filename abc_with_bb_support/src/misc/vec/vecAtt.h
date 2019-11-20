/**CFile****************************************************************

  FileName    [vecAtt.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Resizable arrays.]

  Synopsis    [Array of user-specified attiributes.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: vecAtt.h,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/
 
#ifndef __Vec_Att_H__
#define __Vec_Att_H__

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include <stdio.h>

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

// various attributes
typedef enum { 
    VEC_ATTR_NONE = 0,     // 0
    VEC_ATTR_COPY,         // 1
    VEC_ATTR_LOCAL_AIG,    // 2
    VEC_ATTR_LOCAL_SOP,    // 3
    VEC_ATTR_LOCAL_BDD,    // 4
    VEC_ATTR_GLOBAL_AIG,   // 5
    VEC_ATTR_GLOBAL_SOP,   // 6
    VEC_ATTR_GLOBAL_BDD,   // 7
    VEC_ATTR_LEVEL,        // 8
    VEC_ATTR_LEVEL_REV,    // 9
    VEC_ATTR_RETIME_LAG,   // 10
    VEC_ATTR_FRAIG,        // 11
    VEC_ATTR_MVVAR,        // 12
    VEC_ATTR_DATA1,        // 13
    VEC_ATTR_DATA2,        // 14
    VEC_ATTR_TOTAL_NUM     // 15
} Vec_AttrType_t;

////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Vec_Att_t_  Vec_Att_t;
struct Vec_Att_t_ 
{
    // storage for attributes
    int              nCap;                 // the size of array allocated
    int *            pArrayInt;            // the integer attribute array
    void **          pArrayPtr;            // the pointer attribute array
    // attribute specific info
    void *           pMan;                 // the manager for this attribute
    void (*pFuncFreeMan) (void *);         // the procedure to free the manager
    void*(*pFuncStartObj)(void *);         // the procedure to start one attribute
    void (*pFuncFreeObj) (void *, void *); // the procedure to free one attribute
};

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Allocates a vector with the given capacity.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Vec_Att_t * Vec_AttAlloc( 
    int fInteger, int nSize, void * pMan, 
    void (*pFuncFreeMan) (void *), 
    void*(*pFuncStartObj)(void *), 
    void (*pFuncFreeObj) (void *, void *)  )
{
    Vec_Att_t * p;
    p = ALLOC( Vec_Att_t, 1 );
    memset( p, 0, sizeof(Vec_Att_t) );
    p->pMan          = pMan;
    p->pFuncFreeMan  = pFuncFreeMan;
    p->pFuncStartObj = pFuncStartObj;
    p->pFuncFreeObj  = pFuncFreeObj;
    p->nCap = nSize? nSize : 16;
    if ( fInteger )
    {
        p->pArrayInt = ALLOC( int, p->nCap );
        memset( p->pArrayInt, 0xff, sizeof(int) * p->nCap );
    }
    else
    {
        p->pArrayPtr = ALLOC( void *, p->nCap );
        memset( p->pArrayPtr, 0, sizeof(void *) * p->nCap );
    }
    return p;
}

/**Function*************************************************************

  Synopsis    [Frees the vector.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void * Vec_AttFree( Vec_Att_t * p, int fFreeMan )
{
    void * pMan;
    if ( p == NULL )
        return NULL;
    // free the attributes of objects
    if ( p->pFuncFreeObj )
    {
        int i;
        if ( p->pArrayInt )
        {
            for ( i = 0; i < p->nCap; i++ )
                if ( p->pArrayInt[i] )
                    p->pFuncFreeObj( p->pMan, (void *)p->pArrayInt[i] );
        }
        else
        {
            for ( i = 0; i < p->nCap; i++ )
                if ( p->pArrayPtr[i] )
                    p->pFuncFreeObj( p->pMan, p->pArrayPtr[i] );
        }
    }
    // free the memory manager
    pMan = fFreeMan? NULL : p->pMan;
    if ( p->pMan && fFreeMan )  
        p->pFuncFreeMan( p->pMan );
    FREE( p->pArrayInt );
    FREE( p->pArrayPtr );
    FREE( p );
    return pMan;
}

/**Function*************************************************************

  Synopsis    [Clears the vector.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_AttClear( Vec_Att_t * p )
{
    // free the attributes of objects
    if ( p->pFuncFreeObj )
    {
        int i;
        if ( p->pArrayInt )
        {
            if ( p->pFuncFreeObj )
                for ( i = 0; i < p->nCap; i++ )
                    if ( p->pArrayInt[i] )
                        p->pFuncFreeObj( p->pMan, (void *)p->pArrayInt[i] );
        }
        else
        {
            if ( p->pFuncFreeObj )
                for ( i = 0; i < p->nCap; i++ )
                    if ( p->pArrayPtr[i] )
                        p->pFuncFreeObj( p->pMan, p->pArrayPtr[i] );
        }
    }
    if ( p->pArrayInt )
        memset( p->pArrayInt, 0xff, sizeof(int) * p->nCap );
    else
        memset( p->pArrayPtr, 0, sizeof(void *) * p->nCap );

}

/**Function*************************************************************

  Synopsis    [Deletes one entry from the attribute manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_AttFreeEntry( Vec_Att_t * p, int i )
{
    if ( i >= p->nCap )
        return;
    if ( p->pMan )
    {
        if ( p->pArrayInt[i] && p->pFuncFreeObj )
            p->pFuncFreeObj( p->pMan, (void *)p->pArrayInt[i] );
        if ( p->pArrayPtr[i] && p->pFuncFreeObj )
            p->pFuncFreeObj( p->pMan, (void *)p->pArrayPtr[i] );
    }
    if ( p->pArrayInt )
        p->pArrayInt[i] = ~(unsigned)0;
    else
        p->pArrayPtr[i] = NULL;
}

/**Function*************************************************************

  Synopsis    [Resizes the vector to the given capacity.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_AttGrow( Vec_Att_t * p, int nCapMin )
{
    if ( p->nCap >= nCapMin )
        return;
    if ( p->pArrayInt )
    {
        p->pArrayInt = REALLOC( int, p->pArrayInt, nCapMin ); 
        memset( p->pArrayInt + p->nCap, 0xff, sizeof(int) * (nCapMin - p->nCap) );
    }
    else
    {
        p->pArrayPtr = REALLOC( void *, p->pArrayPtr, nCapMin ); 
        memset( p->pArrayPtr + p->nCap, 0, sizeof(void *) * (nCapMin - p->nCap) );
    }
    p->nCap = nCapMin;
}

/**Function*************************************************************

  Synopsis    [Writes the entry into its place.]

  Description [Only works if the manager is not defined.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_AttWriteEntry( Vec_Att_t * p, int i, void * pEntry )
{
    assert( p->pArrayPtr );
    assert( p->pFuncStartObj == NULL );
    if ( i >= p->nCap )
        Vec_AttGrow( p, (2 * p->nCap > i)? 2 * p->nCap : i + 10 );
    p->pArrayPtr[i] = pEntry;
}

/**Function*************************************************************

  Synopsis    [Writes the entry into its place.]

  Description [Only works if the manager is not defined.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_AttWriteEntryInt( Vec_Att_t * p, int i, int Entry )
{
    assert( p->pArrayInt );
    assert( p->pFuncStartObj == NULL );
    if ( i >= p->nCap )
        Vec_AttGrow( p, (2 * p->nCap > i)? 2 * p->nCap : i + 10 );
    p->pArrayInt[i] = Entry;
}

/**Function*************************************************************

  Synopsis    [Returns the entry.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void * Vec_AttEntry( Vec_Att_t * p, int i )
{
    assert( p->pArrayPtr );
    if ( i >= p->nCap )
        Vec_AttGrow( p, (2 * p->nCap > i)? 2 * p->nCap : i + 10 );
    if ( p->pArrayPtr[i] == NULL && p->pFuncStartObj )
        p->pArrayPtr[i] = p->pFuncStartObj( p->pMan );
    return p->pArrayPtr[i];
}

/**Function*************************************************************

  Synopsis    [Returns the entry.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Vec_AttEntryInt( Vec_Att_t * p, int i )
{
    assert( p->pArrayInt );
    assert( p->pMan == NULL );
    if ( i >= p->nCap )
        Vec_AttGrow( p, (2 * p->nCap > i)? 2 * p->nCap : i + 10 );
    return p->pArrayInt[i];
}

/**Function*************************************************************

  Synopsis    [Returns the entry.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void * Vec_AttMan( Vec_Att_t * p )
{
    return p->pMan;
}

/**Function*************************************************************

  Synopsis    [Returns the array of attributes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void ** Vec_AttArray( Vec_Att_t * p )
{
    return p->pArrayPtr;
}

/**Function*************************************************************

  Synopsis    [Returns the array of attributes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int * Vec_AttArrayInt( Vec_Att_t * p )
{
    return p->pArrayInt;
}

#endif


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

