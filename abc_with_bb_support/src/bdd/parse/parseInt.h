/**CFile****************************************************************

  FileName    [parseInt.h]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Parsing symbolic Boolean formulas into BDDs.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 8, 2003.]

  Revision    [$Id: parseInt.h,v 1.0 2003/09/08 00:00:00 alanmi Exp $]

***********************************************************************/

#ifndef __PARSE_INT_H__
#define __PARSE_INT_H__

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////


#include <stdio.h>
#include "cuddInt.h"
#include "extra.h"
#include "parse.h"

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    STRUCTURE DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

typedef int bool;

typedef struct ParseStackFnStruct    Parse_StackFn_t;    // the function stack
typedef struct ParseStackOpStruct    Parse_StackOp_t;    // the operation stack

////////////////////////////////////////////////////////////////////////
///                       GLOBAL VARIABLES                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                       MACRO DEFINITIONS                          ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== parseStack.c =============================================================*/
extern Parse_StackFn_t *  Parse_StackFnStart  ( int nDepth );
extern bool               Parse_StackFnIsEmpty( Parse_StackFn_t * p );
extern void               Parse_StackFnPush   ( Parse_StackFn_t * p, void * bFunc );
extern void *             Parse_StackFnPop    ( Parse_StackFn_t * p );
extern void               Parse_StackFnFree   ( Parse_StackFn_t * p );

extern Parse_StackOp_t *  Parse_StackOpStart  ( int nDepth );
extern bool               Parse_StackOpIsEmpty( Parse_StackOp_t * p );
extern void               Parse_StackOpPush   ( Parse_StackOp_t * p, int Oper );
extern int                Parse_StackOpPop    ( Parse_StackOp_t * p );
extern void               Parse_StackOpFree   ( Parse_StackOp_t * p );

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
