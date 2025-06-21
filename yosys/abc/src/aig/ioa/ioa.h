/**CFile****************************************************************

  FileName    [ioa.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [AIG package.]

  Synopsis    [External declarations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - April 28, 2007.]

  Revision    [$Id: ioa.h,v 1.00 2007/04/28 00:00:00 alanmi Exp $]

***********************************************************************/

#ifndef ABC__aig__ioa__ioa_h
#define ABC__aig__ioa__ioa_h


////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "misc/vec/vec.h"
//#include "bar.h"
#include "aig/aig/aig.h"

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////



ABC_NAMESPACE_HEADER_START
 

////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                             ITERATORS                            ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== ioaReadAig.c ========================================================*/
extern Aig_Man_t *    Ioa_ReadAigerFromMemory( char * pContents, int nFileSize, int fCheck );
extern Aig_Man_t *    Ioa_ReadAiger( char * pFileName, int fCheck );
/*=== ioaWriteAig.c =======================================================*/
extern Vec_Str_t *    Ioa_WriteAigerIntoMemoryStr( Aig_Man_t * pMan );
extern char *         Ioa_WriteAigerIntoMemory( Aig_Man_t * pMan, int * pnSize );
extern void           Ioa_WriteAiger( Aig_Man_t * pMan, char * pFileName, int fWriteSymbols, int fCompact );
/*=== ioaUtil.c =======================================================*/
extern int            Ioa_FileSize( char * pFileName );
extern char *         Ioa_FileNameGeneric( char * FileName );
extern char *         Ioa_FileNameGenericAppend( char * pBase, char * pSuffix );
extern char *         Ioa_TimeStamp();



ABC_NAMESPACE_HEADER_END



#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

