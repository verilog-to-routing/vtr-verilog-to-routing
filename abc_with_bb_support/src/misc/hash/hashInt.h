/**CFile****************************************************************

  FileName    [hashInt.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Hash maps.]

  Synopsis    [Hash maps.]

  Author      [Aaron P. Hurst]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - May 16, 2006.]

  Revision    [$Id: vecInt.h,v 1.00 2005/06/20 00:00:00 ahurst Exp $]

***********************************************************************/
 
#ifndef ABC__misc__hash__hashInt_h
#define ABC__misc__hash__hashInt_h


////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include "misc/extra/extra.h"

ABC_NAMESPACE_HEADER_START


extern int Hash_DefaultHashFunc(int key, int nBins);

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Hash_Int_t_       Hash_Int_t;
typedef struct Hash_Int_Entry_t_ Hash_Int_Entry_t;

struct Hash_Int_Entry_t_
{
  int                              key;
  int                            data;
  struct Hash_Int_Entry_t_ *       pNext;
};

struct Hash_Int_t_ 
{
  int                             nSize;
  int                             nBins;
  int (* fHash)(int key, int nBins);
  Hash_Int_Entry_t **             pArray;
};



////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

#define Hash_IntForEachEntry( pHash, pEntry, bin)   \
  for(bin=-1, pEntry=NULL; bin < pHash->nBins; (!pEntry)?(pEntry=pHash->pArray[++bin]):(pEntry=pEntry->pNext)) \
    if (pEntry)

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Allocates a hash map with the given number of bins.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Hash_Int_t * Hash_IntAlloc( int nBins )
{
  Hash_Int_t * p;
  int i;
  assert(nBins > 0);
  p = ABC_ALLOC( Hash_Int_t, 1);
  p->nBins = nBins;
  p->fHash = Hash_DefaultHashFunc;
  p->nSize  = 0;
  p->pArray = ABC_ALLOC( Hash_Int_Entry_t *, nBins+1 );
  for(i=0; i<nBins; i++)
    p->pArray[i] = NULL;

  return p;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if a key already exists.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Hash_IntExists( Hash_Int_t *p, int key)
{
  int bin;
  Hash_Int_Entry_t *pEntry, **pLast;

  // find the bin where this key would live
  bin = (*(p->fHash))(key, p->nBins);

  // search for key
  pLast = &(p->pArray[bin]);
  pEntry = p->pArray[bin];
  while(pEntry) {
    if (pEntry->key == key) {
      return 1;
    }
    pLast = &(pEntry->pNext);
    pEntry = pEntry->pNext;
  }

  return 0;
}

/**Function*************************************************************

  Synopsis    [Finds or creates an entry with a key and writes value.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Hash_IntWriteEntry( Hash_Int_t *p, int key, int data )
{
  int bin;
  Hash_Int_Entry_t *pEntry, **pLast;

  // find the bin where this key would live
  bin = (*(p->fHash))(key, p->nBins);

  // search for key
  pLast = &(p->pArray[bin]);
  pEntry = p->pArray[bin];
  while(pEntry) {
    if (pEntry->key == key) {
      pEntry->data = data;
      return;
    }
    pLast = &(pEntry->pNext);
    pEntry = pEntry->pNext;
  }

  // this key does not currently exist
  // create a new entry and add to bin
  p->nSize++;
  (*pLast) = pEntry = ABC_ALLOC( Hash_Int_Entry_t, 1 );
  pEntry->pNext = NULL;
  pEntry->key = key;
  pEntry->data = data;

  return;
}


/**Function*************************************************************

  Synopsis    [Finds or creates an entry with a key.]

  Description [fCreate specifies whether new entries will be created.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Hash_IntEntry( Hash_Int_t *p, int key, int fCreate )
{
  int bin;
  Hash_Int_Entry_t *pEntry, **pLast;

  // find the bin where this key would live
  bin = (*(p->fHash))(key, p->nBins);

  // search for key
  pLast = &(p->pArray[bin]);
  pEntry = p->pArray[bin];
  while(pEntry) {
    if (pEntry->key == key)
      return pEntry->data;
    pLast = &(pEntry->pNext);
    pEntry = pEntry->pNext;
  }

  // this key does not currently exist
  if (fCreate) {
    // create a new entry and add to bin
    p->nSize++;
    (*pLast) = pEntry = ABC_ALLOC( Hash_Int_Entry_t, 1 );
    pEntry->pNext = NULL;
    pEntry->key = key;
    pEntry->data = 0;
    return pEntry->data;
  }

  return 0;
}


/**Function*************************************************************

  Synopsis    [Finds or creates an entry with a key and returns the pointer to it.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int* Hash_IntEntryPtr( Hash_Int_t *p, int key )
{
  int bin;
  Hash_Int_Entry_t *pEntry, **pLast;

  // find the bin where this key would live
  bin = (*(p->fHash))(key, p->nBins);

  // search for key
  pLast = &(p->pArray[bin]);
  pEntry = p->pArray[bin];
  while(pEntry) {
    if (pEntry->key == key)
      return &(pEntry->data);
    pLast = &(pEntry->pNext);
    pEntry = pEntry->pNext;
  }

  // this key does not currently exist
  // create a new entry and add to bin
  p->nSize++;
  (*pLast) = pEntry = ABC_ALLOC( Hash_Int_Entry_t, 1 );
  pEntry->pNext = NULL;
  pEntry->key = key;
  pEntry->data = 0;

  return &(pEntry->data);
}

/**Function*************************************************************

  Synopsis    [Frees the hash.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Hash_IntFree( Hash_Int_t *p ) {
  int bin;
  Hash_Int_Entry_t *pEntry, *pTemp;

  // free bins
  for(bin = 0; bin < p->nBins; bin++) {
    pEntry = p->pArray[bin];
    while(pEntry) {
      pTemp = pEntry;
      pEntry = pEntry->pNext;
      ABC_FREE( pTemp );
    }
  }

  // free hash
  ABC_FREE( p->pArray );
  ABC_FREE( p );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////



ABC_NAMESPACE_HEADER_END

#endif
