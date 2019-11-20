/**CHeaderFile*****************************************************************

  FileName    [cuddInt.h]

  PackageName [cudd]

  Synopsis    [Internal data structures of the CUDD package.]

  Description []

  SeeAlso     []

  Author      [Fabio Somenzi]

  Copyright [This file was created at the University of Colorado at
  Boulder.  The University of Colorado at Boulder makes no warranty
  about the suitability of this software for any purpose.  It is
  presented on an AS IS basis.]

  Revision    [$Id: cuddInt.h,v 1.1.1.1 2003/02/24 22:23:52 wjiang Exp $]

******************************************************************************/

#ifndef _CUDDINT
#define _CUDDINT


/*---------------------------------------------------------------------------*/
/* Nested includes                                                           */
/*---------------------------------------------------------------------------*/

#ifdef DD_MIS
#include "array.h"
#include "list.h"
#include "st.h"
#include "espresso.h"
#include "node.h"
#ifdef SIS
#include "graph.h"
#include "astg.h"
#endif
#include "network.h"
#endif

#include <math.h>
#include "cudd.h"
#include "st.h"

#if defined(__GNUC__)
# define DD_INLINE __inline__
# if (__GNUC__ >2 || __GNUC_MINOR__ >=7)
#   define DD_UNUSED __attribute__ ((__unused__))
# else
#   define DD_UNUSED
# endif
#else
# if defined(__cplusplus)
#   define DD_INLINE inline
# else
#   define DD_INLINE
# endif
# define DD_UNUSED
#endif


/*---------------------------------------------------------------------------*/
/* Constant declarations                                                     */
/*---------------------------------------------------------------------------*/

#define DD_MAXREF		((DdHalfWord) ~0)

#define DD_DEFAULT_RESIZE	10	/* how many extra variables */
					/* should be added when resizing */
#define DD_MEM_CHUNK		1022

/* These definitions work for CUDD_VALUE_TYPE == double */
#define DD_ONE_VAL		(1.0)
#define DD_ZERO_VAL		(0.0)
#define DD_EPSILON		(1.0e-12)

/* The definitions of +/- infinity in terms of HUGE_VAL work on
** the DECstations and on many other combinations of OS/compiler.
*/
#ifdef HAVE_IEEE_754
#  define DD_PLUS_INF_VAL	(HUGE_VAL)
#else
#  define DD_PLUS_INF_VAL	(10e301)
#  define DD_CRI_HI_MARK	(10e150)
#  define DD_CRI_LO_MARK	(-(DD_CRI_HI_MARK))
#endif
#define DD_MINUS_INF_VAL	(-(DD_PLUS_INF_VAL))

#define DD_NON_CONSTANT		((DdNode *) 1)	/* for Cudd_bddIteConstant */

/* Unique table and cache management constants. */
#define DD_MAX_SUBTABLE_DENSITY 4	/* tells when to resize a subtable */
/* gc when this percent are dead (measured w.r.t. slots, not keys)
** The first limit (LO) applies normally. The second limit applies when
** the package believes more space for the unique table (i.e., more dead
** nodes) would improve performance, and the unique table is not already
** too large. The third limit applies when memory is low.
*/
#define DD_GC_FRAC_LO		DD_MAX_SUBTABLE_DENSITY * 0.25
#define DD_GC_FRAC_HI		DD_MAX_SUBTABLE_DENSITY * 1.0
#define DD_GC_FRAC_MIN		0.2
#define DD_MIN_HIT		30	/* resize cache when hit ratio
					   above this percentage (default) */
#define DD_MAX_LOOSE_FRACTION	5 /* 1 / (max fraction of memory used for
				     unique table in fast growth mode) */
#define DD_MAX_CACHE_FRACTION	3 /* 1 / (max fraction of memory used for
				     computed table if resizing enabled) */
#define DD_STASH_FRACTION	64 /* 1 / (fraction of memory set
				      aside for emergencies) */
#define DD_MAX_CACHE_TO_SLOTS_RATIO 4 /* used to limit the cache size */

/* Variable ordering default parameter values. */
#define DD_SIFT_MAX_VAR		1000
#define DD_SIFT_MAX_SWAPS	2000000
#define DD_DEFAULT_RECOMB	0
#define DD_MAX_REORDER_GROWTH	1.2
#define DD_FIRST_REORDER	4004	/* 4 for the constants */
#define DD_DYN_RATIO		2	/* when to dynamically reorder */

/* Primes for cache hash functions. */
#define DD_P1			12582917
#define DD_P2			4256249
#define DD_P3			741457
#define DD_P4			1618033999

/* Cache tags for 3-operand operators.  These tags are stored in the
** least significant bits of the cache operand pointers according to
** the following scheme.  The tag consists of two hex digits.  Both digits
** must be even, so that they do not interfere with complementation bits.
** The least significant one is stored in Bits 3:1 of the f operand in the
** cache entry.  Bit 1 is always 1, so that we can differentiate
** three-operand operations from one- and two-operand operations.
** Therefore, the least significant digit is one of {2,6,a,e}.  The most
** significant digit occupies Bits 3:1 of the g operand in the cache
** entry.  It can by any even digit between 0 and e.  This gives a total
** of 5 bits for the tag proper, which means a maximum of 32 three-operand
** operations. */
#define DD_ADD_ITE_TAG				0x02
#define DD_BDD_AND_ABSTRACT_TAG			0x06
#define DD_BDD_XOR_EXIST_ABSTRACT_TAG		0x0a
#define DD_BDD_ITE_TAG				0x0e
#define DD_ADD_BDD_DO_INTERVAL_TAG		0x22
#define DD_BDD_CLIPPING_AND_ABSTRACT_UP_TAG	0x26
#define DD_BDD_CLIPPING_AND_ABSTRACT_DOWN_TAG	0x2a
#define DD_BDD_COMPOSE_RECUR_TAG		0x2e
#define DD_ADD_COMPOSE_RECUR_TAG		0x42
#define DD_ADD_NON_SIM_COMPOSE_TAG		0x46
#define DD_EQUIV_DC_TAG				0x4a
#define DD_ZDD_ITE_TAG				0x4e
#define DD_ADD_ITE_CONSTANT_TAG			0x62
#define DD_ADD_EVAL_CONST_TAG			0x66
#define DD_BDD_ITE_CONSTANT_TAG			0x6a
#define DD_ADD_OUT_SUM_TAG			0x6e
#define DD_BDD_LEQ_UNLESS_TAG			0x82
#define DD_ADD_TRIANGLE_TAG			0x86

/* Generator constants. */
#define CUDD_GEN_CUBES 0
#define CUDD_GEN_NODES 1
#define CUDD_GEN_ZDD_PATHS 2
#define CUDD_GEN_EMPTY 0
#define CUDD_GEN_NONEMPTY 1


/*---------------------------------------------------------------------------*/
/* Stucture declarations                                                     */
/*---------------------------------------------------------------------------*/

struct DdGen {
    DdManager	*manager;
    int		type;
    int		status;
    union {
	struct {
	    int			*cube;
	    CUDD_VALUE_TYPE	value;
	} cubes;
	struct {
	    st_table		*visited;
	    st_generator	*stGen;
	} nodes;
    } gen;
    struct {
	int	sp;
	DdNode	**stack;
    } stack;
    DdNode	*node;
};

/*---------------------------------------------------------------------------*/
/* Type declarations                                                         */
/*---------------------------------------------------------------------------*/

/* Hooks in CUDD are functions that the application registers with the
** manager so that they are called at appropriate times. The functions
** are passed the manager as argument; they should return 1 if
** successful and 0 otherwise.
*/
typedef struct DdHook {		/* hook list element */
    int (*f) ARGS((DdManager *, char *, void *)); /* function to be called */
    struct DdHook *next;	/* next element in the list */
} DdHook;

#if SIZEOF_VOID_P == 8 && SIZEOF_INT == 4
typedef long ptrint;
typedef unsigned long ptruint;
#else
typedef int ptrint;
typedef unsigned int ptruint;
#endif

#ifdef __osf__
#pragma pointer_size save
#pragma pointer_size short
#endif

typedef DdNode *DdNodePtr;

/* Generic local cache item. */
typedef struct DdLocalCacheItem {
    DdNode *value;
#ifdef DD_CACHE_PROFILE
    ptrint count;
#endif
    DdNode *key[1];
} DdLocalCacheItem;

/* Local cache. */
typedef struct DdLocalCache {
    DdLocalCacheItem *item;
    unsigned int itemsize;
    unsigned int keysize;
    unsigned int slots;
    int shift;
    double lookUps;
    double minHit;
    double hits;
    unsigned int maxslots;
    DdManager *manager;
    struct DdLocalCache *next;
} DdLocalCache;

/* Generic hash item. */
typedef struct DdHashItem {
    struct DdHashItem *next;
    ptrint count;
    DdNode *value;
    DdNode *key[1];
} DdHashItem;

/* Local hash table */
typedef struct DdHashTable {
    unsigned int keysize;
    unsigned int itemsize;
    DdHashItem **bucket;
    DdHashItem *nextFree;
    DdHashItem **memoryList;
    unsigned int numBuckets;
    int shift;
    unsigned int size;
    unsigned int maxsize;
    DdManager *manager;
} DdHashTable;

typedef struct DdCache {
    DdNode *f,*g;		/* DDs */
    ptruint h;			/* either operator or DD */
    DdNode *data;		/* already constructed DD */
#ifdef DD_CACHE_PROFILE
    ptrint count;
#endif
} DdCache;

typedef struct DdSubtable {	/* subtable for one index */
    DdNode **nodelist;		/* hash table */
    int shift;			/* shift for hash function */
    unsigned int slots;		/* size of the hash table */
    unsigned int keys;		/* number of nodes stored in this table */
    unsigned int maxKeys;	/* slots * DD_MAX_SUBTABLE_DENSITY */
    unsigned int dead;		/* number of dead nodes in this table */
    unsigned int next;		/* index of next variable in group */
    int bindVar;		/* flag to bind this variable to its level */
    /* Fields for lazy sifting. */
    Cudd_VariableType varType;  /* variable type (ps, ns, pi) */
    int pairIndex;              /* corresponding variable index (ps <-> ns) */
    int varHandled;		/* flag: 1 means variable is already handled */
    Cudd_LazyGroupType varToBeGrouped; /* tells what grouping to apply */
} DdSubtable;

struct DdManager {	/* specialized DD symbol table */
    /* Constants */
    DdNode sentinel;		/* for collision lists */
    DdNode *one;		/* constant 1 */
    DdNode *zero;		/* constant 0 */
    DdNode *plusinfinity;	/* plus infinity */
    DdNode *minusinfinity;	/* minus infinity */
    DdNode *background;		/* background value */
    /* Computed Table */
    DdCache *acache;		/* address of allocated memory for cache */
    DdCache *cache;		/* the cache-based computed table */
    unsigned int cacheSlots;	/* total number of cache entries */
    int cacheShift;		/* shift value for cache hash function */
    double cacheMisses;		/* number of cache misses (since resizing) */
    double cacheHits;		/* number of cache hits (since resizing) */
    double minHit;		/* hit percentage above which to resize */
    int cacheSlack;		/* slots still available for resizing */
    unsigned int maxCacheHard;	/* hard limit for cache size */
    /* Unique Table */
    int size;			/* number of unique subtables */
    int sizeZ;			/* for ZDD */
    int maxSize;		/* max number of subtables before resizing */
    int maxSizeZ;		/* for ZDD */
    DdSubtable *subtables;	/* array of unique subtables */
    DdSubtable *subtableZ;	/* for ZDD */
    DdSubtable constants;	/* unique subtable for the constants */
    unsigned int slots;		/* total number of hash buckets */
    unsigned int keys;		/* total number of BDD and ADD nodes */
    unsigned int keysZ;		/* total number of ZDD nodes */
    unsigned int dead;		/* total number of dead BDD and ADD nodes */
    unsigned int deadZ;		/* total number of dead ZDD nodes */
    unsigned int maxLive;	/* maximum number of live nodes */
    unsigned int minDead;	/* do not GC if fewer than these dead */
    double gcFrac;		/* gc when this fraction is dead */
    int gcEnabled;		/* gc is enabled */
    unsigned int looseUpTo;	/* slow growth beyond this limit */
				/* (measured w.r.t. slots, not keys) */
    unsigned int initSlots;	/* initial size of a subtable */
    DdNode **stack;		/* stack for iterative procedures */
    double allocated;		/* number of nodes allocated */
				/* (not during reordering) */
    double reclaimed;		/* number of nodes brought back from the dead */
    int isolated;		/* isolated projection functions */
    int *perm;			/* current variable perm. (index to level) */
    int *permZ;			/* for ZDD */
    int *invperm;		/* current inv. var. perm. (level to index) */
    int *invpermZ;		/* for ZDD */
    DdNode **vars;		/* projection functions */
    int *map;			/* variable map for fast swap */
    DdNode **univ;		/* ZDD 1 for each variable */
    int linearSize;		/* number of rows and columns of linear */
    long *interact;		/* interacting variable matrix */
    long *linear;		/* linear transform matrix */
    /* Memory Management */
    DdNode **memoryList;	/* memory manager for symbol table */
    DdNode *nextFree;		/* list of free nodes */
    char *stash;		/* memory reserve */
#ifndef DD_NO_DEATH_ROW
    DdNode **deathRow;		/* queue for dereferencing */
    int deathRowDepth;		/* number of slots in the queue */
    int nextDead;		/* index in the queue */
    unsigned deadMask;		/* mask for circular index update */
#endif
    /* General Parameters */
    CUDD_VALUE_TYPE epsilon;	/* tolerance on comparisons */
    /* Dynamic Reordering Parameters */
    int reordered;		/* flag set at the end of reordering */
    int reorderings;		/* number of calls to Cudd_ReduceHeap */
    int siftMaxVar;		/* maximum number of vars sifted */
    int siftMaxSwap;		/* maximum number of swaps per sifting */
    double maxGrowth;		/* maximum growth during reordering */
    double maxGrowthAlt;	/* alternate maximum growth for reordering */
    int reordCycle;		/* how often to apply alternate threshold */
    int autoDyn;		/* automatic dynamic reordering flag (BDD) */
    int autoDynZ;		/* automatic dynamic reordering flag (ZDD) */
    Cudd_ReorderingType autoMethod;  /* default reordering method */
    Cudd_ReorderingType autoMethodZ; /* default reordering method (ZDD) */
    int realign;		/* realign ZDD order after BDD reordering */
    int realignZ;		/* realign BDD order after ZDD reordering */
    unsigned int nextDyn;	/* reorder if this size is reached */
    unsigned int countDead;	/* if 0, count deads to trigger reordering */
    MtrNode *tree;		/* Variable group tree (BDD) */
    MtrNode *treeZ;		/* Variable group tree (ZDD) */
    Cudd_AggregationType groupcheck; /* Used during group sifting */
    int recomb;			/* Used during group sifting */
    int symmviolation;		/* Used during group sifting */
    int arcviolation;		/* Used during group sifting */
    int populationSize;		/* population size for GA */
    int	numberXovers;		/* number of crossovers for GA */
    DdLocalCache *localCaches;	/* local caches currently in existence */
#ifdef __osf__
#pragma pointer_size restore
#endif
    char *hooks;		/* application-specific field (used by vis) */
    DdHook *preGCHook;		/* hooks to be called before GC */
    DdHook *postGCHook;		/* hooks to be called after GC */
    DdHook *preReorderingHook;	/* hooks to be called before reordering */
    DdHook *postReorderingHook;	/* hooks to be called after reordering */
    FILE *out;			/* stdout for this manager */
    FILE *err;			/* stderr for this manager */
#ifdef __osf__
#pragma pointer_size save
#pragma pointer_size short
#endif
    Cudd_ErrorType errorCode;	/* info on last error */
    /* Statistical counters. */
    long memused;		/* total memory allocated for the manager */
    long maxmem;		/* target maximum memory */
    long maxmemhard;		/* hard limit for maximum memory */
    int garbageCollections;	/* number of garbage collections */
    long GCTime;		/* total time spent in garbage collection */
    long reordTime;		/* total time spent in reordering */
    double totCachehits;	/* total number of cache hits */
    double totCacheMisses;	/* total number of cache misses */
    double cachecollisions;	/* number of cache collisions */
    double cacheinserts;	/* number of cache insertions */
    double cacheLastInserts;	/* insertions at the last cache resizing */
    double cachedeletions;	/* number of deletions during garbage coll. */
#ifdef DD_STATS
    double nodesFreed;		/* number of nodes returned to the free list */
    double nodesDropped;	/* number of nodes killed by dereferencing */
#endif
    unsigned int peakLiveNodes;	/* maximum number of live nodes */
#ifdef DD_UNIQUE_PROFILE
    double uniqueLookUps;	/* number of unique table lookups */
    double uniqueLinks;		/* total distance traveled in coll. chains */
#endif
#ifdef DD_COUNT
    double recursiveCalls;	/* number of recursive calls */
#ifdef DD_STATS
    double nextSample;		/* when to write next line of stats */
#endif
    double swapSteps;		/* number of elementary reordering steps */
#endif
#ifdef DD_MIS
    /* mis/verif compatibility fields */
    array_t *iton;		/* maps ids in ddNode to node_t */
    array_t *order;		/* copy of order_list */
    lsHandle handle;		/* where it is in network BDD list */
    network_t *network;
    st_table *local_order;	/* for local BDDs */
    int nvars;			/* variables used so far */
    int threshold;		/* for pseudo var threshold value*/
#endif
};

typedef struct Move {
    DdHalfWord x;
    DdHalfWord y;
    unsigned int flags;
    int size;
    struct Move *next;
} Move;

/* Generic level queue item. */
typedef struct DdQueueItem {
    struct DdQueueItem *next;
    struct DdQueueItem *cnext;
    void *key;
} DdQueueItem;

/* Level queue. */
typedef struct DdLevelQueue {
    void *first;
    DdQueueItem **last;
    DdQueueItem *freelist;
    DdQueueItem **buckets;
    int levels;
    int itemsize;
    int size;
    int maxsize;
    int numBuckets;
    int shift;
} DdLevelQueue;

#ifdef __osf__
#pragma pointer_size restore
#endif

/*---------------------------------------------------------------------------*/
/* Variable declarations                                                     */
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
/* Macro declarations                                                        */
/*---------------------------------------------------------------------------*/

/**Macro***********************************************************************

  Synopsis    [Adds node to the head of the free list.]

  Description [Adds node to the head of the free list.  Does not
  deallocate memory chunks that become free.  This function is also
  used by the dynamic reordering functions.]

  SideEffects [None]

  SeeAlso     [cuddAllocNode cuddDynamicAllocNode]

******************************************************************************/
#define cuddDeallocNode(unique,node) \
    (node)->next = (unique)->nextFree; \
    (unique)->nextFree = node;


/**Macro***********************************************************************

  Synopsis     [Increases the reference count of a node, if it is not
  saturated.]

  Description  [Increases the reference count of a node, if it is not
  saturated. This being a macro, it is faster than Cudd_Ref, but it
  cannot be used in constructs like cuddRef(a = b()).]

  SideEffects  [none]

  SeeAlso      [Cudd_Ref]

******************************************************************************/
#define cuddRef(n) cuddSatInc(Cudd_Regular(n)->ref)


/**Macro***********************************************************************

  Synopsis     [Decreases the reference count of a node, if it is not
  saturated.]

  Description  [Decreases the reference count of node. It is primarily
  used in recursive procedures to decrease the ref count of a result
  node before returning it. This accomplishes the goal of removing the
  protection applied by a previous cuddRef. This being a macro, it is
  faster than Cudd_Deref, but it cannot be used in constructs like
  cuddDeref(a = b()).]

  SideEffects  [none]

  SeeAlso      [Cudd_Deref]

******************************************************************************/
#define cuddDeref(n) cuddSatDec(Cudd_Regular(n)->ref)


/**Macro***********************************************************************

  Synopsis     [Returns 1 if the node is a constant node.]

  Description  [Returns 1 if the node is a constant node (rather than an
  internal node). All constant nodes have the same index
  (CUDD_CONST_INDEX). The pointer passed to cuddIsConstant must be regular.]

  SideEffects  [none]

  SeeAlso      [Cudd_IsConstant]

******************************************************************************/
#define cuddIsConstant(node) ((node)->index == CUDD_CONST_INDEX)


/**Macro***********************************************************************

  Synopsis     [Returns the then child of an internal node.]

  Description  [Returns the then child of an internal node. If
  <code>node</code> is a constant node, the result is unpredictable.
  The pointer passed to cuddT must be regular.]

  SideEffects  [none]

  SeeAlso      [Cudd_T]

******************************************************************************/
#define cuddT(node) ((node)->type.kids.T)


/**Macro***********************************************************************

  Synopsis     [Returns the else child of an internal node.]

  Description  [Returns the else child of an internal node. If
  <code>node</code> is a constant node, the result is unpredictable.
  The pointer passed to cuddE must be regular.]

  SideEffects  [none]

  SeeAlso      [Cudd_E]

******************************************************************************/
#define cuddE(node) ((node)->type.kids.E)


/**Macro***********************************************************************

  Synopsis     [Returns the value of a constant node.]

  Description  [Returns the value of a constant node. If
  <code>node</code> is an internal node, the result is unpredictable.
  The pointer passed to cuddV must be regular.]

  SideEffects  [none]

  SeeAlso      [Cudd_V]

******************************************************************************/
#define cuddV(node) ((node)->type.value)


/**Macro***********************************************************************

  Synopsis    [Finds the current position of variable index in the
  order.]

  Description [Finds the current position of variable index in the
  order.  This macro duplicates the functionality of Cudd_ReadPerm,
  but it does not check for out-of-bounds indices and it is more
  efficient.]

  SideEffects [none]

  SeeAlso     [Cudd_ReadPerm]

******************************************************************************/
#define	cuddI(dd,index) (((index)==CUDD_CONST_INDEX)?(int)(index):(dd)->perm[(index)])


/**Macro***********************************************************************

  Synopsis    [Finds the current position of ZDD variable index in the
  order.]

  Description [Finds the current position of ZDD variable index in the
  order.  This macro duplicates the functionality of Cudd_ReadPermZdd,
  but it does not check for out-of-bounds indices and it is more
  efficient.]

  SideEffects [none]

  SeeAlso     [Cudd_ReadPermZdd]

******************************************************************************/
#define	cuddIZ(dd,index) (((index)==CUDD_CONST_INDEX)?(int)(index):(dd)->permZ[(index)])


/**Macro***********************************************************************

  Synopsis    [Hash function for the unique table.]

  Description []

  SideEffects [none]

  SeeAlso     [ddCHash ddCHash2]

******************************************************************************/
#if SIZEOF_VOID_P == 8 && SIZEOF_INT == 4
#define ddHash(f,g,s) \
((((unsigned)(unsigned long)(f) * DD_P1 + \
   (unsigned)(unsigned long)(g)) * DD_P2) >> (s))
#else
#define ddHash(f,g,s) \
((((unsigned)(f) * DD_P1 + (unsigned)(g)) * DD_P2) >> (s))
#endif


/**Macro***********************************************************************

  Synopsis    [Hash function for the cache.]

  Description []

  SideEffects [none]

  SeeAlso     [ddHash ddCHash2]

******************************************************************************/
#if SIZEOF_VOID_P == 8 && SIZEOF_INT == 4
#define ddCHash(o,f,g,h,s) \
((((((unsigned)(unsigned long)(f) + (unsigned)(unsigned long)(o)) * DD_P1 + \
    (unsigned)(unsigned long)(g)) * DD_P2 + \
   (unsigned)(unsigned long)(h)) * DD_P3) >> (s))
#else
#define ddCHash(o,f,g,h,s) \
((((((unsigned)(f) + (unsigned)(o)) * DD_P1 + (unsigned)(g)) * DD_P2 + \
   (unsigned)(h)) * DD_P3) >> (s))
#endif


/**Macro***********************************************************************

  Synopsis    [Hash function for the cache for functions with two
  operands.]

  Description []

  SideEffects [none]

  SeeAlso     [ddHash ddCHash]

******************************************************************************/
#if SIZEOF_VOID_P == 8 && SIZEOF_INT == 4
#define ddCHash2(o,f,g,s) \
(((((unsigned)(unsigned long)(f) + (unsigned)(unsigned long)(o)) * DD_P1 + \
   (unsigned)(unsigned long)(g)) * DD_P2) >> (s))
#else
#define ddCHash2(o,f,g,s) \
(((((unsigned)(f) + (unsigned)(o)) * DD_P1 + (unsigned)(g)) * DD_P2) >> (s))
#endif


/**Macro***********************************************************************

  Synopsis    [Clears the 4 least significant bits of a pointer.]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/
#define cuddClean(p) ((DdNode *)((ptruint)(p) & ~0xf))


/**Macro***********************************************************************

  Synopsis    [Computes the minimum of two numbers.]

  Description []

  SideEffects [none]

  SeeAlso     [ddMax]

******************************************************************************/
#define ddMin(x,y) (((y) < (x)) ? (y) : (x))


/**Macro***********************************************************************

  Synopsis    [Computes the maximum of two numbers.]

  Description []

  SideEffects [none]

  SeeAlso     [ddMin]

******************************************************************************/
#define ddMax(x,y) (((y) > (x)) ? (y) : (x))


/**Macro***********************************************************************

  Synopsis    [Computes the absolute value of a number.]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/
#define ddAbs(x) (((x)<0) ? -(x) : (x))


/**Macro***********************************************************************

  Synopsis    [Returns 1 if the absolute value of the difference of the two
  arguments x and y is less than e.]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/
#define ddEqualVal(x,y,e) (ddAbs((x)-(y))<(e))


/**Macro***********************************************************************

  Synopsis    [Saturating increment operator.]

  Description []

  SideEffects [none]

  SeeAlso     [cuddSatDec]

******************************************************************************/
#if SIZEOF_VOID_P == 8 && SIZEOF_INT == 4
#define cuddSatInc(x) ((x)++)
#else
#define cuddSatInc(x) ((x) += (x) != (DdHalfWord)DD_MAXREF)
#endif


/**Macro***********************************************************************

  Synopsis    [Saturating decrement operator.]

  Description []

  SideEffects [none]

  SeeAlso     [cuddSatInc]

******************************************************************************/
#if SIZEOF_VOID_P == 8 && SIZEOF_INT == 4
#define cuddSatDec(x) ((x)--)
#else
#define cuddSatDec(x) ((x) -= (x) != (DdHalfWord)DD_MAXREF)
#endif


/**Macro***********************************************************************

  Synopsis    [Returns the constant 1 node.]

  Description []

  SideEffects [none]

  SeeAlso     [DD_ZERO DD_PLUS_INFINITY DD_MINUS_INFINITY]

******************************************************************************/
#define DD_ONE(dd)		((dd)->one)


/**Macro***********************************************************************

  Synopsis    [Returns the arithmetic 0 constant node.]

  Description [Returns the arithmetic 0 constant node. This is different
  from the logical zero. The latter is obtained by
  Cudd_Not(DD_ONE(dd)).]

  SideEffects [none]

  SeeAlso     [DD_ONE Cudd_Not DD_PLUS_INFINITY DD_MINUS_INFINITY]

******************************************************************************/
#define DD_ZERO(dd) ((dd)->zero)


/**Macro***********************************************************************

  Synopsis    [Returns the plus infinity constant node.]

  Description []

  SideEffects [none]

  SeeAlso     [DD_ONE DD_ZERO DD_MINUS_INFINITY]

******************************************************************************/
#define DD_PLUS_INFINITY(dd) ((dd)->plusinfinity)


/**Macro***********************************************************************

  Synopsis    [Returns the minus infinity constant node.]

  Description []

  SideEffects [none]

  SeeAlso     [DD_ONE DD_ZERO DD_PLUS_INFINITY]

******************************************************************************/
#define DD_MINUS_INFINITY(dd) ((dd)->minusinfinity)


/**Macro***********************************************************************

  Synopsis    [Enforces DD_MINUS_INF_VAL <= x <= DD_PLUS_INF_VAL.]

  Description [Enforces DD_MINUS_INF_VAL <= x <= DD_PLUS_INF_VAL.
  Furthermore, if x <= DD_MINUS_INF_VAL/2, x is set to
  DD_MINUS_INF_VAL. Similarly, if DD_PLUS_INF_VAL/2 <= x, x is set to
  DD_PLUS_INF_VAL.  Normally this macro is a NOOP. However, if
  HAVE_IEEE_754 is not defined, it makes sure that a value does not
  get larger than infinity in absolute value, and once it gets to
  infinity, stays there.  If the value overflows before this macro is
  applied, no recovery is possible.]

  SideEffects [none]

  SeeAlso     []

******************************************************************************/
#ifdef HAVE_IEEE_754
#define cuddAdjust(x)
#else
#define cuddAdjust(x)		((x) = ((x) >= DD_CRI_HI_MARK) ? DD_PLUS_INF_VAL : (((x) <= DD_CRI_LO_MARK) ? DD_MINUS_INF_VAL : (x)))
#endif


/**Macro***********************************************************************

  Synopsis    [Extract the least significant digit of a double digit.]

  Description [Extract the least significant digit of a double digit. Used
  in the manipulation of arbitrary precision integers.]

  SideEffects [None]

  SeeAlso     [DD_MSDIGIT]

******************************************************************************/
#define DD_LSDIGIT(x)	((x) & DD_APA_MASK)


/**Macro***********************************************************************

  Synopsis    [Extract the most significant digit of a double digit.]

  Description [Extract the most significant digit of a double digit. Used
  in the manipulation of arbitrary precision integers.]

  SideEffects [None]

  SeeAlso     [DD_LSDIGIT]

******************************************************************************/
#define DD_MSDIGIT(x)	((x) >> DD_APA_BITS)


/**Macro***********************************************************************

  Synopsis    [Outputs a line of stats.]

  Description [Outputs a line of stats if DD_COUNT and DD_STATS are
  defined. Increments the number of recursive calls if DD_COUNT is
  defined.]

  SideEffects [None]

  SeeAlso     []

******************************************************************************/
#ifdef DD_COUNT
#ifdef DD_STATS
#define statLine(dd) dd->recursiveCalls++; \
if (dd->recursiveCalls == dd->nextSample) {(void) fprintf(dd->err, \
"@%.0f: %u nodes %u live %.0f dropped %.0f reclaimed\n", dd->recursiveCalls, \
dd->keys, dd->keys - dd->dead, dd->nodesDropped, dd->reclaimed); \
dd->nextSample += 250000;}
#else
#define statLine(dd) dd->recursiveCalls++;
#endif
#else
#define statLine(dd)
#endif


/**AutomaticStart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Function prototypes                                                       */
/*---------------------------------------------------------------------------*/

EXTERN DdNode * cuddAddExistAbstractRecur ARGS((DdManager *manager, DdNode *f, DdNode *cube));
EXTERN DdNode * cuddAddUnivAbstractRecur ARGS((DdManager *manager, DdNode *f, DdNode *cube));
EXTERN DdNode * cuddAddOrAbstractRecur ARGS((DdManager *manager, DdNode *f, DdNode *cube));
EXTERN DdNode * cuddAddApplyRecur ARGS((DdManager *dd, DdNode * (*)(DdManager *, DdNode **, DdNode **), DdNode *f, DdNode *g));
EXTERN DdNode * cuddAddMonadicApplyRecur ARGS((DdManager * dd, DdNode * (*op)(DdManager *, DdNode *), DdNode * f));
EXTERN DdNode * cuddAddScalarInverseRecur ARGS((DdManager *dd, DdNode *f, DdNode *epsilon));
EXTERN DdNode * cuddAddIteRecur ARGS((DdManager *dd, DdNode *f, DdNode *g, DdNode *h));
EXTERN DdNode * cuddAddCmplRecur ARGS((DdManager *dd, DdNode *f));
EXTERN DdNode * cuddAddNegateRecur ARGS((DdManager *dd, DdNode *f));
EXTERN DdNode * cuddAddRoundOffRecur ARGS((DdManager *dd, DdNode *f, double trunc));
EXTERN DdNode * cuddUnderApprox ARGS((DdManager *dd, DdNode *f, int numVars, int threshold, int safe, double quality));
EXTERN DdNode * cuddRemapUnderApprox ARGS((DdManager *dd, DdNode *f, int numVars, int threshold, double quality));
EXTERN DdNode * cuddBiasedUnderApprox ARGS((DdManager *dd, DdNode *f, DdNode *b, int numVars, int threshold, double quality1, double quality0));
EXTERN DdNode * cuddBddAndAbstractRecur ARGS((DdManager *manager, DdNode *f, DdNode *g, DdNode *cube));
EXTERN int cuddAnnealing ARGS((DdManager *table, int lower, int upper));
EXTERN DdNode * cuddBddExistAbstractRecur ARGS((DdManager *manager, DdNode *f, DdNode *cube));
EXTERN DdNode * cuddBddXorExistAbstractRecur ARGS((DdManager *manager, DdNode *f, DdNode *g, DdNode *cube));
EXTERN DdNode * cuddBddBooleanDiffRecur ARGS((DdManager *manager, DdNode *f, DdNode *var));
EXTERN DdNode * cuddBddIteRecur ARGS((DdManager *dd, DdNode *f, DdNode *g, DdNode *h));
EXTERN DdNode * cuddBddIntersectRecur ARGS((DdManager *dd, DdNode *f, DdNode *g));
EXTERN DdNode * cuddBddAndRecur ARGS((DdManager *manager, DdNode *f, DdNode *g));
EXTERN DdNode * cuddBddXorRecur ARGS((DdManager *manager, DdNode *f, DdNode *g));
EXTERN DdNode * cuddBddTransfer ARGS((DdManager *ddS, DdManager *ddD, DdNode *f));
EXTERN DdNode * cuddAddBddDoPattern ARGS((DdManager *dd, DdNode *f));
EXTERN int cuddInitCache ARGS((DdManager *unique, unsigned int cacheSize, unsigned int maxCacheSize));
EXTERN void cuddCacheInsert ARGS((DdManager *table, ptruint op, DdNode *f, DdNode *g, DdNode *h, DdNode *data));
EXTERN void cuddCacheInsert2 ARGS((DdManager *table, DdNode * (*)(DdManager *, DdNode *, DdNode *), DdNode *f, DdNode *g, DdNode *data));
EXTERN void cuddCacheInsert1 ARGS((DdManager *table, DdNode * (*)(DdManager *, DdNode *), DdNode *f, DdNode *data));
EXTERN DdNode * cuddCacheLookup ARGS((DdManager *table, ptruint op, DdNode *f, DdNode *g, DdNode *h));
EXTERN DdNode * cuddCacheLookupZdd ARGS((DdManager *table, ptruint op, DdNode *f, DdNode *g, DdNode *h));
EXTERN DdNode * cuddCacheLookup2 ARGS((DdManager *table, DdNode * (*)(DdManager *, DdNode *, DdNode *), DdNode *f, DdNode *g));
EXTERN DdNode * cuddCacheLookup1 ARGS((DdManager *table, DdNode * (*)(DdManager *, DdNode *), DdNode *f));
EXTERN DdNode * cuddCacheLookup2Zdd ARGS((DdManager *table, DdNode * (*)(DdManager *, DdNode *, DdNode *), DdNode *f, DdNode *g));
EXTERN DdNode * cuddCacheLookup1Zdd ARGS((DdManager *table, DdNode * (*)(DdManager *, DdNode *), DdNode *f));
EXTERN DdNode * cuddConstantLookup ARGS((DdManager *table, ptruint op, DdNode *f, DdNode *g, DdNode *h));
EXTERN int cuddCacheProfile ARGS((DdManager *table, FILE *fp));
EXTERN void cuddCacheResize ARGS((DdManager *table));
EXTERN void cuddCacheFlush ARGS((DdManager *table));
EXTERN int cuddComputeFloorLog2 ARGS((unsigned int value));
EXTERN int cuddHeapProfile ARGS((DdManager *dd));
EXTERN void cuddPrintNode ARGS((DdNode *f, FILE *fp));
EXTERN void cuddPrintVarGroups ARGS((DdManager * dd, MtrNode * root, int zdd, int silent));
EXTERN DdNode * cuddBddClippingAnd ARGS((DdManager *dd, DdNode *f, DdNode *g, int maxDepth, int direction));
EXTERN DdNode * cuddBddClippingAndAbstract ARGS((DdManager *dd, DdNode *f, DdNode *g, DdNode *cube, int maxDepth, int direction));
EXTERN void cuddGetBranches ARGS((DdNode *g, DdNode **g1, DdNode **g0));
EXTERN int cuddCheckCube ARGS((DdManager *dd, DdNode *g));
EXTERN DdNode * cuddCofactorRecur ARGS((DdManager *dd, DdNode *f, DdNode *g));
EXTERN DdNode * cuddBddComposeRecur ARGS((DdManager *dd, DdNode *f, DdNode *g, DdNode *proj));
EXTERN DdNode * cuddAddComposeRecur ARGS((DdManager *dd, DdNode *f, DdNode *g, DdNode *proj));
EXTERN int cuddExact ARGS((DdManager *table, int lower, int upper));
EXTERN DdNode * cuddBddConstrainRecur ARGS((DdManager *dd, DdNode *f, DdNode *c));
EXTERN DdNode * cuddBddRestrictRecur ARGS((DdManager *dd, DdNode *f, DdNode *c));
EXTERN DdNode * cuddAddConstrainRecur ARGS((DdManager *dd, DdNode *f, DdNode *c));
EXTERN DdNode * cuddAddRestrictRecur ARGS((DdManager *dd, DdNode *f, DdNode *c));
EXTERN DdNode * cuddBddLICompaction ARGS((DdManager *dd, DdNode *f, DdNode *c));
EXTERN int cuddGa ARGS((DdManager *table, int lower, int upper));
EXTERN int cuddTreeSifting ARGS((DdManager *table, Cudd_ReorderingType method));
EXTERN int cuddZddInitUniv ARGS((DdManager *zdd));
EXTERN void cuddZddFreeUniv ARGS((DdManager *zdd));
EXTERN void cuddSetInteract ARGS((DdManager *table, int x, int y));
EXTERN int cuddTestInteract ARGS((DdManager *table, int x, int y));
EXTERN int cuddInitInteract ARGS((DdManager *table));
EXTERN DdLocalCache * cuddLocalCacheInit ARGS((DdManager *manager, unsigned int keySize, unsigned int cacheSize, unsigned int maxCacheSize));
EXTERN void cuddLocalCacheQuit ARGS((DdLocalCache *cache));
EXTERN void cuddLocalCacheInsert ARGS((DdLocalCache *cache, DdNodePtr *key, DdNode *value));
EXTERN DdNode * cuddLocalCacheLookup ARGS((DdLocalCache *cache, DdNodePtr *key));
EXTERN void cuddLocalCacheClearDead ARGS((DdManager *manager));
EXTERN int cuddIsInDeathRow ARGS((DdManager *dd, DdNode *f));
EXTERN int cuddTimesInDeathRow ARGS((DdManager *dd, DdNode *f));
EXTERN void cuddLocalCacheClearAll ARGS((DdManager *manager));
#ifdef DD_CACHE_PROFILE
EXTERN int cuddLocalCacheProfile ARGS((DdLocalCache *cache));
#endif
EXTERN DdHashTable * cuddHashTableInit ARGS((DdManager *manager, unsigned int keySize, unsigned int initSize));
EXTERN void cuddHashTableQuit ARGS((DdHashTable *hash));
EXTERN int cuddHashTableInsert ARGS((DdHashTable *hash, DdNodePtr *key, DdNode *value, ptrint count));
EXTERN DdNode * cuddHashTableLookup ARGS((DdHashTable *hash, DdNodePtr *key));
EXTERN int cuddHashTableInsert1 ARGS((DdHashTable *hash, DdNode *f, DdNode *value, ptrint count));
EXTERN DdNode * cuddHashTableLookup1 ARGS((DdHashTable *hash, DdNode *f));
EXTERN int cuddHashTableInsert2 ARGS((DdHashTable *hash, DdNode *f, DdNode *g, DdNode *value, ptrint count));
EXTERN DdNode * cuddHashTableLookup2 ARGS((DdHashTable *hash, DdNode *f, DdNode *g));
EXTERN int cuddHashTableInsert3 ARGS((DdHashTable *hash, DdNode *f, DdNode *g, DdNode *h, DdNode *value, ptrint count));
EXTERN DdNode * cuddHashTableLookup3 ARGS((DdHashTable *hash, DdNode *f, DdNode *g, DdNode *h));
EXTERN DdLevelQueue * cuddLevelQueueInit ARGS((int levels, int itemSize, int numBuckets));
EXTERN void cuddLevelQueueQuit ARGS((DdLevelQueue *queue));
EXTERN void * cuddLevelQueueEnqueue ARGS((DdLevelQueue *queue, void *key, int level));
EXTERN void cuddLevelQueueDequeue ARGS((DdLevelQueue *queue, int level));
EXTERN int cuddLinearAndSifting ARGS((DdManager *table, int lower, int upper));
EXTERN DdNode * cuddBddLiteralSetIntersectionRecur ARGS((DdManager *dd, DdNode *f, DdNode *g));
EXTERN DdNode * cuddCProjectionRecur ARGS((DdManager *dd, DdNode *R, DdNode *Y, DdNode *Ysupp));
EXTERN DdNode * cuddBddClosestCube ARGS((DdManager *dd, DdNode *f, DdNode *g, CUDD_VALUE_TYPE bound));
EXTERN void cuddReclaim ARGS((DdManager *table, DdNode *n));
EXTERN void cuddReclaimZdd ARGS((DdManager *table, DdNode *n));
EXTERN void cuddClearDeathRow ARGS((DdManager *table));
EXTERN void cuddShrinkDeathRow ARGS((DdManager *table));
EXTERN DdNode * cuddDynamicAllocNode ARGS((DdManager *table));
EXTERN int cuddSifting ARGS((DdManager *table, int lower, int upper));
EXTERN int cuddSwapping ARGS((DdManager *table, int lower, int upper, Cudd_ReorderingType heuristic));
EXTERN int cuddNextHigh ARGS((DdManager *table, int x));
EXTERN int cuddNextLow ARGS((DdManager *table, int x));
EXTERN int cuddSwapInPlace ARGS((DdManager *table, int x, int y));
EXTERN int cuddBddAlignToZdd ARGS((DdManager *table));
EXTERN DdNode * cuddBddMakePrime ARGS((DdManager *dd, DdNode *cube, DdNode *f));
EXTERN DdNode * cuddSolveEqnRecur ARGS((DdManager *bdd, DdNode *F, DdNode *Y, DdNode **G, int n, int *yIndex, int i));
EXTERN DdNode * cuddVerifySol ARGS((DdManager *bdd, DdNode *F, DdNode **G, int *yIndex, int n));
#ifdef ST_INCLUDED
EXTERN DdNode* cuddSplitSetRecur ARGS((DdManager *manager, st_table *mtable, int *varSeen, DdNode *p, double n, double max, int index));
#endif
EXTERN DdNode * cuddSubsetHeavyBranch ARGS((DdManager *dd, DdNode *f, int numVars, int threshold));
EXTERN DdNode * cuddSubsetShortPaths ARGS((DdManager *dd, DdNode *f, int numVars, int threshold, int hardlimit));
EXTERN int cuddSymmCheck ARGS((DdManager *table, int x, int y));
EXTERN int cuddSymmSifting ARGS((DdManager *table, int lower, int upper));
EXTERN int cuddSymmSiftingConv ARGS((DdManager *table, int lower, int upper));
EXTERN DdNode * cuddAllocNode ARGS((DdManager *unique));
EXTERN DdManager * cuddInitTable ARGS((unsigned int numVars, unsigned int numVarsZ, unsigned int numSlots, unsigned int looseUpTo));
EXTERN void cuddFreeTable ARGS((DdManager *unique));
EXTERN int cuddGarbageCollect ARGS((DdManager *unique, int clearCache));
EXTERN int cuddGarbageCollectZdd ARGS((DdManager *unique, int clearCache));
EXTERN DdNode * cuddZddGetNode ARGS((DdManager *zdd, int id, DdNode *T, DdNode *E));
EXTERN DdNode * cuddZddGetNodeIVO ARGS((DdManager *dd, int index, DdNode *g, DdNode *h));
EXTERN DdNode * cuddUniqueInter ARGS((DdManager *unique, int index, DdNode *T, DdNode *E));
EXTERN DdNode * cuddUniqueInterIVO ARGS((DdManager *unique, int index, DdNode *T, DdNode *E));
EXTERN DdNode * cuddUniqueInterZdd ARGS((DdManager *unique, int index, DdNode *T, DdNode *E));
EXTERN DdNode * cuddUniqueConst ARGS((DdManager *unique, CUDD_VALUE_TYPE value));
EXTERN void cuddRehash ARGS((DdManager *unique, int i));
EXTERN void cuddShrinkSubtable ARGS((DdManager *unique, int i));
EXTERN int cuddInsertSubtables ARGS((DdManager *unique, int n, int level));
EXTERN int cuddDestroySubtables ARGS((DdManager *unique, int n));
EXTERN int cuddResizeTableZdd ARGS((DdManager *unique, int index));
EXTERN void cuddSlowTableGrowth ARGS((DdManager *unique));
EXTERN int cuddP ARGS((DdManager *dd, DdNode *f));
#ifdef ST_INCLUDED
EXTERN enum st_retval cuddStCountfree ARGS((char *key, char *value, char *arg));
EXTERN int cuddCollectNodes ARGS((DdNode *f, st_table *visited));
#endif
EXTERN int cuddWindowReorder ARGS((DdManager *table, int low, int high, Cudd_ReorderingType submethod));
EXTERN DdNode	* cuddZddProduct ARGS((DdManager *dd, DdNode *f, DdNode *g));
EXTERN DdNode	* cuddZddUnateProduct ARGS((DdManager *dd, DdNode *f, DdNode *g));
EXTERN DdNode	* cuddZddWeakDiv ARGS((DdManager *dd, DdNode *f, DdNode *g));
EXTERN DdNode	* cuddZddWeakDivF ARGS((DdManager *dd, DdNode *f, DdNode *g));
EXTERN DdNode	* cuddZddDivide ARGS((DdManager *dd, DdNode *f, DdNode *g));
EXTERN DdNode	* cuddZddDivideF ARGS((DdManager *dd, DdNode *f, DdNode *g));
EXTERN int cuddZddGetCofactors3 ARGS((DdManager *dd, DdNode *f, int v, DdNode **f1, DdNode **f0, DdNode **fd));
EXTERN int cuddZddGetCofactors2 ARGS((DdManager *dd, DdNode *f, int v, DdNode **f1, DdNode **f0));
EXTERN DdNode	* cuddZddComplement ARGS((DdManager *dd, DdNode *node));
EXTERN int cuddZddGetPosVarIndex(DdManager * dd, int index);
EXTERN int cuddZddGetNegVarIndex(DdManager * dd, int index);
EXTERN int cuddZddGetPosVarLevel(DdManager * dd, int index);
EXTERN int cuddZddGetNegVarLevel(DdManager * dd, int index);
EXTERN int cuddZddTreeSifting ARGS((DdManager *table, Cudd_ReorderingType method));
EXTERN DdNode	* cuddZddIsop ARGS((DdManager *dd, DdNode *L, DdNode *U, DdNode **zdd_I));
EXTERN DdNode	* cuddBddIsop ARGS((DdManager *dd, DdNode *L, DdNode *U));
EXTERN DdNode	* cuddMakeBddFromZddCover ARGS((DdManager *dd, DdNode *node));
EXTERN int cuddZddLinearSifting ARGS((DdManager *table, int lower, int upper));
EXTERN int cuddZddAlignToBdd ARGS((DdManager *table));
EXTERN int cuddZddNextHigh ARGS((DdManager *table, int x));
EXTERN int cuddZddNextLow ARGS((DdManager *table, int x));
EXTERN int cuddZddUniqueCompare ARGS((int *ptr_x, int *ptr_y));
EXTERN int cuddZddSwapInPlace ARGS((DdManager *table, int x, int y));
EXTERN int cuddZddSwapping ARGS((DdManager *table, int lower, int upper, Cudd_ReorderingType heuristic));
EXTERN int cuddZddSifting ARGS((DdManager *table, int lower, int upper));
EXTERN DdNode * cuddZddIte ARGS((DdManager *dd, DdNode *f, DdNode *g, DdNode *h));
EXTERN DdNode * cuddZddUnion ARGS((DdManager *zdd, DdNode *P, DdNode *Q));
EXTERN DdNode * cuddZddIntersect ARGS((DdManager *zdd, DdNode *P, DdNode *Q));
EXTERN DdNode * cuddZddDiff ARGS((DdManager *zdd, DdNode *P, DdNode *Q));
EXTERN DdNode * cuddZddChangeAux ARGS((DdManager *zdd, DdNode *P, DdNode *zvar));
EXTERN DdNode * cuddZddSubset1 ARGS((DdManager *dd, DdNode *P, int var));
EXTERN DdNode * cuddZddSubset0 ARGS((DdManager *dd, DdNode *P, int var));
EXTERN DdNode * cuddZddChange ARGS((DdManager *dd, DdNode *P, int var));
EXTERN int cuddZddSymmCheck ARGS((DdManager *table, int x, int y));
EXTERN int cuddZddSymmSifting ARGS((DdManager *table, int lower, int upper));
EXTERN int cuddZddSymmSiftingConv ARGS((DdManager *table, int lower, int upper));
EXTERN int cuddZddP ARGS((DdManager *zdd, DdNode *f));

/**AutomaticEnd***************************************************************/

#endif /* _CUDDINT */
