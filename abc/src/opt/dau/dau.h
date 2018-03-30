/**CFile****************************************************************

  FileName    [dau.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [DAG-aware unmapping.]

  Synopsis    [External declarations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: dau.h,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/
 
#ifndef ABC__DAU___h
#define ABC__DAU___h


////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include "misc/vec/vec.h"

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

ABC_NAMESPACE_HEADER_START

#define DAU_MAX_VAR    12 // should be 6 or more
#define DAU_MAX_STR  2000
#define DAU_MAX_WORD  (1<<(DAU_MAX_VAR-6))

////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////

// network types
typedef enum { 
    DAU_DSD_NONE = 0,      // 0:  unknown
    DAU_DSD_CONST0,        // 1:  constant
    DAU_DSD_VAR,           // 2:  variable
    DAU_DSD_AND,           // 3:  AND
    DAU_DSD_XOR,           // 4:  XOR
    DAU_DSD_MUX,           // 5:  MUX
    DAU_DSD_PRIME          // 6:  PRIME
} Dau_DsdType_t;

typedef struct Dss_Man_t_ Dss_Man_t;
typedef struct Abc_TtHieMan_t_ Abc_TtHieMan_t;

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

static inline int Dau_DsdIsConst( char * p )  { return (p[0] == '0' || p[0] == '1') && p[1] == 0;    }
static inline int Dau_DsdIsConst0( char * p ) { return  p[0] == '0' && p[1] == 0;                    }
static inline int Dau_DsdIsConst1( char * p ) { return  p[0] == '1' && p[1] == 0;                    }
static inline int Dau_DsdIsVar( char * p )    { if ( *p == '!' ) p++; return *p >= 'a' && *p <= 'z'; }
static inline int Dau_DsdReadVar( char * p )  { if ( *p == '!' ) p++; return *p - 'a';               }

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== dauCanon.c ==========================================================*/
extern unsigned      Abc_TtCanonicize( word * pTruth, int nVars, char * pCanonPerm );
extern unsigned      Abc_TtCanonicizePhase( word * pTruth, int nVars );
/*=== dauDsd.c  ==========================================================*/
extern int *         Dau_DsdComputeMatches( char * p );
extern int           Dau_DsdDecompose( word * pTruth, int nVarsInit, int fSplitPrime, int fWriteTruth, char * pRes );
extern int           Dau_DsdDecomposeLevel( word * pTruth, int nVarsInit, int fSplitPrime, int fWriteTruth, char * pRes, int * pVarLevels );
extern void          Dau_DsdPrintFromTruthFile( FILE * pFile, word * pTruth, int nVarsInit );
extern void          Dau_DsdPrintFromTruth( word * pTruth, int nVarsInit );
extern word *        Dau_DsdToTruth( char * p, int nVars );
extern word          Dau_Dsd6ToTruth( char * p );
extern void          Dau_DsdNormalize( char * p );
extern int           Dau_DsdCountAnds( char * pDsd );
extern void          Dau_DsdTruthCompose_rec( word * pFunc, word pFanins[DAU_MAX_VAR][DAU_MAX_WORD], word * pRes, int nVars, int nWordsR );
extern int           Dau_DsdCheck1Step( void * p, word * pTruth, int nVarsInit, int * pVarLevels );

/*=== dauGia.c  ==========================================================*/
extern int           Dsm_ManTruthToGia( void * p, word * pTruth, Vec_Int_t * vLeaves, Vec_Int_t * vCover );
extern void *        Dsm_ManDeriveGia( void * p, int fUseMuxes );

/*=== dauMerge.c  ==========================================================*/
extern void          Dau_DsdRemoveBraces( char * pDsd, int * pMatches );
extern char *        Dau_DsdMerge( char * pDsd0i, int * pPerm0, char * pDsd1i, int * pPerm1, int fCompl0, int fCompl1, int nVars );

/*=== dauNonDsd.c  ==========================================================*/
extern Vec_Int_t *   Dau_DecFindSets_int( word * pInit, int nVars, int * pSched[16] );
extern Vec_Int_t *   Dau_DecFindSets( word * pInit, int nVars );
extern void          Dau_DecSortSet( unsigned set, int nVars, int * pnUnique, int * pnShared, int * pnFree );
extern void          Dau_DecPrintSets( Vec_Int_t * vSets, int nVars );
extern void          Dau_DecPrintSet( unsigned set, int nVars, int fNewLine );

/*=== dauTree.c  ==========================================================*/
extern Dss_Man_t *   Dss_ManAlloc( int nVars, int nNonDecLimit );
extern void          Dss_ManFree( Dss_Man_t * p );
extern int           Dss_ManMerge( Dss_Man_t * p, int * iDsd, int * nFans, int ** pFans, unsigned uSharedMask, int nKLutSize, unsigned char * pPerm, word * pTruth );
extern void          Dss_ManPrint( char * pFileName, Dss_Man_t * p );


ABC_NAMESPACE_HEADER_END



#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

