/**CFile****************************************************************

  FileName    [nmInt.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Name manager.]

  Synopsis    [Internal declarations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: nmInt.h,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/
 
#ifndef __NM_INT_H__
#define __NM_INT_H__

#ifdef __cplusplus
extern "C" {
#endif

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include "extra.h"
#include "vec.h"
#include "nm.h"

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Nm_Entry_t_ Nm_Entry_t;
struct Nm_Entry_t_
{
    unsigned         Type   :  4;   // object type
    unsigned         ObjId  : 28;   // object ID
    Nm_Entry_t *     pNextI2N;      // the next entry in the ID hash table
    Nm_Entry_t *     pNextN2I;      // the next entry in the name hash table
    Nm_Entry_t *     pNameSake;     // the next entry with the same name
    char             Name[0];       // name of the object
};

struct Nm_Man_t_
{
    Nm_Entry_t **    pBinsI2N;      // mapping IDs into names
    Nm_Entry_t **    pBinsN2I;      // mapping names into IDs 
    int              nBins;         // the number of bins in tables
    int              nEntries;      // the number of entries
    int              nSizeFactor;   // determined how much larger the table should be
    int              nGrowthFactor; // determined how much the table grows after resizing
    Extra_MmFlex_t * pMem;          // memory manager for entries (and names)
};

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== nmTable.c ==========================================================*/
extern int              Nm_ManTableAdd( Nm_Man_t * p, Nm_Entry_t * pEntry );
extern int              Nm_ManTableDelete( Nm_Man_t * p, int ObjId );
extern Nm_Entry_t *     Nm_ManTableLookupId( Nm_Man_t * p, int ObjId );
extern Nm_Entry_t *     Nm_ManTableLookupName( Nm_Man_t * p, char * pName, int Type );
extern unsigned int     Cudd_PrimeNm( unsigned int p );

#ifdef __cplusplus
}
#endif

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


