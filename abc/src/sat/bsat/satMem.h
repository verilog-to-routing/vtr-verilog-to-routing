/**CFile****************************************************************

  FileName    [satMem.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [SAT solver.]

  Synopsis    [Memory management.]

  Author      [Alan Mishchenko <alanmi@eecs.berkeley.edu>]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - January 1, 2004.]

  Revision    [$Id: satMem.h,v 1.0 2004/01/01 1:00:00 alanmi Exp $]

***********************************************************************/
 
#ifndef ABC__sat__bsat__satMem_h
#define ABC__sat__bsat__satMem_h

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include "misc/util/abc_global.h"

ABC_NAMESPACE_HEADER_START

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    STRUCTURE DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

typedef struct Sat_MmFixed_t_ Sat_MmFixed_t;
typedef struct Sat_MmFlex_t_  Sat_MmFlex_t;
typedef struct Sat_MmStep_t_  Sat_MmStep_t;

////////////////////////////////////////////////////////////////////////
///                       GLOBAL VARIABLES                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                       MACRO DEFINITIONS                          ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DECLARATIONS                        ///
////////////////////////////////////////////////////////////////////////

// fixed-size-block memory manager
extern Sat_MmFixed_t *     Sat_MmFixedStart( int nEntrySize );
extern void                Sat_MmFixedStop( Sat_MmFixed_t * p, int fVerbose );
extern char *              Sat_MmFixedEntryFetch( Sat_MmFixed_t * p );
extern void                Sat_MmFixedEntryRecycle( Sat_MmFixed_t * p, char * pEntry );
extern void                Sat_MmFixedRestart( Sat_MmFixed_t * p );
extern int                 Sat_MmFixedReadMemUsage( Sat_MmFixed_t * p );
// flexible-size-block memory manager
extern Sat_MmFlex_t *      Sat_MmFlexStart();
extern void                Sat_MmFlexStop( Sat_MmFlex_t * p, int fVerbose );
extern char *              Sat_MmFlexEntryFetch( Sat_MmFlex_t * p, int nBytes );
extern int                 Sat_MmFlexReadMemUsage( Sat_MmFlex_t * p );
// hierarchical memory manager
extern Sat_MmStep_t *      Sat_MmStepStart( int nSteps );
extern void                Sat_MmStepStop( Sat_MmStep_t * p, int fVerbose );
extern void                Sat_MmStepRestart( Sat_MmStep_t * p );
extern char *              Sat_MmStepEntryFetch( Sat_MmStep_t * p, int nBytes );
extern void                Sat_MmStepEntryRecycle( Sat_MmStep_t * p, char * pEntry, int nBytes );
extern int                 Sat_MmStepReadMemUsage( Sat_MmStep_t * p );



ABC_NAMESPACE_HEADER_END

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

