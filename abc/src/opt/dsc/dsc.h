/**CFile****************************************************************

  FileName    [dsc.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Disjoint support decomposition - ICCD'15]

  Synopsis    [Disjoint-support decomposition with cofactoring and boolean difference analysis
               from V. Callegaro, F. S. Marranghello, M. G. A. Martins, R. P. Ribas and A. I. Reis,
               "Bottom-up disjoint-support decomposition based on cofactor and boolean difference analysis," ICCD'15]

  Author      [Vinicius Callegaro, Mayler G. A. Martins, Felipe S. Marranghello, Renato P. Ribas and Andre I. Reis]

  Affiliation [UFRGS - Federal University of Rio Grande do Sul - Brazil]

  Date        [Ver. 1.0. Started - October 24, 2014.]

  Revision    [$Id: dsc.h,v 1.00 2014/10/24 00:00:00 vcallegaro Exp $]

***********************************************************************/

#ifndef ABC__DSC___h
#define ABC__DSC___h


////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <string.h>
#include "misc/util/abc_global.h"

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

ABC_NAMESPACE_HEADER_START

#define DSC_MAX_VAR        16 // should be 6 or more, i.e. DSC_MAX_VAR >= 6
#define DSC_MAX_STR        DSC_MAX_VAR << 2 // DSC_MAX_VAR * 4

////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== dsc.c ==========================================================*/

/**
 * memory allocator with a capacity of storing 3*nVars
 * truth-tables for negative and positive cofactors and
 * the boolean difference for each input variable
 */
extern word * Dsc_alloc_pool(int nVars);

/**
 * This method implements the paper proposed by V. Callegaro, F. S. Marranghello, M. G. A. Martins, R. P. Ribas and A. I. Reis,
 * entitled "Bottom-up disjoint-support decomposition based on cofactor and boolean difference analysis", presented at ICCD 2015.
 * pTruth: pointer for the truth table representing the target function.
 * nVarsInit: the number of variables of the truth table of the target function.
 * pRes: pointer for storing the resulting decomposition, whenever a decomposition can be found.
 * pool: NULL or a pointer for with a capacity of storing 3*nVars truth-tables. IF NULL, the function will allocate and free the memory of each call.
 * (the results presented on ICCD paper are running this method with NULL for the memory pool).
 * The method returns 0 if a full decomposition was found and a negative value otherwise.
 */
extern int Dsc_Decompose(word * pTruth, const int nVarsInit, char * const pRes, word *pool);

/**
 * just free the memory pool
 */
extern void Dsc_free_pool(word * pool);

int * Dsc_ComputeMatches( char * p );
int Dsc_CountAnds_rec( char * pStr, char ** p, int * pMatches );
extern int Dsc_CountAnds( char * pDsd );

ABC_NAMESPACE_HEADER_END

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
