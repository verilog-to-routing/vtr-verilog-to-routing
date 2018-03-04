/**CFile****************************************************************

  FileName    [timInt.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Hierarchy/timing manager.]

  Synopsis    [Internal declarations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - April 28, 2007.]

  Revision    [$Id: timInt.h,v 1.00 2007/04/28 00:00:00 alanmi Exp $]

***********************************************************************/

#ifndef ABC__aig__tim__timInt_h
#define ABC__aig__tim__timInt_h


////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "misc/vec/vec.h"
#include "misc/mem/mem.h"
#include "tim.h"

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

ABC_NAMESPACE_HEADER_START

////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Tim_Box_t_           Tim_Box_t;
typedef struct Tim_Obj_t_           Tim_Obj_t;

// timing manager
struct Tim_Man_t_
{
    Vec_Ptr_t *      vBoxes;         // the timing boxes
    Vec_Ptr_t *      vDelayTables;   // pointers to the delay tables
    Mem_Flex_t *     pMemObj;        // memory manager for boxes
    int              nTravIds;       // traversal ID of the manager
    int              fUseTravId;     // enables the use of traversal ID
    int              nCis;           // the number of PIs
    int              nCos;           // the number of POs
    Tim_Obj_t *      pCis;           // timing info for the PIs
    Tim_Obj_t *      pCos;           // timing info for the POs
};

// timing box
struct Tim_Box_t_
{
    int              iBox;           // the unique ID of this box
    int              TravId;         // traversal ID of this box
    int              nInputs;        // the number of box inputs (POs)
    int              nOutputs;       // the number of box outputs (PIs)
    int              iDelayTable;    // index of the delay table
    int              iCopy;          // copy of this box
    int              fBlack;         // this is black box
    int              Inouts[0];      // the int numbers of PIs and POs
};

// timing object
struct Tim_Obj_t_
{
    int              Id;             // the ID of this object
    int              TravId;         // traversal ID of this object
    int              iObj2Box;       // mapping of the object into its box
    int              iObj2Num;       // mapping of the object into its number in the box
    float            timeArr;        // arrival time of the object
    float            timeReq;        // required time of the object
};

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

static inline Tim_Obj_t * Tim_ManCi( Tim_Man_t * p, int i )                           { assert( i < p->nCis ); return p->pCis + i;      }
static inline Tim_Obj_t * Tim_ManCo( Tim_Man_t * p, int i )                           { assert( i < p->nCos ); return p->pCos + i;      }
static inline Tim_Box_t * Tim_ManBox( Tim_Man_t * p, int i )                          { return (Tim_Box_t *)Vec_PtrEntry(p->vBoxes, i); }

static inline Tim_Box_t * Tim_ManCiBox( Tim_Man_t * p, int i )                        { return Tim_ManCi(p,i)->iObj2Box < 0 ? NULL : (Tim_Box_t *)Vec_PtrEntry( p->vBoxes, Tim_ManCi(p,i)->iObj2Box ); }
static inline Tim_Box_t * Tim_ManCoBox( Tim_Man_t * p, int i )                        { return Tim_ManCo(p,i)->iObj2Box < 0 ? NULL : (Tim_Box_t *)Vec_PtrEntry( p->vBoxes, Tim_ManCo(p,i)->iObj2Box ); }

static inline Tim_Obj_t * Tim_ManBoxInput( Tim_Man_t * p, Tim_Box_t * pBox, int i )   { assert( i < pBox->nInputs  ); return p->pCos + pBox->Inouts[i];               }
static inline Tim_Obj_t * Tim_ManBoxOutput( Tim_Man_t * p, Tim_Box_t * pBox, int i )  { assert( i < pBox->nOutputs ); return p->pCis + pBox->Inouts[pBox->nInputs+i]; }

////////////////////////////////////////////////////////////////////////
///                             ITERATORS                            ///
////////////////////////////////////////////////////////////////////////

#define Tim_ManForEachCi( p, pObj, i )                                  \
    for ( i = 0; (i < (p)->nCis) && ((pObj) = (p)->pCis + i); i++ )
#define Tim_ManForEachCo( p, pObj, i )                                  \
    for ( i = 0; (i < (p)->nCos) && ((pObj) = (p)->pCos + i); i++ )

#define Tim_ManForEachPi( p, pObj, i )                                  \
    Tim_ManForEachCi( p, pObj, i ) if ( pObj->iObj2Box >= 0 ) {} else 
#define Tim_ManForEachPo( p, pObj, i )                                  \
    Tim_ManForEachCo( p, pObj, i ) if ( pObj->iObj2Box >= 0 ) {} else 

#define Tim_ManForEachBox( p, pBox, i )                                 \
    Vec_PtrForEachEntry( Tim_Box_t *, p->vBoxes, pBox, i )

#define Tim_ManBoxForEachInput( p, pBox, pObj, i )                      \
    for ( i = 0; (i < (pBox)->nInputs) && ((pObj) = Tim_ManBoxInput(p, pBox, i)); i++ )
#define Tim_ManBoxForEachOutput( p, pBox, pObj, i )                     \
    for ( i = 0; (i < (pBox)->nOutputs) && ((pObj) = Tim_ManBoxOutput(p, pBox, i)); i++ )

#define Tim_ManForEachTable( p, pTable, i )                             \
    Vec_PtrForEachEntry( float *, p->vDelayTables, pTable, i )

////////////////////////////////////////////////////////////////////////
///                     SEQUENTIAL ITERATORS                         ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== time.c ===========================================================*/


ABC_NAMESPACE_HEADER_END



#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

