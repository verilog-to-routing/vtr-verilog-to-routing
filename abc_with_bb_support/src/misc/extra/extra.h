/**CFile****************************************************************

  FileName    [extra.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [extra]

  Synopsis    [Various reusable software utilities.]

  Description [This library contains a number of operators and 
  traversal routines developed to extend the functionality of 
  CUDD v.2.3.x, by Fabio Somenzi (http://vlsi.colorado.edu/~fabio/)
  To compile your code with the library, #include "extra.h" 
  in your source files and link your project to CUDD and this 
  library. Use the library at your own risk and with caution. 
  Note that debugging of some operators still continues.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: extra.h,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#ifndef __EXTRA_H__
#define __EXTRA_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
#define inline __inline // compatible with MS VS 6.0
#endif

/*---------------------------------------------------------------------------*/
/* Nested includes                                                           */
/*---------------------------------------------------------------------------*/

// this include should be the first one in the list
// it is used to catch memory leaks on Windows
#include "leaks.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include "st.h"
#include "cuddInt.h"

/*---------------------------------------------------------------------------*/
/* Constant declarations                                                     */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Stucture declarations                                                     */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Type declarations                                                         */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Variable declarations                                                     */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Macro declarations                                                        */
/*---------------------------------------------------------------------------*/

typedef unsigned char      uint8;
typedef unsigned short     uint16;
typedef unsigned int       uint32;
#ifdef WIN32
typedef unsigned __int64   uint64;
#else
typedef unsigned long long uint64;
#endif

/* constants of the manager */
#define     b0     Cudd_Not((dd)->one)
#define     b1              (dd)->one
#define     z0              (dd)->zero
#define     z1              (dd)->one
#define     a0              (dd)->zero
#define     a1              (dd)->one

// hash key macros
#define hashKey1(a,TSIZE) \
((unsigned)(a) % TSIZE)

#define hashKey2(a,b,TSIZE) \
(((unsigned)(a) + (unsigned)(b) * DD_P1) % TSIZE)

#define hashKey3(a,b,c,TSIZE) \
(((((unsigned)(a) + (unsigned)(b)) * DD_P1 + (unsigned)(c)) * DD_P2 ) % TSIZE)

#define hashKey4(a,b,c,d,TSIZE) \
((((((unsigned)(a) + (unsigned)(b)) * DD_P1 + (unsigned)(c)) * DD_P2 + \
   (unsigned)(d)) * DD_P3) % TSIZE)

#define hashKey5(a,b,c,d,e,TSIZE) \
(((((((unsigned)(a) + (unsigned)(b)) * DD_P1 + (unsigned)(c)) * DD_P2 + \
   (unsigned)(d)) * DD_P3 + (unsigned)(e)) * DD_P1) % TSIZE)

#ifndef PRT
#define PRT(a,t)  printf("%s = ", (a)); printf("%6.2f sec\n", (float)(t)/(float)(CLOCKS_PER_SEC))
#define PRTn(a,t)  printf("%s = ", (a)); printf("%6.2f sec  ", (float)(t)/(float)(CLOCKS_PER_SEC))
#endif

#ifndef PRTP
#define PRTP(a,t,T)  printf("%s = ", (a)); printf("%6.2f sec (%6.2f %%)\n", (float)(t)/(float)(CLOCKS_PER_SEC), (T)? 100.0*(t)/(T) : 0.0)
#endif

/*===========================================================================*/
/*     Various Utilities                                                     */
/*===========================================================================*/

/*=== extraBddAuto.c ========================================================*/

extern DdNode *     Extra_bddSpaceFromFunctionFast( DdManager * dd, DdNode * bFunc );
extern DdNode *     Extra_bddSpaceFromFunction( DdManager * dd, DdNode * bF, DdNode * bG );
extern DdNode *      extraBddSpaceFromFunction( DdManager * dd, DdNode * bF, DdNode * bG );
extern DdNode *     Extra_bddSpaceFromFunctionPos( DdManager * dd, DdNode * bFunc );
extern DdNode *      extraBddSpaceFromFunctionPos( DdManager * dd, DdNode * bFunc );
extern DdNode *     Extra_bddSpaceFromFunctionNeg( DdManager * dd, DdNode * bFunc );
extern DdNode *      extraBddSpaceFromFunctionNeg( DdManager * dd, DdNode * bFunc );

extern DdNode *     Extra_bddSpaceCanonVars( DdManager * dd, DdNode * bSpace );
extern DdNode *      extraBddSpaceCanonVars( DdManager * dd, DdNode * bSpace );

extern DdNode *     Extra_bddSpaceEquations( DdManager * dd, DdNode * bSpace );
extern DdNode *     Extra_bddSpaceEquationsNeg( DdManager * dd, DdNode * bSpace );
extern DdNode *      extraBddSpaceEquationsNeg( DdManager * dd, DdNode * bSpace );
extern DdNode *     Extra_bddSpaceEquationsPos( DdManager * dd, DdNode * bSpace );
extern DdNode *      extraBddSpaceEquationsPos( DdManager * dd, DdNode * bSpace );

extern DdNode *     Extra_bddSpaceFromMatrixPos( DdManager * dd, DdNode * zA );
extern DdNode *      extraBddSpaceFromMatrixPos( DdManager * dd, DdNode * zA );
extern DdNode *     Extra_bddSpaceFromMatrixNeg( DdManager * dd, DdNode * zA );
extern DdNode *      extraBddSpaceFromMatrixNeg( DdManager * dd, DdNode * zA );

extern DdNode *     Extra_bddSpaceReduce( DdManager * dd, DdNode * bFunc, DdNode * bCanonVars );
extern DdNode **    Extra_bddSpaceExorGates( DdManager * dd, DdNode * bFuncRed, DdNode * zEquations );

/*=== extraBddCas.c =============================================================*/

/* performs the binary encoding of the set of function using the given vars */
extern DdNode *     Extra_bddEncodingBinary( DdManager * dd, DdNode ** pbFuncs, int nFuncs, DdNode ** pbVars, int nVars );
/* solves the column encoding problem using a sophisticated method */
extern DdNode *     Extra_bddEncodingNonStrict( DdManager * dd, DdNode ** pbColumns, int nColumns, DdNode * bVarsCol, DdNode ** pCVars, int nMulti, int * pSimple );
/* collects the nodes under the cut and, for each node, computes the sum of paths leading to it from the root */
extern st_table *   Extra_bddNodePathsUnderCut( DdManager * dd, DdNode * bFunc, int CutLevel );
/* collects the nodes under the cut starting from the given set of ADD nodes */
extern int          Extra_bddNodePathsUnderCutArray( DdManager * dd, DdNode ** paNodes, DdNode ** pbCubes, int nNodes, DdNode ** paNodesRes, DdNode ** pbCubesRes, int CutLevel );
/* find the profile of a DD (the number of edges crossing each level) */
extern int          Extra_ProfileWidth( DdManager * dd, DdNode * F, int * Profile, int CutLevel );

/*=== extraBddMisc.c ========================================================*/

extern DdNode *     Extra_TransferPermute( DdManager * ddSource, DdManager * ddDestination, DdNode * f, int * Permute );
extern DdNode *     Extra_TransferLevelByLevel( DdManager * ddSource, DdManager * ddDestination, DdNode * f );
extern DdNode *     Extra_bddRemapUp( DdManager * dd, DdNode * bF );
extern DdNode *     Extra_bddMove( DdManager * dd, DdNode * bF, int fShiftUp );
extern DdNode *     extraBddMove( DdManager * dd, DdNode * bF, DdNode * bFlag );
extern void         Extra_StopManager( DdManager * dd );
extern void         Extra_bddPrint( DdManager * dd, DdNode * F );
extern void         extraDecomposeCover( DdManager* dd, DdNode*  zC, DdNode** zC0, DdNode** zC1, DdNode** zC2 );
extern int          Extra_bddSuppSize( DdManager * dd, DdNode * bSupp );
extern int          Extra_bddSuppContainVar( DdManager * dd, DdNode * bS, DdNode * bVar );
extern int          Extra_bddSuppOverlapping( DdManager * dd, DdNode * S1, DdNode * S2 );
extern int          Extra_bddSuppDifferentVars( DdManager * dd, DdNode * S1, DdNode * S2, int DiffMax );
extern int          Extra_bddSuppCheckContainment( DdManager * dd, DdNode * bL, DdNode * bH, DdNode ** bLarge, DdNode ** bSmall );
extern int *        Extra_SupportArray( DdManager * dd, DdNode * F, int * support );
extern int *        Extra_VectorSupportArray( DdManager * dd, DdNode ** F, int n, int * support );
extern DdNode *     Extra_bddFindOneCube( DdManager * dd, DdNode * bF );
extern DdNode *     Extra_bddGetOneCube( DdManager * dd, DdNode * bFunc );
extern DdNode *     Extra_bddComputeRangeCube( DdManager * dd, int iStart, int iStop );
extern DdNode *     Extra_bddBitsToCube( DdManager * dd, int Code, int CodeWidth, DdNode ** pbVars, int fMsbFirst );
extern DdNode *     Extra_bddSupportNegativeCube( DdManager * dd, DdNode * f );
extern int          Extra_bddIsVar( DdNode * bFunc );
extern DdNode *     Extra_bddCreateAnd( DdManager * dd, int nVars );
extern DdNode *     Extra_bddCreateOr( DdManager * dd, int nVars );
extern DdNode *     Extra_bddCreateExor( DdManager * dd, int nVars );
extern DdNode *     Extra_zddPrimes( DdManager * dd, DdNode * F );
extern void         Extra_bddPermuteArray( DdManager * dd, DdNode ** bNodesIn, DdNode ** bNodesOut, int nNodes, int *permut );

/*=== extraBddKmap.c ================================================================*/

/* displays the Karnaugh Map of a function */
extern void        Extra_PrintKMap( FILE * pFile, DdManager * dd, DdNode * OnSet, DdNode * OffSet, int nVars, DdNode ** XVars, int fSuppType, char ** pVarNames );
/* displays the Karnaugh Map of a relation */
extern void        Extra_PrintKMapRelation( FILE * pFile, DdManager * dd, DdNode * OnSet, DdNode * OffSet, int nXVars, int nYVars, DdNode ** XVars, DdNode ** YVars );

/*=== extraBddSymm.c =================================================================*/

typedef struct Extra_SymmInfo_t_  Extra_SymmInfo_t;
struct Extra_SymmInfo_t_ {
    int nVars;      // the number of variables in the support
    int nVarsMax;   // the number of variables in the DD manager
    int nSymms;     // the number of pair-wise symmetries
    int nNodes;     // the number of nodes in a ZDD (if applicable)
    int * pVars;    // the list of all variables present in the support
    char ** pSymms; // the symmetry information
};

/* computes the classical symmetry information for the function - recursive */
extern Extra_SymmInfo_t *  Extra_SymmPairsCompute( DdManager * dd, DdNode * bFunc );
/* computes the classical symmetry information for the function - using naive approach */
extern Extra_SymmInfo_t *  Extra_SymmPairsComputeNaive( DdManager * dd, DdNode * bFunc );
extern int         Extra_bddCheckVarsSymmetricNaive( DdManager * dd, DdNode * bF, int iVar1, int iVar2 );

/* allocates the data structure */
extern Extra_SymmInfo_t *  Extra_SymmPairsAllocate( int nVars );
/* deallocates the data structure */
extern void        Extra_SymmPairsDissolve( Extra_SymmInfo_t * );
/* print the contents the data structure */
extern void        Extra_SymmPairsPrint( Extra_SymmInfo_t * );
/* converts the ZDD into the Extra_SymmInfo_t structure */
extern Extra_SymmInfo_t *  Extra_SymmPairsCreateFromZdd( DdManager * dd, DdNode * zPairs, DdNode * bVars );

/* computes the classical symmetry information as a ZDD */
extern DdNode *    Extra_zddSymmPairsCompute( DdManager * dd, DdNode * bF, DdNode * bVars );
extern DdNode *     extraZddSymmPairsCompute( DdManager * dd, DdNode * bF, DdNode * bVars );
/* returns a singleton-set ZDD containing all variables that are symmetric with the given one */
extern DdNode *    Extra_zddGetSymmetricVars( DdManager * dd, DdNode * bF, DdNode * bG, DdNode * bVars );
extern DdNode *     extraZddGetSymmetricVars( DdManager * dd, DdNode * bF, DdNode * bG, DdNode * bVars );
/* converts a set of variables into a set of singleton subsets */
extern DdNode *    Extra_zddGetSingletons( DdManager * dd, DdNode * bVars );
extern DdNode *     extraZddGetSingletons( DdManager * dd, DdNode * bVars );
/* filters the set of variables using the support of the function */
extern DdNode *    Extra_bddReduceVarSet( DdManager * dd, DdNode * bVars, DdNode * bF );
extern DdNode *     extraBddReduceVarSet( DdManager * dd, DdNode * bVars, DdNode * bF );

/* checks the possibility that the two vars are symmetric */
extern int         Extra_bddCheckVarsSymmetric( DdManager * dd, DdNode * bF, int iVar1, int iVar2 );
extern DdNode *     extraBddCheckVarsSymmetric( DdManager * dd, DdNode * bF, DdNode * bVars );

/* build the set of all tuples of K variables out of N from the BDD cube */
extern DdNode *    Extra_zddTuplesFromBdd( DdManager * dd, int K, DdNode * bVarsN );
extern DdNode *     extraZddTuplesFromBdd( DdManager * dd, DdNode * bVarsK, DdNode * bVarsN );
/* selects one subset from a ZDD representing the set of subsets */
extern DdNode *    Extra_zddSelectOneSubset( DdManager * dd, DdNode * zS );
extern DdNode *     extraZddSelectOneSubset( DdManager * dd, DdNode * zS );

/*=== extraBddUnate.c =================================================================*/

typedef struct Extra_UnateVar_t_  Extra_UnateVar_t;
struct Extra_UnateVar_t_ {
    unsigned    iVar : 30;  // index of the variable
    unsigned    Pos  :  1;  // 1 if positive unate
    unsigned    Neg  :  1;  // 1 if negative unate
};

typedef struct Extra_UnateInfo_t_  Extra_UnateInfo_t;
struct Extra_UnateInfo_t_ {
    int nVars;      // the number of variables in the support
    int nVarsMax;   // the number of variables in the DD manager
    int nUnate;     // the number of unate variables
    Extra_UnateVar_t * pVars;    // the array of variables present in the support
};

/* allocates the data structure */
extern Extra_UnateInfo_t *  Extra_UnateInfoAllocate( int nVars );
/* deallocates the data structure */
extern void        Extra_UnateInfoDissolve( Extra_UnateInfo_t * );
/* print the contents the data structure */
extern void        Extra_UnateInfoPrint( Extra_UnateInfo_t * );
/* converts the ZDD into the Extra_SymmInfo_t structure */
extern Extra_UnateInfo_t *  Extra_UnateInfoCreateFromZdd( DdManager * dd, DdNode * zUnate, DdNode * bVars );
/* naive check of unateness of one variable */
extern int         Extra_bddCheckUnateNaive( DdManager * dd, DdNode * bF, int iVar );

/* computes the unateness information for the function */
extern Extra_UnateInfo_t *  Extra_UnateComputeFast( DdManager * dd, DdNode * bFunc );
extern Extra_UnateInfo_t *  Extra_UnateComputeSlow( DdManager * dd, DdNode * bFunc );

/* computes the classical symmetry information as a ZDD */
extern DdNode *    Extra_zddUnateInfoCompute( DdManager * dd, DdNode * bF, DdNode * bVars );
extern DdNode *     extraZddUnateInfoCompute( DdManager * dd, DdNode * bF, DdNode * bVars );

/* converts a set of variables into a set of singleton subsets */
extern DdNode *    Extra_zddGetSingletonsBoth( DdManager * dd, DdNode * bVars );
extern DdNode *     extraZddGetSingletonsBoth( DdManager * dd, DdNode * bVars );

/*=== extraUtilBitMatrix.c ================================================================*/

typedef struct Extra_BitMat_t_ Extra_BitMat_t;
extern Extra_BitMat_t * Extra_BitMatrixStart( int nSize );
extern void         Extra_BitMatrixClean( Extra_BitMat_t * p );
extern void         Extra_BitMatrixStop( Extra_BitMat_t * p );
extern void         Extra_BitMatrixPrint( Extra_BitMat_t * p );
extern int          Extra_BitMatrixReadSize( Extra_BitMat_t * p );
extern void         Extra_BitMatrixInsert1( Extra_BitMat_t * p, int i, int k );
extern int          Extra_BitMatrixLookup1( Extra_BitMat_t * p, int i, int k );
extern void         Extra_BitMatrixDelete1( Extra_BitMat_t * p, int i, int k );
extern void         Extra_BitMatrixInsert2( Extra_BitMat_t * p, int i, int k );
extern int          Extra_BitMatrixLookup2( Extra_BitMat_t * p, int i, int k );
extern void         Extra_BitMatrixDelete2( Extra_BitMat_t * p, int i, int k );
extern void         Extra_BitMatrixOr( Extra_BitMat_t * p, int i, unsigned * pInfo );
extern void         Extra_BitMatrixOrTwo( Extra_BitMat_t * p, int i, int j );
extern int          Extra_BitMatrixCountOnesUpper( Extra_BitMat_t * p );
extern int          Extra_BitMatrixIsDisjoint( Extra_BitMat_t * p1, Extra_BitMat_t * p2 );
extern int          Extra_BitMatrixIsClique( Extra_BitMat_t * p );

/*=== extraUtilFile.c ========================================================*/

extern char *       Extra_FileGetSimilarName( char * pFileNameWrong, char * pS1, char * pS2, char * pS3, char * pS4, char * pS5 );
extern char *       Extra_FileNameExtension( char * FileName );
extern char *       Extra_FileNameAppend( char * pBase, char * pSuffix );
extern char *       Extra_FileNameGeneric( char * FileName );
extern int          Extra_FileSize( char * pFileName );
extern char *       Extra_FileRead( FILE * pFile );
extern char *       Extra_TimeStamp();
extern char *       Extra_StringAppend( char * pStrGiven, char * pStrAdd );
extern unsigned     Extra_ReadBinary( char * Buffer );
extern void         Extra_PrintBinary( FILE * pFile, unsigned Sign[], int nBits );
extern int          Extra_ReadHexadecimal( unsigned Sign[], char * pString, int nVars );
extern void         Extra_PrintHexadecimal( FILE * pFile, unsigned Sign[], int nVars );
extern void         Extra_PrintHexadecimalString( char * pString, unsigned Sign[], int nVars );
extern void         Extra_PrintHex( FILE * pFile, unsigned uTruth, int nVars );
extern void         Extra_PrintSymbols( FILE * pFile, char Char, int nTimes, int fPrintNewLine );

/*=== extraUtilReader.c ========================================================*/

typedef struct Extra_FileReader_t_ Extra_FileReader_t;
extern Extra_FileReader_t * Extra_FileReaderAlloc( char * pFileName, 
    char * pCharsComment, char * pCharsStop, char * pCharsClean );
extern void         Extra_FileReaderFree( Extra_FileReader_t * p );
extern char *       Extra_FileReaderGetFileName( Extra_FileReader_t * p );
extern int          Extra_FileReaderGetFileSize( Extra_FileReader_t * p );
extern int          Extra_FileReaderGetCurPosition( Extra_FileReader_t * p );
extern void *       Extra_FileReaderGetTokens( Extra_FileReader_t * p );
extern int          Extra_FileReaderGetLineNumber( Extra_FileReader_t * p, int iToken );

/*=== extraUtilMemory.c ========================================================*/

typedef struct Extra_MmFixed_t_    Extra_MmFixed_t;    
typedef struct Extra_MmFlex_t_     Extra_MmFlex_t;     
typedef struct Extra_MmStep_t_     Extra_MmStep_t;     

// fixed-size-block memory manager
extern Extra_MmFixed_t *  Extra_MmFixedStart( int nEntrySize );
extern void        Extra_MmFixedStop( Extra_MmFixed_t * p );
extern char *      Extra_MmFixedEntryFetch( Extra_MmFixed_t * p );
extern void        Extra_MmFixedEntryRecycle( Extra_MmFixed_t * p, char * pEntry );
extern void        Extra_MmFixedRestart( Extra_MmFixed_t * p );
extern int         Extra_MmFixedReadMemUsage( Extra_MmFixed_t * p );
extern int         Extra_MmFixedReadMaxEntriesUsed( Extra_MmFixed_t * p );
// flexible-size-block memory manager
extern Extra_MmFlex_t * Extra_MmFlexStart();
extern void        Extra_MmFlexStop( Extra_MmFlex_t * p );
extern void        Extra_MmFlexPrint( Extra_MmFlex_t * p );
extern char *      Extra_MmFlexEntryFetch( Extra_MmFlex_t * p, int nBytes );
extern int         Extra_MmFlexReadMemUsage( Extra_MmFlex_t * p );
// hierarchical memory manager
extern Extra_MmStep_t * Extra_MmStepStart( int nSteps );
extern void        Extra_MmStepStop( Extra_MmStep_t * p );
extern char *      Extra_MmStepEntryFetch( Extra_MmStep_t * p, int nBytes );
extern void        Extra_MmStepEntryRecycle( Extra_MmStep_t * p, char * pEntry, int nBytes );
extern int         Extra_MmStepReadMemUsage( Extra_MmStep_t * p );

/*=== extraUtilMisc.c ========================================================*/

/* finds the smallest integer larger or equal than the logarithm */
extern int         Extra_Base2Log( unsigned Num );
extern int         Extra_Base2LogDouble( double Num );
extern int         Extra_Base10Log( unsigned Num );
/* returns the power of two as a double */
extern double      Extra_Power2( int Num );
extern int         Extra_Power3( int Num );
/* the number of combinations of k elements out of n */
extern int         Extra_NumCombinations( int k, int n  );
extern int *       Extra_DeriveRadixCode( int Number, int Radix, int nDigits );
/* counts the number of 1s in the bitstring */
extern int         Extra_CountOnes( unsigned char * pBytes, int nBytes );
/* the factorial of number */
extern int         Extra_Factorial( int n );
/* the permutation of the given number of elements */
extern char **     Extra_Permutations( int n );
/* permutation and complementation of a truth table */
unsigned           Extra_TruthPermute( unsigned Truth, char * pPerms, int nVars, int fReverse );
unsigned           Extra_TruthPolarize( unsigned uTruth, int Polarity, int nVars );
/* canonical forms of a truth table */
extern unsigned    Extra_TruthCanonN( unsigned uTruth, int nVars );
extern unsigned    Extra_TruthCanonNN( unsigned uTruth, int nVars );
extern unsigned    Extra_TruthCanonP( unsigned uTruth, int nVars );
extern unsigned    Extra_TruthCanonNP( unsigned uTruth, int nVars );
extern unsigned    Extra_TruthCanonNPN( unsigned uTruth, int nVars );
/* canonical forms of 4-variable functions */
extern void        Extra_Truth4VarNPN( unsigned short ** puCanons, char ** puPhases, char ** puPerms, unsigned char ** puMap );
extern void        Extra_Truth4VarN( unsigned short ** puCanons, char *** puPhases, char ** ppCounters, int nPhasesMax );
/* permutation mapping */
extern unsigned short Extra_TruthPerm4One( unsigned uTruth, int Phase );
extern unsigned    Extra_TruthPerm5One( unsigned uTruth, int Phase );
extern void        Extra_TruthPerm6One( unsigned * uTruth, int Phase, unsigned * uTruthRes );
extern void        Extra_TruthExpand( int nVars, int nWords, unsigned * puTruth, unsigned uPhase, unsigned * puTruthR );
/* precomputing tables for permutation mapping */
extern void **     Extra_ArrayAlloc( int nCols, int nRows, int Size );
extern unsigned short ** Extra_TruthPerm43();
extern unsigned ** Extra_TruthPerm53();
extern unsigned ** Extra_TruthPerm54();
/* bubble sort for small number of entries */
extern void        Extra_BubbleSort( int Order[], int Costs[], int nSize, int fIncreasing );
/* for independence from CUDD */
extern unsigned int Cudd_PrimeCopy( unsigned int  p );

/*=== extraUtilCanon.c ========================================================*/

/* fast computation of N-canoninical form up to 6 inputs */
extern int         Extra_TruthCanonFastN( int nVarsMax, int nVarsReal, unsigned * pt, unsigned ** pptRes, char ** ppfRes );

/*=== extraUtilProgress.c ================================================================*/

typedef struct ProgressBarStruct ProgressBar;

extern ProgressBar * Extra_ProgressBarStart( FILE * pFile, int nItemsTotal );
extern void        Extra_ProgressBarStop( ProgressBar * p );
extern void        Extra_ProgressBarUpdate_int( ProgressBar * p, int nItemsCur, char * pString );

static inline void Extra_ProgressBarUpdate( ProgressBar * p, int nItemsCur, char * pString ) 
{  if ( p && nItemsCur < *((int*)p) ) return; Extra_ProgressBarUpdate_int(p, nItemsCur, pString); }

/*=== extraUtilTruth.c ================================================================*/

static inline int   Extra_Float2Int( float Val )     { return *((int *)&Val);               }
static inline float Extra_Int2Float( int Num )       { return *((float *)&Num);             }
static inline int   Extra_BitWordNum( int nBits )    { return nBits/(8*sizeof(unsigned)) + ((nBits%(8*sizeof(unsigned))) > 0);  }
static inline int   Extra_TruthWordNum( int nVars )  { return nVars <= 5 ? 1 : (1 << (nVars - 5)); }

static inline void  Extra_TruthSetBit( unsigned * p, int Bit )   { p[Bit>>5] |= (1<<(Bit & 31));               }
static inline void  Extra_TruthXorBit( unsigned * p, int Bit )   { p[Bit>>5] ^= (1<<(Bit & 31));               }
static inline int   Extra_TruthHasBit( unsigned * p, int Bit )   { return (p[Bit>>5] & (1<<(Bit & 31))) > 0;   }

static inline int Extra_WordCountOnes( unsigned uWord )
{
    uWord = (uWord & 0x55555555) + ((uWord>>1) & 0x55555555);
    uWord = (uWord & 0x33333333) + ((uWord>>2) & 0x33333333);
    uWord = (uWord & 0x0F0F0F0F) + ((uWord>>4) & 0x0F0F0F0F);
    uWord = (uWord & 0x00FF00FF) + ((uWord>>8) & 0x00FF00FF);
    return  (uWord & 0x0000FFFF) + (uWord>>16);
}
static inline int Extra_TruthCountOnes( unsigned * pIn, int nVars )
{
    int w, Counter = 0;
    for ( w = Extra_TruthWordNum(nVars)-1; w >= 0; w-- )
        Counter += Extra_WordCountOnes(pIn[w]);
    return Counter;
}
static inline int Extra_TruthIsEqual( unsigned * pIn0, unsigned * pIn1, int nVars )
{
    int w;
    for ( w = Extra_TruthWordNum(nVars)-1; w >= 0; w-- )
        if ( pIn0[w] != pIn1[w] )
            return 0;
    return 1;
}
static inline int Extra_TruthIsConst0( unsigned * pIn, int nVars )
{
    int w;
    for ( w = Extra_TruthWordNum(nVars)-1; w >= 0; w-- )
        if ( pIn[w] )
            return 0;
    return 1;
}
static inline int Extra_TruthIsConst1( unsigned * pIn, int nVars )
{
    int w;
    for ( w = Extra_TruthWordNum(nVars)-1; w >= 0; w-- )
        if ( pIn[w] != ~(unsigned)0 )
            return 0;
    return 1;
}
static inline int Extra_TruthIsImply( unsigned * pIn1, unsigned * pIn2, int nVars )
{
    int w;
    for ( w = Extra_TruthWordNum(nVars)-1; w >= 0; w-- )
        if ( pIn1[w] & ~pIn2[w] )
            return 0;
    return 1;
}
static inline void Extra_TruthCopy( unsigned * pOut, unsigned * pIn, int nVars )
{
    int w;
    for ( w = Extra_TruthWordNum(nVars)-1; w >= 0; w-- )
        pOut[w] = pIn[w];
}
static inline void Extra_TruthClear( unsigned * pOut, int nVars )
{
    int w;
    for ( w = Extra_TruthWordNum(nVars)-1; w >= 0; w-- )
        pOut[w] = 0;
}
static inline void Extra_TruthFill( unsigned * pOut, int nVars )
{
    int w;
    for ( w = Extra_TruthWordNum(nVars)-1; w >= 0; w-- )
        pOut[w] = ~(unsigned)0;
}
static inline void Extra_TruthNot( unsigned * pOut, unsigned * pIn, int nVars )
{
    int w;
    for ( w = Extra_TruthWordNum(nVars)-1; w >= 0; w-- )
        pOut[w] = ~pIn[w];
}
static inline void Extra_TruthAnd( unsigned * pOut, unsigned * pIn0, unsigned * pIn1, int nVars )
{
    int w;
    for ( w = Extra_TruthWordNum(nVars)-1; w >= 0; w-- )
        pOut[w] = pIn0[w] & pIn1[w];
}
static inline void Extra_TruthOr( unsigned * pOut, unsigned * pIn0, unsigned * pIn1, int nVars )
{
    int w;
    for ( w = Extra_TruthWordNum(nVars)-1; w >= 0; w-- )
        pOut[w] = pIn0[w] | pIn1[w];
}
static inline void Extra_TruthSharp( unsigned * pOut, unsigned * pIn0, unsigned * pIn1, int nVars )
{
    int w;
    for ( w = Extra_TruthWordNum(nVars)-1; w >= 0; w-- )
        pOut[w] = pIn0[w] & ~pIn1[w];
}
static inline void Extra_TruthNand( unsigned * pOut, unsigned * pIn0, unsigned * pIn1, int nVars )
{
    int w;
    for ( w = Extra_TruthWordNum(nVars)-1; w >= 0; w-- )
        pOut[w] = ~(pIn0[w] & pIn1[w]);
}
static inline void Extra_TruthAndPhase( unsigned * pOut, unsigned * pIn0, unsigned * pIn1, int nVars, int fCompl0, int fCompl1 )
{
    int w;
    if ( fCompl0 && fCompl1 )
    {
        for ( w = Extra_TruthWordNum(nVars)-1; w >= 0; w-- )
            pOut[w] = ~(pIn0[w] | pIn1[w]);
    }
    else if ( fCompl0 && !fCompl1 )
    {
        for ( w = Extra_TruthWordNum(nVars)-1; w >= 0; w-- )
            pOut[w] = ~pIn0[w] & pIn1[w];
    }
    else if ( !fCompl0 && fCompl1 )
    {
        for ( w = Extra_TruthWordNum(nVars)-1; w >= 0; w-- )
            pOut[w] = pIn0[w] & ~pIn1[w];
    }
    else // if ( !fCompl0 && !fCompl1 )
    {
        for ( w = Extra_TruthWordNum(nVars)-1; w >= 0; w-- )
            pOut[w] = pIn0[w] & pIn1[w];
    }
}

extern unsigned ** Extra_TruthElementary( int nVars );
extern void        Extra_TruthSwapAdjacentVars( unsigned * pOut, unsigned * pIn, int nVars, int Start );
extern void        Extra_TruthStretch( unsigned * pOut, unsigned * pIn, int nVars, int nVarsAll, unsigned Phase );
extern void        Extra_TruthShrink( unsigned * pOut, unsigned * pIn, int nVars, int nVarsAll, unsigned Phase );
extern int         Extra_TruthVarInSupport( unsigned * pTruth, int nVars, int iVar );
extern int         Extra_TruthSupportSize( unsigned * pTruth, int nVars );
extern int         Extra_TruthSupport( unsigned * pTruth, int nVars );
extern void        Extra_TruthCofactor0( unsigned * pTruth, int nVars, int iVar );
extern void        Extra_TruthCofactor1( unsigned * pTruth, int nVars, int iVar );
extern void        Extra_TruthExist( unsigned * pTruth, int nVars, int iVar );
extern void        Extra_TruthForall( unsigned * pTruth, int nVars, int iVar );
extern void        Extra_TruthMux( unsigned * pOut, unsigned * pCof0, unsigned * pCof1, int nVars, int iVar );
extern void        Extra_TruthChangePhase( unsigned * pTruth, int nVars, int iVar );
extern int         Extra_TruthMinCofSuppOverlap( unsigned * pTruth, int nVars, int * pVarMin );
extern void        Extra_TruthCountOnesInCofs( unsigned * pTruth, int nVars, short * pStore );
extern unsigned    Extra_TruthHash( unsigned * pIn, int nWords );
extern unsigned    Extra_TruthSemiCanonicize( unsigned * pInOut, unsigned * pAux, int nVars, char * pCanonPerm, short * pStore );

/*=== extraUtilUtil.c ================================================================*/

#ifndef ABS
#define ABS(a)			((a) < 0 ? -(a) : (a))
#endif

#ifndef MAX
#define MAX(a,b)		((a) > (b) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a,b)		((a) < (b) ? (a) : (b))
#endif

#ifndef ALLOC
#define ALLOC(type, num)	 ((type *) malloc(sizeof(type) * (num)))
#endif

#ifndef FREE
#define FREE(obj)		     ((obj) ? (free((char *) (obj)), (obj) = 0) : 0)
#endif

#ifndef REALLOC
#define REALLOC(type, obj, num)	\
        ((obj) ? ((type *) realloc((char *)(obj), sizeof(type) * (num))) : \
	     ((type *) malloc(sizeof(type) * (num))))
#endif


extern long        Extra_CpuTime();
extern int         Extra_GetSoftDataLimit();
extern void        Extra_UtilGetoptReset();
extern int         Extra_UtilGetopt( int argc, char *argv[], char *optstring );
extern char *      Extra_UtilPrintTime( long t );
extern char *      Extra_UtilStrsav( char *s );
extern char *      Extra_UtilTildeExpand( char *fname );
extern char *      Extra_UtilFileSearch( char *file, char *path, char *mode );
extern void        (*Extra_UtilMMoutOfMemory)();

extern char *      globalUtilOptarg;
extern int         globalUtilOptind;

/**AutomaticEnd***************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* __EXTRA_H__ */
