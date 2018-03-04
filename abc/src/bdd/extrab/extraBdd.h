/**CFile****************************************************************

  FileName    [extraBdd.h]

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

  Revision    [$Id: extraBdd.h,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#ifndef ABC__misc__extra__extra_bdd_h
#define ABC__misc__extra__extra_bdd_h


#ifdef _WIN32
#define inline __inline // compatible with MS VS 6.0
#endif

/*---------------------------------------------------------------------------*/
/* Nested includes                                                           */
/*---------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "misc/st/st.h"
#include "bdd/cudd/cuddInt.h"
#include "misc/extra/extra.h"


ABC_NAMESPACE_HEADER_START


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

/* constants of the manager */
#define     b0     Cudd_Not((dd)->one)
#define     b1              (dd)->one
#define     z0              (dd)->zero
#define     z1              (dd)->one
#define     a0              (dd)->zero
#define     a1              (dd)->one

// hash key macros
#define hashKey1(a,TSIZE) \
((ABC_PTRUINT_T)(a) % TSIZE)

#define hashKey2(a,b,TSIZE) \
(((ABC_PTRUINT_T)(a) + (ABC_PTRUINT_T)(b) * DD_P1) % TSIZE)

#define hashKey3(a,b,c,TSIZE) \
(((((ABC_PTRUINT_T)(a) + (ABC_PTRUINT_T)(b)) * DD_P1 + (ABC_PTRUINT_T)(c)) * DD_P2 ) % TSIZE)

#define hashKey4(a,b,c,d,TSIZE) \
((((((ABC_PTRUINT_T)(a) + (ABC_PTRUINT_T)(b)) * DD_P1 + (ABC_PTRUINT_T)(c)) * DD_P2 + \
   (ABC_PTRUINT_T)(d)) * DD_P3) % TSIZE)

#define hashKey5(a,b,c,d,e,TSIZE) \
(((((((ABC_PTRUINT_T)(a) + (ABC_PTRUINT_T)(b)) * DD_P1 + (ABC_PTRUINT_T)(c)) * DD_P2 + \
   (ABC_PTRUINT_T)(d)) * DD_P3 + (ABC_PTRUINT_T)(e)) * DD_P1) % TSIZE)

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
extern st__table *   Extra_bddNodePathsUnderCut( DdManager * dd, DdNode * bFunc, int CutLevel );
/* collects the nodes under the cut starting from the given set of ADD nodes */
extern int          Extra_bddNodePathsUnderCutArray( DdManager * dd, DdNode ** paNodes, DdNode ** pbCubes, int nNodes, DdNode ** paNodesRes, DdNode ** pbCubesRes, int CutLevel );
/* find the profile of a DD (the number of edges crossing each level) */
extern int          Extra_ProfileWidth( DdManager * dd, DdNode * F, int * Profile, int CutLevel );

/*=== extraBddImage.c ================================================================*/

typedef struct Extra_ImageTree_t_  Extra_ImageTree_t;
extern Extra_ImageTree_t * Extra_bddImageStart( 
    DdManager * dd, DdNode * bCare,
    int nParts, DdNode ** pbParts,
    int nVars, DdNode ** pbVars, int fVerbose );
extern DdNode *    Extra_bddImageCompute( Extra_ImageTree_t * pTree, DdNode * bCare );
extern void        Extra_bddImageTreeDelete( Extra_ImageTree_t * pTree );
extern DdNode *    Extra_bddImageRead( Extra_ImageTree_t * pTree );

typedef struct Extra_ImageTree2_t_  Extra_ImageTree2_t;
extern Extra_ImageTree2_t * Extra_bddImageStart2( 
    DdManager * dd, DdNode * bCare,
    int nParts, DdNode ** pbParts,
    int nVars, DdNode ** pbVars, int fVerbose );
extern DdNode *    Extra_bddImageCompute2( Extra_ImageTree2_t * pTree, DdNode * bCare );
extern void        Extra_bddImageTreeDelete2( Extra_ImageTree2_t * pTree );
extern DdNode *    Extra_bddImageRead2( Extra_ImageTree2_t * pTree );

/*=== extraBddMisc.c ========================================================*/

extern DdNode *     Extra_TransferPermute( DdManager * ddSource, DdManager * ddDestination, DdNode * f, int * Permute );
extern DdNode *     Extra_TransferLevelByLevel( DdManager * ddSource, DdManager * ddDestination, DdNode * f );
extern DdNode *     Extra_bddRemapUp( DdManager * dd, DdNode * bF );
extern DdNode *     Extra_bddMove( DdManager * dd, DdNode * bF, int nVars );
extern DdNode *     extraBddMove( DdManager * dd, DdNode * bF, DdNode * bFlag );
extern void         Extra_StopManager( DdManager * dd );
extern void         Extra_bddPrint( DdManager * dd, DdNode * F );
extern void         Extra_bddPrintSupport( DdManager * dd, DdNode * F );
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
extern DdNode *     Extra_bddComputeCube( DdManager * dd, DdNode ** bXVars, int nVars );
extern DdNode *     Extra_bddChangePolarity( DdManager * dd, DdNode * bFunc, DdNode * bVars );
extern DdNode *     extraBddChangePolarity( DdManager * dd, DdNode * bFunc, DdNode * bVars );
extern int          Extra_bddVarIsInCube( DdNode * bCube, int iVar );
extern DdNode *     Extra_bddAndPermute( DdManager * ddF, DdNode * bF, DdManager * ddG, DdNode * bG, int * pPermute );
extern int          Extra_bddCountCubes( DdManager * dd, DdNode ** pFuncs, int nFuncs, int fMode, int nLimit, int * pGuide );
extern void         Extra_zddDumpPla( DdManager * dd, DdNode * zCover, int nVars, char * pFileName );

#ifndef ABC_PRB
#define ABC_PRB(dd,f)       printf("%s = ", #f); Extra_bddPrint(dd,f); printf("\n")
#endif

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

extern DdNode *    Extra_bddAndTime( DdManager * dd, DdNode * f, DdNode * g, int TimeOut );
extern DdNode *    Extra_bddAndAbstractTime( DdManager * manager, DdNode * f, DdNode * g, DdNode * cube, int TimeOut );
extern DdNode *    Extra_TransferPermuteTime( DdManager * ddSource, DdManager * ddDestination, DdNode * f, int * Permute, int TimeOut );

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

/**AutomaticEnd***************************************************************/



ABC_NAMESPACE_HEADER_END



#endif /* __EXTRA_H__ */
