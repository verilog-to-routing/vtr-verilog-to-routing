/**CFile****************************************************************

  FileName    [cloud.h]

  PackageName [Fast application-specific BDD package.]

  Synopsis    [Interface of the package.]

  Author      [Alan Mishchenko <alanmi@ece.pdx.edu>]
  
  Affiliation [ECE Department. Portland State University, Portland, Oregon.]

  Date        [Ver. 1.0. Started - June 10, 2002.]

  Revision    [$Id: cloud.h,v 1.0 2002/06/10 03:00:00 alanmi Exp $]

***********************************************************************/

#ifndef ABC__aig__kit__cloud_h
#define ABC__aig__kit__cloud_h


#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "misc/util/abc_global.h"



ABC_NAMESPACE_HEADER_START


#ifdef _WIN32
#define inline __inline // compatible with MS VS 6.0
#endif

////////////////////////////////////////////////////////////////////////
//  n |  2^n ||  n |    2^n ||  n |        2^n ||  n |           2^n  //
//====================================================================//
//  1 |    2 ||  9 |    512 || 17 |    131,072 || 25 |    33,554,432  //
//  2 |    4 || 10 |  1,024 || 18 |    262,144 || 26 |    67,108,864  // 
//  3 |    8 || 11 |  2,048 || 19 |    524,288 || 27 |   134,217,728  //
//  4 |   16 || 12 |  4,096 || 20 |  1,048,576 || 28 |   268,435,456  //
//  5 |   32 || 13 |  8,192 || 21 |  2,097,152 || 29 |   536,870,912  //
//  6 |   64 || 14 | 16,384 || 22 |  4,194,304 || 30 | 1,073,741,824  //
//  7 |  128 || 15 | 32,768 || 23 |  8,388,608 || 31 | 2,147,483,648  //
//  8 |  256 || 16 | 65,536 || 24 | 16,777,216 || 32 | 4,294,967,296  //
////////////////////////////////////////////////////////////////////////

// data structure typedefs
typedef struct cloudManager       CloudManager;
typedef unsigned                  CloudVar;
typedef unsigned                  CloudSign;
typedef struct cloudNode          CloudNode;
typedef struct cloudCacheEntry1   CloudCacheEntry1;
typedef struct cloudCacheEntry2   CloudCacheEntry2;
typedef struct cloudCacheEntry3   CloudCacheEntry3;

// operation codes used to set up the cache
typedef enum { 
	CLOUD_OPER_AND, 
	CLOUD_OPER_XOR, 
	CLOUD_OPER_BDIFF, 
	CLOUD_OPER_LEQ 
} CloudOper;

/*
// the number of operators using cache
static int CacheOperNum = 4;

// the ratio of cache size to the unique table size for each operator
static int CacheLogRatioDefault[4] = {
	4, // CLOUD_OPER_AND, 
	8, // CLOUD_OPER_XOR, 
	8, // CLOUD_OPER_BDIFF, 
	8  // CLOUD_OPER_LEQ 
};

// the ratio of cache size to the unique table size for each operator
static int CacheSize[4] = {
	2, // CLOUD_OPER_AND, 
	2, // CLOUD_OPER_XOR, 
	2, // CLOUD_OPER_BDIFF, 
	2  // CLOUD_OPER_LEQ 
};
*/

// data structure definitions
struct cloudManager            // the fast bdd manager     
{
	// variables
	int nVars;                 // the number of variables allocated
	// bits
	int bitsNode;              // the number of bits used for the node
	int bitsCache[4];          // default: bitsNode - CacheSizeRatio[i]
	// shifts
	int shiftUnique;           // 8*sizeof(unsigned) - (bitsNode + 1)
	int shiftCache[4];         // 8*sizeof(unsigned) -  bitsCache[i]
	// nodes 
	int nNodesAlloc;           // 2 ^ (bitsNode + 1)
	int nNodesLimit;           // 2 ^  bitsNode
	int nNodesCur;             // the current number of nodes (including const1 and vars)
	// signature
	CloudSign nSignCur;

	// statistics
	int nMemUsed;              // memory usage in bytes
	// cache stats
	int nUniqueHits;           // hits in the unique table
	int nUniqueMisses;         // misses in the unique table
	int nCacheHits;            // hits in the caches
	int nCacheMisses;          // misses in the caches
	// the number of steps through the hash table
	int nUniqueSteps;

	// tables
	CloudNode * tUnique;       // the unique table to store BDD nodes

	// special nodes
	CloudNode * pNodeStart;    // the pointer to the first node
	CloudNode * pNodeEnd;      // the pointer to the first node out of the table

	// constants and variables
	CloudNode *  one;          // the one function
	CloudNode *  zero;         // the zero function
	CloudNode ** vars;         // the elementary variables

    // temporary storage for nodes
    CloudNode ** ppNodes;

	// caches
	CloudCacheEntry2 * tCaches[20];    // caches
};

struct cloudNode   // representation of the node in the unique table
{
	CloudSign   s;         // signature
	CloudVar    v;         // variable
	CloudNode * e;         // negative cofactor
	CloudNode * t;         // positive cofactor
};
struct cloudCacheEntry1  // one-argument cache
{
	CloudSign   s;         // signature
	CloudNode * a;         // argument 1
	CloudNode * r;         // result
};
struct cloudCacheEntry2  // the two-argument cache
{
	CloudSign   s;         // signature
	CloudNode * a;
	CloudNode * b;
	CloudNode * r;
};
struct cloudCacheEntry3  // the three-argument cache
{
	CloudSign   s;         // signature
	CloudNode * a;
	CloudNode * b;
	CloudNode * c;
	CloudNode * r;
};


// parameters
#define CLOUD_NODE_BITS              23

#define CLOUD_CONST_INDEX            ((unsigned)0x0fffffff)
#define CLOUD_MARK_ON                ((unsigned)0x10000000)
#define CLOUD_MARK_OFF               ((unsigned)0xefffffff)

// hash functions a la Buddy
#define cloudHashBuddy2(x,y,s)       ((((x)+(y))*((x)+(y)+1)/2) & ((1<<(32-(s)))-1))
#define cloudHashBuddy3(x,y,z,s)     (cloudHashBuddy2((cloudHashBuddy2((x),(y),(s))),(z),(s)) & ((1<<(32-(s)))-1))
// hash functions a la Cudd
#define DD_P1		  	             12582917
#define DD_P2			             4256249
#define DD_P3			             741457
#define DD_P4			             1618033999
#define cloudHashCudd2(f,g,s)        ((((unsigned)(ABC_PTRUINT_T)(f) * DD_P1 + (unsigned)(ABC_PTRUINT_T)(g)) * DD_P2) >> (s))
#define cloudHashCudd3(f,g,h,s)      (((((unsigned)(ABC_PTRUINT_T)(f) * DD_P1 + (unsigned)(ABC_PTRUINT_T)(g)) * DD_P2 + (unsigned)(ABC_PTRUINT_T)(h)) * DD_P3) >> (s))

// node complementation (using node)
#define Cloud_Regular(p)             ((CloudNode*)(((ABC_PTRUINT_T)(p)) & ~01))   // get the regular node (w/o bubble)
#define Cloud_Not(p)                 ((CloudNode*)(((ABC_PTRUINT_T)(p)) ^  01))   // complement the node
#define Cloud_NotCond(p,c)           ((CloudNode*)(((ABC_PTRUINT_T)(p)) ^ (c)))   // complement the node conditionally
#define Cloud_IsComplement(p)        ((int)(((ABC_PTRUINT_T)(p)) & 01))           // check if complemented
// checking constants (using node)
#define Cloud_IsConstant(p)          (((Cloud_Regular(p))->v & CLOUD_MARK_OFF) == CLOUD_CONST_INDEX)
#define cloudIsConstant(p)           (((p)->v & CLOUD_MARK_OFF) == CLOUD_CONST_INDEX)

// retrieving values from the node (using node structure)
#define Cloud_V(p)                   ((Cloud_Regular(p))->v)
#define Cloud_E(p)                   ((Cloud_Regular(p))->e)
#define Cloud_T(p)                   ((Cloud_Regular(p))->t)
// retrieving values from the regular node (using node structure)
#define cloudV(p)                    ((p)->v)
#define cloudE(p)                    ((p)->e)
#define cloudT(p)                    ((p)->t)
// marking/unmarking (using node structure)
#define cloudNodeMark(p)             ((p)->v       |= CLOUD_MARK_ON)
#define cloudNodeUnmark(p)           ((p)->v       &= CLOUD_MARK_OFF)
#define cloudNodeIsMarked(p)         ((int)((p)->v &  CLOUD_MARK_ON))

// cache lookups and inserts (using node)
#define cloudCacheLookup1(p,sign,f)     (((p)->s == (sign) && (p)->a == (f))? ((p)->r): (0))
#define cloudCacheLookup2(p,sign,f,g)   (((p)->s == (sign) && (p)->a == (f) && (p)->b == (g))? ((p)->r): (0))
#define cloudCacheLookup3(p,sign,f,g,h) (((p)->s == (sign) && (p)->a == (f) && (p)->b == (g) && (p)->c == (h))? ((p)->r): (0))
// cache inserts
#define cloudCacheInsert1(p,sign,f,r)     (((p)->s = (sign)), ((p)->a = (f)), ((p)->r = (r)))
#define cloudCacheInsert2(p,sign,f,g,r)   (((p)->s = (sign)), ((p)->a = (f)), ((p)->b = (g)), ((p)->r = (r)))
#define cloudCacheInsert3(p,sign,f,g,h,r) (((p)->s = (sign)), ((p)->a = (f)), ((p)->b = (g)), ((p)->c = (h)), ((p)->r = (r)))

//#define CLOUD_ASSERT(p)              (assert((p) >= (dd->pNodeStart-1) && (p) < dd->pNodeEnd))
#define CLOUD_ASSERT(p)            assert((p) >= dd->tUnique && (p) < dd->tUnique+dd->nNodesAlloc)

////////////////////////////////////////////////////////////////////////
///                       FUNCTION DECLARATIONS                      ///
////////////////////////////////////////////////////////////////////////
// starting/stopping 
extern CloudManager * Cloud_Init( int nVars, int nBits );
extern void           Cloud_Quit( CloudManager * dd );
extern void           Cloud_Restart( CloudManager * dd );
extern void           Cloud_CacheAllocate( CloudManager * dd, CloudOper oper, int size );
extern CloudNode *    Cloud_MakeNode( CloudManager * dd, CloudVar v, CloudNode * t, CloudNode * e );
// support and node count
extern CloudNode *    Cloud_Support( CloudManager * dd, CloudNode * n );
extern int            Cloud_SupportSize( CloudManager * dd, CloudNode * n );
extern int            Cloud_DagSize( CloudManager * dd, CloudNode * n );
extern int            Cloud_DagCollect( CloudManager * dd, CloudNode * n );
extern int            Cloud_SharingSize( CloudManager * dd, CloudNode * * pn, int nn );
// cubes
extern CloudNode *    Cloud_GetOneCube( CloudManager * dd, CloudNode * n );
extern void           Cloud_bddPrint( CloudManager * dd, CloudNode * Func );
extern void           Cloud_bddPrintCube( CloudManager * dd, CloudNode * Cube );
// operations
extern CloudNode *    Cloud_bddAnd( CloudManager * dd, CloudNode * f, CloudNode * g );
extern CloudNode *    Cloud_bddOr( CloudManager * dd, CloudNode * f, CloudNode * g );
// stats 
extern void           Cloud_PrintInfo( CloudManager * dd );
extern void           Cloud_PrintHashTable( CloudManager * dd );



ABC_NAMESPACE_HEADER_END



#endif

////////////////////////////////////////////////////////////////////////
///                            END OF FILE                           ///
////////////////////////////////////////////////////////////////////////
