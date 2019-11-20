/**CHeaderFile*****************************************************************

  FileName    [cudd.h]

  PackageName [cudd]

  Synopsis    [The University of Colorado decision diagram package.]

  Description [External functions and data strucures of the CUDD package.
  <ul>
  <li> To turn on the gathering of statistics, define DD_STATS.
  <li> To link with mis, define DD_MIS.
  </ul>
  Modified by Abelardo Pardo to interface it to VIS.
  ]

  SeeAlso     []

  Author      [Fabio Somenzi]

  Copyright [This file was created at the University of Colorado at
  Boulder.  The University of Colorado at Boulder makes no warranty
  about the suitability of this software for any purpose.  It is
  presented on an AS IS basis.]

  Revision    [$Id: cudd.h,v 1.1.1.1 2003/02/24 22:23:50 wjiang Exp $]

******************************************************************************/

#ifndef _CUDD
#define _CUDD

/*---------------------------------------------------------------------------*/
/* Nested includes                                                           */
/*---------------------------------------------------------------------------*/

#include "mtr.h"
#include "epd.h"

/*---------------------------------------------------------------------------*/
/* Constant declarations                                                     */
/*---------------------------------------------------------------------------*/

#define CUDD_VERSION "2.3.1"

#ifndef SIZEOF_VOID_P
#define SIZEOF_VOID_P 4
#endif
#ifndef SIZEOF_INT
#define SIZEOF_INT 4
#endif
#ifndef SIZEOF_LONG
#define SIZEOF_LONG 4
#endif

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#define CUDD_VALUE_TYPE		double
#define CUDD_OUT_OF_MEM		-1
/* The sizes of the subtables and the cache must be powers of two. */
#define CUDD_UNIQUE_SLOTS	256	/* initial size of subtables */
#define CUDD_CACHE_SLOTS	262144	/* default size of the cache */

/* Constants for residue functions. */
#define CUDD_RESIDUE_DEFAULT	0
#define CUDD_RESIDUE_MSB	1
#define CUDD_RESIDUE_TC		2

/* CUDD_MAXINDEX is defined in such a way that on 32-bit and 64-bit
** machines one can cast an index to (int) without generating a negative
** number.
*/
#if SIZEOF_VOID_P == 8 && SIZEOF_INT == 4
#define CUDD_MAXINDEX		(((DdHalfWord) ~0) >> 1)
#else
#define CUDD_MAXINDEX		((DdHalfWord) ~0)
#endif

/* CUDD_CONST_INDEX is the index of constant nodes.  Currently this
** is a synonim for CUDD_MAXINDEX. */
#define CUDD_CONST_INDEX	CUDD_MAXINDEX

/* These constants define the digits used in the representation of
** arbitrary precision integers.  The two configurations tested use 8
** and 16 bits for each digit.  The typedefs should be in agreement
** with these definitions.
*/
#define DD_APA_BITS	16
#define DD_APA_BASE	(1 << DD_APA_BITS)
#define DD_APA_MASK	(DD_APA_BASE - 1)
#define DD_APA_HEXPRINT	"%04x"

/*---------------------------------------------------------------------------*/
/* Stucture declarations                                                     */
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
/* Type declarations                                                         */
/*---------------------------------------------------------------------------*/

/**Enum************************************************************************

  Synopsis    [Type of reordering algorithm.]

  Description [Type of reordering algorithm.]

******************************************************************************/
typedef enum {
    CUDD_REORDER_SAME,
    CUDD_REORDER_NONE,
    CUDD_REORDER_RANDOM,
    CUDD_REORDER_RANDOM_PIVOT,
    CUDD_REORDER_SIFT,
    CUDD_REORDER_SIFT_CONVERGE,
    CUDD_REORDER_SYMM_SIFT,
    CUDD_REORDER_SYMM_SIFT_CONV,
    CUDD_REORDER_WINDOW2,
    CUDD_REORDER_WINDOW3,
    CUDD_REORDER_WINDOW4,
    CUDD_REORDER_WINDOW2_CONV,
    CUDD_REORDER_WINDOW3_CONV,
    CUDD_REORDER_WINDOW4_CONV,
    CUDD_REORDER_GROUP_SIFT,
    CUDD_REORDER_GROUP_SIFT_CONV,
    CUDD_REORDER_ANNEALING,
    CUDD_REORDER_GENETIC,
    CUDD_REORDER_LINEAR,
    CUDD_REORDER_LINEAR_CONVERGE,
    CUDD_REORDER_LAZY_SIFT,
    CUDD_REORDER_EXACT
} Cudd_ReorderingType;


/**Enum************************************************************************

  Synopsis    [Type of aggregation methods.]

  Description [Type of aggregation methods.]

******************************************************************************/
typedef enum {
    CUDD_NO_CHECK,
    CUDD_GROUP_CHECK,
    CUDD_GROUP_CHECK2,
    CUDD_GROUP_CHECK3,
    CUDD_GROUP_CHECK4,
    CUDD_GROUP_CHECK5,
    CUDD_GROUP_CHECK6,
    CUDD_GROUP_CHECK7,
    CUDD_GROUP_CHECK8,
    CUDD_GROUP_CHECK9
} Cudd_AggregationType;


/**Enum************************************************************************

  Synopsis    [Type of hooks.]

  Description [Type of hooks.]

******************************************************************************/
typedef enum {
    CUDD_PRE_GC_HOOK,
    CUDD_POST_GC_HOOK,
    CUDD_PRE_REORDERING_HOOK,
    CUDD_POST_REORDERING_HOOK
} Cudd_HookType;


/**Enum************************************************************************

  Synopsis    [Type of error codes.]

  Description [Type of  error codes.]

******************************************************************************/
typedef enum {
    CUDD_NO_ERROR,
    CUDD_MEMORY_OUT,
    CUDD_TOO_MANY_NODES,
    CUDD_MAX_MEM_EXCEEDED,
    CUDD_INVALID_ARG,
    CUDD_INTERNAL_ERROR
} Cudd_ErrorType;


/**Enum************************************************************************

  Synopsis    [Group type for lazy sifting.]

  Description [Group type for lazy sifting.]

******************************************************************************/
typedef enum {
    CUDD_LAZY_NONE,
    CUDD_LAZY_SOFT_GROUP,
    CUDD_LAZY_HARD_GROUP,
    CUDD_LAZY_UNGROUP
} Cudd_LazyGroupType;


/**Enum************************************************************************

  Synopsis    [Variable type.]

  Description [Variable type. Currently used only in lazy sifting.]

******************************************************************************/
typedef enum {
    CUDD_VAR_PRIMARY_INPUT,
    CUDD_VAR_PRESENT_STATE,
    CUDD_VAR_NEXT_STATE
} Cudd_VariableType;


#if SIZEOF_VOID_P == 8 && SIZEOF_INT == 4
typedef unsigned int   DdHalfWord;
#else
typedef unsigned short DdHalfWord;
#endif

#ifdef __osf__
#pragma pointer_size save
#pragma pointer_size short
#endif

typedef struct DdNode DdNode;

typedef struct DdChildren {
    struct DdNode *T;
    struct DdNode *E;
} DdChildren;

/* The DdNode structure is the only one exported out of the package */
struct DdNode {
    DdHalfWord index;
    DdHalfWord ref;		/* reference count */
    DdNode *next;		/* next pointer for unique table */
    union {
	CUDD_VALUE_TYPE value;	/* for constant nodes */
	DdChildren kids;	/* for internal nodes */
    } type;
};

#ifdef __osf__
#pragma pointer_size restore
#endif

typedef struct DdManager DdManager;

typedef struct DdGen DdGen;

/* These typedefs for arbitrary precision arithmetic should agree with
** the corresponding constant definitions above. */
typedef unsigned short int DdApaDigit;
typedef unsigned long int DdApaDoubleDigit;
typedef DdApaDigit * DdApaNumber;

/*---------------------------------------------------------------------------*/
/* Variable declarations                                                     */
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
/* Macro declarations                                                        */
/*---------------------------------------------------------------------------*/


/**Macro***********************************************************************

  Synopsis     [Returns 1 if the node is a constant node.]

  Description  [Returns 1 if the node is a constant node (rather than an
  internal node). All constant nodes have the same index
  (CUDD_CONST_INDEX). The pointer passed to Cudd_IsConstant may be either
  regular or complemented.]

  SideEffects  [none]

  SeeAlso      []

******************************************************************************/
#define Cudd_IsConstant(node) ((Cudd_Regular(node))->index == CUDD_CONST_INDEX)


/**Macro***********************************************************************

  Synopsis     [Complements a DD.]

  Description  [Complements a DD by flipping the complement attribute of
  the pointer (the least significant bit).]

  SideEffects  [none]

  SeeAlso      [Cudd_NotCond]

******************************************************************************/
#define Cudd_Not(node) ((DdNode *)((long)(node) ^ 01))


/**Macro***********************************************************************

  Synopsis     [Complements a DD if a condition is true.]

  Description  [Complements a DD if condition c is true; c should be
  either 0 or 1, because it is used directly (for efficiency). If in
  doubt on the values c may take, use "(c) ? Cudd_Not(node) : node".]

  SideEffects  [none]

  SeeAlso      [Cudd_Not]

******************************************************************************/
#define Cudd_NotCond(node,c) ((DdNode *)((long)(node) ^ (c)))


/**Macro***********************************************************************

  Synopsis     [Returns the regular version of a pointer.]

  Description  []

  SideEffects  [none]

  SeeAlso      [Cudd_Complement Cudd_IsComplement]

******************************************************************************/
#define Cudd_Regular(node) ((DdNode *)((unsigned long)(node) & ~01))


/**Macro***********************************************************************

  Synopsis     [Returns the complemented version of a pointer.]

  Description  []

  SideEffects  [none]

  SeeAlso      [Cudd_Regular Cudd_IsComplement]

******************************************************************************/
#define Cudd_Complement(node) ((DdNode *)((unsigned long)(node) | 01))


/**Macro***********************************************************************

  Synopsis     [Returns 1 if a pointer is complemented.]

  Description  []

  SideEffects  [none]

  SeeAlso      [Cudd_Regular Cudd_Complement]

******************************************************************************/
#define Cudd_IsComplement(node)	((int) ((long) (node) & 01))


/**Macro***********************************************************************

  Synopsis     [Returns the then child of an internal node.]

  Description  [Returns the then child of an internal node. If
  <code>node</code> is a constant node, the result is unpredictable.]

  SideEffects  [none]

  SeeAlso      [Cudd_E Cudd_V]

******************************************************************************/
#define Cudd_T(node) ((Cudd_Regular(node))->type.kids.T)


/**Macro***********************************************************************

  Synopsis     [Returns the else child of an internal node.]

  Description  [Returns the else child of an internal node. If
  <code>node</code> is a constant node, the result is unpredictable.]

  SideEffects  [none]

  SeeAlso      [Cudd_T Cudd_V]

******************************************************************************/
#define Cudd_E(node) ((Cudd_Regular(node))->type.kids.E)


/**Macro***********************************************************************

  Synopsis     [Returns the value of a constant node.]

  Description  [Returns the value of a constant node. If
  <code>node</code> is an internal node, the result is unpredictable.]

  SideEffects  [none]

  SeeAlso      [Cudd_T Cudd_E]

******************************************************************************/
#define Cudd_V(node) ((Cudd_Regular(node))->type.value)


/**Macro***********************************************************************

  Synopsis     [Returns the current position in the order of variable
  index.]

  Description [Returns the current position in the order of variable
  index. This macro is obsolete and is kept for compatibility. New
  applications should use Cudd_ReadPerm instead.]

  SideEffects  [none]

  SeeAlso      [Cudd_ReadPerm]

******************************************************************************/
#define Cudd_ReadIndex(dd,index) (Cudd_ReadPerm(dd,index))


/**Macro***********************************************************************

  Synopsis     [Iterates over the cubes of a decision diagram.]

  Description  [Iterates over the cubes of a decision diagram f.
  <ul>
  <li> DdManager *manager;
  <li> DdNode *f;
  <li> DdGen *gen;
  <li> int *cube;
  <li> CUDD_VALUE_TYPE value;
  </ul>
  Cudd_ForeachCube allocates and frees the generator. Therefore the
  application should not try to do that. Also, the cube is freed at the
  end of Cudd_ForeachCube and hence is not available outside of the loop.<p>
  CAUTION: It is assumed that dynamic reordering will not occur while
  there are open generators. It is the user's responsibility to make sure
  that dynamic reordering does not occur. As long as new nodes are not created
  during generation, and dynamic reordering is not called explicitly,
  dynamic reordering will not occur. Alternatively, it is sufficient to
  disable dynamic reordering. It is a mistake to dispose of a diagram
  on which generation is ongoing.]

  SideEffects  [none]

  SeeAlso      [Cudd_ForeachNode Cudd_FirstCube Cudd_NextCube Cudd_GenFree
  Cudd_IsGenEmpty Cudd_AutodynDisable]

******************************************************************************/
#define Cudd_ForeachCube(manager, f, gen, cube, value)\
    for((gen) = Cudd_FirstCube(manager, f, &cube, &value);\
	Cudd_IsGenEmpty(gen) ? Cudd_GenFree(gen) : TRUE;\
	(void) Cudd_NextCube(gen, &cube, &value))


/**Macro***********************************************************************

  Synopsis     [Iterates over the nodes of a decision diagram.]

  Description  [Iterates over the nodes of a decision diagram f.
  <ul>
  <li> DdManager *manager;
  <li> DdNode *f;
  <li> DdGen *gen;
  <li> DdNode *node;
  </ul>
  The nodes are returned in a seemingly random order.
  Cudd_ForeachNode allocates and frees the generator. Therefore the
  application should not try to do that.<p>
  CAUTION: It is assumed that dynamic reordering will not occur while
  there are open generators. It is the user's responsibility to make sure
  that dynamic reordering does not occur. As long as new nodes are not created
  during generation, and dynamic reordering is not called explicitly,
  dynamic reordering will not occur. Alternatively, it is sufficient to
  disable dynamic reordering. It is a mistake to dispose of a diagram
  on which generation is ongoing.]

  SideEffects  [none]

  SeeAlso      [Cudd_ForeachCube Cudd_FirstNode Cudd_NextNode Cudd_GenFree
  Cudd_IsGenEmpty Cudd_AutodynDisable]

******************************************************************************/
#define Cudd_ForeachNode(manager, f, gen, node)\
    for((gen) = Cudd_FirstNode(manager, f, &node);\
	Cudd_IsGenEmpty(gen) ? Cudd_GenFree(gen) : TRUE;\
	(void) Cudd_NextNode(gen, &node))


/**Macro***********************************************************************

  Synopsis     [Iterates over the paths of a ZDD.]

  Description  [Iterates over the paths of a ZDD f.
  <ul>
  <li> DdManager *manager;
  <li> DdNode *f;
  <li> DdGen *gen;
  <li> int *path;
  </ul>
  Cudd_zddForeachPath allocates and frees the generator. Therefore the
  application should not try to do that. Also, the path is freed at the
  end of Cudd_zddForeachPath and hence is not available outside of the loop.<p>
  CAUTION: It is assumed that dynamic reordering will not occur while
  there are open generators.  It is the user's responsibility to make sure
  that dynamic reordering does not occur.  As long as new nodes are not created
  during generation, and dynamic reordering is not called explicitly,
  dynamic reordering will not occur.  Alternatively, it is sufficient to
  disable dynamic reordering.  It is a mistake to dispose of a diagram
  on which generation is ongoing.]

  SideEffects  [none]

  SeeAlso      [Cudd_zddFirstPath Cudd_zddNextPath Cudd_GenFree
  Cudd_IsGenEmpty Cudd_AutodynDisable]

******************************************************************************/
#define Cudd_zddForeachPath(manager, f, gen, path)\
    for((gen) = Cudd_zddFirstPath(manager, f, &path);\
	Cudd_IsGenEmpty(gen) ? Cudd_GenFree(gen) : TRUE;\
	(void) Cudd_zddNextPath(gen, &path))


/* These are potential duplicates. */
#ifndef EXTERN
#   ifdef __cplusplus
#	define EXTERN extern "C"
#   else
#	define EXTERN extern
#   endif
#endif
#ifndef ARGS
#   if defined(__STDC__) || defined(__cplusplus)
#	define ARGS(protos)	protos		/* ANSI C */
#   else /* !(__STDC__ || __cplusplus) */
#	define ARGS(protos)	()		/* K&R C */
#   endif /* !(__STDC__ || __cplusplus) */
#endif


/**AutomaticStart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Function prototypes                                                       */
/*---------------------------------------------------------------------------*/

EXTERN DdNode * Cudd_addNewVar ARGS((DdManager *dd));
EXTERN DdNode * Cudd_addNewVarAtLevel ARGS((DdManager *dd, int level));
EXTERN DdNode * Cudd_bddNewVar ARGS((DdManager *dd));
EXTERN DdNode * Cudd_bddNewVarAtLevel ARGS((DdManager *dd, int level));
EXTERN DdNode * Cudd_addIthVar ARGS((DdManager *dd, int i));
EXTERN DdNode * Cudd_bddIthVar ARGS((DdManager *dd, int i));
EXTERN DdNode * Cudd_zddIthVar ARGS((DdManager *dd, int i));
EXTERN int Cudd_zddVarsFromBddVars ARGS((DdManager *dd, int multiplicity));
EXTERN DdNode * Cudd_addConst ARGS((DdManager *dd, CUDD_VALUE_TYPE c));
EXTERN int Cudd_IsNonConstant ARGS((DdNode *f));
EXTERN void Cudd_AutodynEnable ARGS((DdManager *unique, Cudd_ReorderingType method));
EXTERN void Cudd_AutodynDisable ARGS((DdManager *unique));
EXTERN int Cudd_ReorderingStatus ARGS((DdManager *unique, Cudd_ReorderingType *method));
EXTERN void Cudd_AutodynEnableZdd ARGS((DdManager *unique, Cudd_ReorderingType method));
EXTERN void Cudd_AutodynDisableZdd ARGS((DdManager *unique));
EXTERN int Cudd_ReorderingStatusZdd ARGS((DdManager *unique, Cudd_ReorderingType *method));
EXTERN int Cudd_zddRealignmentEnabled ARGS((DdManager *unique));
EXTERN void Cudd_zddRealignEnable ARGS((DdManager *unique));
EXTERN void Cudd_zddRealignDisable ARGS((DdManager *unique));
EXTERN int Cudd_bddRealignmentEnabled ARGS((DdManager *unique));
EXTERN void Cudd_bddRealignEnable ARGS((DdManager *unique));
EXTERN void Cudd_bddRealignDisable ARGS((DdManager *unique));
EXTERN DdNode * Cudd_ReadOne ARGS((DdManager *dd));
EXTERN DdNode * Cudd_ReadZddOne ARGS((DdManager *dd, int i));
EXTERN DdNode * Cudd_ReadZero ARGS((DdManager *dd));
EXTERN DdNode * Cudd_ReadLogicZero ARGS((DdManager *dd));
EXTERN DdNode * Cudd_ReadPlusInfinity ARGS((DdManager *dd));
EXTERN DdNode * Cudd_ReadMinusInfinity ARGS((DdManager *dd));
EXTERN DdNode * Cudd_ReadBackground ARGS((DdManager *dd));
EXTERN void Cudd_SetBackground ARGS((DdManager *dd, DdNode *bck));
EXTERN unsigned int Cudd_ReadCacheSlots ARGS((DdManager *dd));
EXTERN double Cudd_ReadCacheUsedSlots ARGS((DdManager * dd));
EXTERN double Cudd_ReadCacheLookUps ARGS((DdManager *dd));
EXTERN double Cudd_ReadCacheHits ARGS((DdManager *dd));
EXTERN double Cudd_ReadRecursiveCalls ARGS ((DdManager * dd));
EXTERN unsigned int Cudd_ReadMinHit ARGS((DdManager *dd));
EXTERN void Cudd_SetMinHit ARGS((DdManager *dd, unsigned int hr));
EXTERN unsigned int Cudd_ReadLooseUpTo ARGS((DdManager *dd));
EXTERN void Cudd_SetLooseUpTo ARGS((DdManager *dd, unsigned int lut));
EXTERN unsigned int Cudd_ReadMaxCache ARGS((DdManager *dd));
EXTERN unsigned int Cudd_ReadMaxCacheHard ARGS((DdManager *dd));
EXTERN void Cudd_SetMaxCacheHard ARGS((DdManager *dd, unsigned int mc));
EXTERN int Cudd_ReadSize ARGS((DdManager *dd));
EXTERN int Cudd_ReadZddSize ARGS((DdManager *dd));
EXTERN unsigned int Cudd_ReadSlots ARGS((DdManager *dd));
EXTERN double Cudd_ReadUsedSlots ARGS((DdManager * dd));
EXTERN double Cudd_ExpectedUsedSlots ARGS((DdManager * dd));
EXTERN unsigned int Cudd_ReadKeys ARGS((DdManager *dd));
EXTERN unsigned int Cudd_ReadDead ARGS((DdManager *dd));
EXTERN unsigned int Cudd_ReadMinDead ARGS((DdManager *dd));
EXTERN int Cudd_ReadReorderings ARGS((DdManager *dd));
EXTERN long Cudd_ReadReorderingTime ARGS((DdManager * dd));
EXTERN int Cudd_ReadGarbageCollections ARGS((DdManager * dd));
EXTERN long Cudd_ReadGarbageCollectionTime ARGS((DdManager * dd));
EXTERN double Cudd_ReadNodesFreed ARGS((DdManager * dd));
EXTERN double Cudd_ReadNodesDropped ARGS((DdManager * dd));
EXTERN double Cudd_ReadUniqueLookUps ARGS((DdManager * dd));
EXTERN double Cudd_ReadUniqueLinks ARGS((DdManager * dd));
EXTERN int Cudd_ReadSiftMaxVar ARGS((DdManager *dd));
EXTERN void Cudd_SetSiftMaxVar ARGS((DdManager *dd, int smv));
EXTERN int Cudd_ReadSiftMaxSwap ARGS((DdManager *dd));
EXTERN void Cudd_SetSiftMaxSwap ARGS((DdManager *dd, int sms));
EXTERN double Cudd_ReadMaxGrowth ARGS((DdManager *dd));
EXTERN void Cudd_SetMaxGrowth ARGS((DdManager *dd, double mg));
EXTERN double Cudd_ReadMaxGrowthAlternate ARGS((DdManager * dd));
EXTERN void Cudd_SetMaxGrowthAlternate ARGS((DdManager * dd, double mg));
EXTERN int Cudd_ReadReorderingCycle ARGS((DdManager * dd));
EXTERN void Cudd_SetReorderingCycle ARGS((DdManager * dd, int cycle));
EXTERN MtrNode * Cudd_ReadTree ARGS((DdManager *dd));
EXTERN void Cudd_SetTree ARGS((DdManager *dd, MtrNode *tree));
EXTERN void Cudd_FreeTree ARGS((DdManager *dd));
EXTERN MtrNode * Cudd_ReadZddTree ARGS((DdManager *dd));
EXTERN void Cudd_SetZddTree ARGS((DdManager *dd, MtrNode *tree));
EXTERN void Cudd_FreeZddTree ARGS((DdManager *dd));
EXTERN unsigned int Cudd_NodeReadIndex ARGS((DdNode *node));
EXTERN int Cudd_ReadPerm ARGS((DdManager *dd, int i));
EXTERN int Cudd_ReadPermZdd ARGS((DdManager *dd, int i));
EXTERN int Cudd_ReadInvPerm ARGS((DdManager *dd, int i));
EXTERN int Cudd_ReadInvPermZdd ARGS((DdManager *dd, int i));
EXTERN DdNode * Cudd_ReadVars ARGS((DdManager *dd, int i));
EXTERN CUDD_VALUE_TYPE Cudd_ReadEpsilon ARGS((DdManager *dd));
EXTERN void Cudd_SetEpsilon ARGS((DdManager *dd, CUDD_VALUE_TYPE ep));
EXTERN Cudd_AggregationType Cudd_ReadGroupcheck ARGS((DdManager *dd));
EXTERN void Cudd_SetGroupcheck ARGS((DdManager *dd, Cudd_AggregationType gc));
EXTERN int Cudd_GarbageCollectionEnabled ARGS((DdManager *dd));
EXTERN void Cudd_EnableGarbageCollection ARGS((DdManager *dd));
EXTERN void Cudd_DisableGarbageCollection ARGS((DdManager *dd));
EXTERN int Cudd_DeadAreCounted ARGS((DdManager *dd));
EXTERN void Cudd_TurnOnCountDead ARGS((DdManager *dd));
EXTERN void Cudd_TurnOffCountDead ARGS((DdManager *dd));
EXTERN int Cudd_ReadRecomb ARGS((DdManager *dd));
EXTERN void Cudd_SetRecomb ARGS((DdManager *dd, int recomb));
EXTERN int Cudd_ReadSymmviolation ARGS((DdManager *dd));
EXTERN void Cudd_SetSymmviolation ARGS((DdManager *dd, int symmviolation));
EXTERN int Cudd_ReadArcviolation ARGS((DdManager *dd));
EXTERN void Cudd_SetArcviolation ARGS((DdManager *dd, int arcviolation));
EXTERN int Cudd_ReadPopulationSize ARGS((DdManager *dd));
EXTERN void Cudd_SetPopulationSize ARGS((DdManager *dd, int populationSize));
EXTERN int Cudd_ReadNumberXovers ARGS((DdManager *dd));
EXTERN void Cudd_SetNumberXovers ARGS((DdManager *dd, int numberXovers));
EXTERN long Cudd_ReadMemoryInUse ARGS((DdManager *dd));
EXTERN int Cudd_PrintInfo ARGS((DdManager *dd, FILE *fp));
EXTERN long Cudd_ReadPeakNodeCount ARGS((DdManager *dd));
EXTERN int Cudd_ReadPeakLiveNodeCount ARGS((DdManager * dd));
EXTERN long Cudd_ReadNodeCount ARGS((DdManager *dd));
EXTERN long Cudd_zddReadNodeCount ARGS((DdManager *dd));
EXTERN int Cudd_AddHook ARGS((DdManager *dd, int (*f)(DdManager *, char *, void *), Cudd_HookType where));
EXTERN int Cudd_RemoveHook ARGS((DdManager *dd, int (*f)(DdManager *, char *, void *), Cudd_HookType where));
EXTERN int Cudd_IsInHook ARGS((DdManager * dd, int (*f)(DdManager *, char *, void *), Cudd_HookType where));
EXTERN int Cudd_StdPreReordHook ARGS((DdManager *dd, char *str, void *data));
EXTERN int Cudd_StdPostReordHook ARGS((DdManager *dd, char *str, void *data));
EXTERN int Cudd_EnableReorderingReporting ARGS((DdManager *dd));
EXTERN int Cudd_DisableReorderingReporting ARGS((DdManager *dd));
EXTERN int Cudd_ReorderingReporting ARGS((DdManager *dd));
EXTERN Cudd_ErrorType Cudd_ReadErrorCode ARGS((DdManager *dd));
EXTERN void Cudd_ClearErrorCode ARGS((DdManager *dd));
EXTERN FILE * Cudd_ReadStdout ARGS((DdManager *dd));
EXTERN void Cudd_SetStdout ARGS((DdManager *dd, FILE *fp));
EXTERN FILE * Cudd_ReadStderr ARGS((DdManager *dd));
EXTERN void Cudd_SetStderr ARGS((DdManager *dd, FILE *fp));
EXTERN unsigned int Cudd_ReadNextReordering ARGS((DdManager *dd));
EXTERN void Cudd_SetNextReordering ARGS((DdManager *dd, unsigned int next));
EXTERN double Cudd_ReadSwapSteps ARGS((DdManager *dd));
EXTERN unsigned int Cudd_ReadMaxLive ARGS((DdManager *dd));
EXTERN void Cudd_SetMaxLive ARGS((DdManager *dd, unsigned int maxLive));
EXTERN long Cudd_ReadMaxMemory ARGS((DdManager *dd));
EXTERN void Cudd_SetMaxMemory ARGS((DdManager *dd, long maxMemory));
EXTERN int Cudd_bddBindVar ARGS((DdManager *dd, int index));
EXTERN int Cudd_bddUnbindVar ARGS((DdManager *dd, int index));
EXTERN int Cudd_bddVarIsBound ARGS((DdManager *dd, int index));
EXTERN DdNode * Cudd_addExistAbstract ARGS((DdManager *manager, DdNode *f, DdNode *cube));
EXTERN DdNode * Cudd_addUnivAbstract ARGS((DdManager *manager, DdNode *f, DdNode *cube));
EXTERN DdNode * Cudd_addOrAbstract ARGS((DdManager *manager, DdNode *f, DdNode *cube));
EXTERN DdNode * Cudd_addApply ARGS((DdManager *dd, DdNode * (*)(DdManager *, DdNode **, DdNode **), DdNode *f, DdNode *g));
EXTERN DdNode * Cudd_addPlus ARGS((DdManager *dd, DdNode **f, DdNode **g));
EXTERN DdNode * Cudd_addTimes ARGS((DdManager *dd, DdNode **f, DdNode **g));
EXTERN DdNode * Cudd_addThreshold ARGS((DdManager *dd, DdNode **f, DdNode **g));
EXTERN DdNode * Cudd_addSetNZ ARGS((DdManager *dd, DdNode **f, DdNode **g));
EXTERN DdNode * Cudd_addDivide ARGS((DdManager *dd, DdNode **f, DdNode **g));
EXTERN DdNode * Cudd_addMinus ARGS((DdManager *dd, DdNode **f, DdNode **g));
EXTERN DdNode * Cudd_addMinimum ARGS((DdManager *dd, DdNode **f, DdNode **g));
EXTERN DdNode * Cudd_addMaximum ARGS((DdManager *dd, DdNode **f, DdNode **g));
EXTERN DdNode * Cudd_addOneZeroMaximum ARGS((DdManager *dd, DdNode **f, DdNode **g));
EXTERN DdNode * Cudd_addDiff ARGS((DdManager *dd, DdNode **f, DdNode **g));
EXTERN DdNode * Cudd_addAgreement ARGS((DdManager *dd, DdNode **f, DdNode **g));
EXTERN DdNode * Cudd_addOr ARGS((DdManager *dd, DdNode **f, DdNode **g));
EXTERN DdNode * Cudd_addNand ARGS((DdManager *dd, DdNode **f, DdNode **g));
EXTERN DdNode * Cudd_addNor ARGS((DdManager *dd, DdNode **f, DdNode **g));
EXTERN DdNode * Cudd_addXor ARGS((DdManager *dd, DdNode **f, DdNode **g));
EXTERN DdNode * Cudd_addXnor ARGS((DdManager *dd, DdNode **f, DdNode **g));
EXTERN DdNode * Cudd_addMonadicApply ARGS((DdManager * dd, DdNode * (*op)(DdManager *, DdNode *), DdNode * f));
EXTERN DdNode * Cudd_addLog ARGS((DdManager * dd, DdNode * f));
EXTERN DdNode * Cudd_addFindMax ARGS((DdManager *dd, DdNode *f));
EXTERN DdNode * Cudd_addFindMin ARGS((DdManager *dd, DdNode *f));
EXTERN DdNode * Cudd_addIthBit ARGS((DdManager *dd, DdNode *f, int bit));
EXTERN DdNode * Cudd_addScalarInverse ARGS((DdManager *dd, DdNode *f, DdNode *epsilon));
EXTERN DdNode * Cudd_addIte ARGS((DdManager *dd, DdNode *f, DdNode *g, DdNode *h));
EXTERN DdNode * Cudd_addIteConstant ARGS((DdManager *dd, DdNode *f, DdNode *g, DdNode *h));
EXTERN DdNode * Cudd_addEvalConst ARGS((DdManager *dd, DdNode *f, DdNode *g));
EXTERN int Cudd_addLeq ARGS((DdManager * dd, DdNode * f, DdNode * g));
EXTERN DdNode * Cudd_addCmpl ARGS((DdManager *dd, DdNode *f));
EXTERN DdNode * Cudd_addNegate ARGS((DdManager *dd, DdNode *f));
EXTERN DdNode * Cudd_addRoundOff ARGS((DdManager *dd, DdNode *f, int N));
EXTERN DdNode * Cudd_addWalsh ARGS((DdManager *dd, DdNode **x, DdNode **y, int n));
EXTERN DdNode * Cudd_addResidue ARGS((DdManager *dd, int n, int m, int options, int top));
EXTERN DdNode * Cudd_bddAndAbstract ARGS((DdManager *manager, DdNode *f, DdNode *g, DdNode *cube));
EXTERN int Cudd_ApaNumberOfDigits ARGS((int binaryDigits));
EXTERN DdApaNumber Cudd_NewApaNumber ARGS((int digits));
EXTERN void Cudd_ApaCopy ARGS((int digits, DdApaNumber source, DdApaNumber dest));
EXTERN DdApaDigit Cudd_ApaAdd ARGS((int digits, DdApaNumber a, DdApaNumber b, DdApaNumber sum));
EXTERN DdApaDigit Cudd_ApaSubtract ARGS((int digits, DdApaNumber a, DdApaNumber b, DdApaNumber diff));
EXTERN DdApaDigit Cudd_ApaShortDivision ARGS((int digits, DdApaNumber dividend, DdApaDigit divisor, DdApaNumber quotient));
EXTERN unsigned int Cudd_ApaIntDivision ARGS((int  digits, DdApaNumber dividend, unsigned int  divisor, DdApaNumber  quotient));
EXTERN void Cudd_ApaShiftRight ARGS((int digits, DdApaDigit in, DdApaNumber a, DdApaNumber b));
EXTERN void Cudd_ApaSetToLiteral ARGS((int digits, DdApaNumber number, DdApaDigit literal));
EXTERN void Cudd_ApaPowerOfTwo ARGS((int digits, DdApaNumber number, int power));
EXTERN int Cudd_ApaCompare ARGS((int digitsFirst, DdApaNumber  first, int digitsSecond, DdApaNumber  second));
EXTERN int Cudd_ApaCompareRatios ARGS ((int digitsFirst, DdApaNumber firstNum, unsigned int firstDen, int digitsSecond, DdApaNumber secondNum, unsigned int secondDen));
EXTERN int Cudd_ApaPrintHex ARGS((FILE *fp, int digits, DdApaNumber number));
EXTERN int Cudd_ApaPrintDecimal ARGS((FILE *fp, int digits, DdApaNumber number));
EXTERN int Cudd_ApaPrintExponential ARGS((FILE * fp, int  digits, DdApaNumber  number, int precision));
EXTERN DdApaNumber Cudd_ApaCountMinterm ARGS((DdManager *manager, DdNode *node, int nvars, int *digits));
EXTERN int Cudd_ApaPrintMinterm ARGS((FILE *fp, DdManager *dd, DdNode *node, int nvars));
EXTERN int Cudd_ApaPrintMintermExp ARGS((FILE * fp, DdManager * dd, DdNode * node, int  nvars, int precision));
EXTERN int Cudd_ApaPrintDensity ARGS((FILE * fp, DdManager * dd, DdNode * node, int  nvars));
EXTERN DdNode * Cudd_UnderApprox ARGS((DdManager *dd, DdNode *f, int numVars, int threshold, int safe, double quality));
EXTERN DdNode * Cudd_OverApprox ARGS((DdManager *dd, DdNode *f, int numVars, int threshold, int safe, double quality));
EXTERN DdNode * Cudd_RemapUnderApprox ARGS((DdManager *dd, DdNode *f, int numVars, int threshold, double quality));
EXTERN DdNode * Cudd_RemapOverApprox ARGS((DdManager *dd, DdNode *f, int numVars, int threshold, double quality));
EXTERN DdNode * Cudd_BiasedUnderApprox ARGS((DdManager *dd, DdNode *f, DdNode *b, int numVars, int threshold, double quality1, double quality0));
EXTERN DdNode * Cudd_BiasedOverApprox ARGS((DdManager *dd, DdNode *f, DdNode *b, int numVars, int threshold, double quality1, double quality0));
EXTERN DdNode * Cudd_bddExistAbstract ARGS((DdManager *manager, DdNode *f, DdNode *cube));
EXTERN DdNode * Cudd_bddXorExistAbstract ARGS((DdManager *manager, DdNode *f, DdNode *g, DdNode *cube));
EXTERN DdNode * Cudd_bddUnivAbstract ARGS((DdManager *manager, DdNode *f, DdNode *cube));
EXTERN DdNode * Cudd_bddBooleanDiff ARGS((DdManager *manager, DdNode *f, int x));
EXTERN int Cudd_bddVarIsDependent ARGS((DdManager *dd, DdNode *f, DdNode *var));
EXTERN double Cudd_bddCorrelation ARGS((DdManager *manager, DdNode *f, DdNode *g));
EXTERN double Cudd_bddCorrelationWeights ARGS((DdManager *manager, DdNode *f, DdNode *g, double *prob));
EXTERN DdNode * Cudd_bddIte ARGS((DdManager *dd, DdNode *f, DdNode *g, DdNode *h));
EXTERN DdNode * Cudd_bddIteConstant ARGS((DdManager *dd, DdNode *f, DdNode *g, DdNode *h));
EXTERN DdNode * Cudd_bddIntersect ARGS((DdManager *dd, DdNode *f, DdNode *g));
EXTERN DdNode * Cudd_bddAnd ARGS((DdManager *dd, DdNode *f, DdNode *g));
EXTERN DdNode * Cudd_bddOr ARGS((DdManager *dd, DdNode *f, DdNode *g));
EXTERN DdNode * Cudd_bddNand ARGS((DdManager *dd, DdNode *f, DdNode *g));
EXTERN DdNode * Cudd_bddNor ARGS((DdManager *dd, DdNode *f, DdNode *g));
EXTERN DdNode * Cudd_bddXor ARGS((DdManager *dd, DdNode *f, DdNode *g));
EXTERN DdNode * Cudd_bddXnor ARGS((DdManager *dd, DdNode *f, DdNode *g));
EXTERN int Cudd_bddLeq ARGS((DdManager *dd, DdNode *f, DdNode *g));
EXTERN DdNode * Cudd_addBddThreshold ARGS((DdManager *dd, DdNode *f, CUDD_VALUE_TYPE value));
EXTERN DdNode * Cudd_addBddStrictThreshold ARGS((DdManager *dd, DdNode *f, CUDD_VALUE_TYPE value));
EXTERN DdNode * Cudd_addBddInterval ARGS((DdManager *dd, DdNode *f, CUDD_VALUE_TYPE lower, CUDD_VALUE_TYPE upper));
EXTERN DdNode * Cudd_addBddIthBit ARGS((DdManager *dd, DdNode *f, int bit));
EXTERN DdNode * Cudd_BddToAdd ARGS((DdManager *dd, DdNode *B));
EXTERN DdNode * Cudd_addBddPattern ARGS((DdManager *dd, DdNode *f));
EXTERN DdNode * Cudd_bddTransfer ARGS((DdManager *ddSource, DdManager *ddDestination, DdNode *f));
EXTERN int Cudd_DebugCheck ARGS((DdManager *table));
EXTERN int Cudd_CheckKeys ARGS((DdManager *table));
EXTERN DdNode * Cudd_bddClippingAnd ARGS((DdManager *dd, DdNode *f, DdNode *g, int maxDepth, int direction));
EXTERN DdNode * Cudd_bddClippingAndAbstract ARGS((DdManager *dd, DdNode *f, DdNode *g, DdNode *cube, int maxDepth, int direction));
EXTERN DdNode * Cudd_Cofactor ARGS((DdManager *dd, DdNode *f, DdNode *g));
EXTERN DdNode * Cudd_bddCompose ARGS((DdManager *dd, DdNode *f, DdNode *g, int v));
EXTERN DdNode * Cudd_addCompose ARGS((DdManager *dd, DdNode *f, DdNode *g, int v));
EXTERN DdNode * Cudd_addPermute ARGS((DdManager *manager, DdNode *node, int *permut));
EXTERN DdNode * Cudd_addSwapVariables ARGS((DdManager *dd, DdNode *f, DdNode **x, DdNode **y, int n));
EXTERN DdNode * Cudd_bddPermute ARGS((DdManager *manager, DdNode *node, int *permut));
EXTERN DdNode * Cudd_bddVarMap ARGS((DdManager *manager, DdNode *f));
EXTERN int Cudd_SetVarMap ARGS((DdManager *manager, DdNode **x, DdNode **y, int n));
EXTERN DdNode * Cudd_bddSwapVariables ARGS((DdManager *dd, DdNode *f, DdNode **x, DdNode **y, int n));
EXTERN DdNode * Cudd_bddAdjPermuteX ARGS((DdManager *dd, DdNode *B, DdNode **x, int n));
EXTERN DdNode * Cudd_addVectorCompose ARGS((DdManager *dd, DdNode *f, DdNode **vector));
EXTERN DdNode * Cudd_addGeneralVectorCompose ARGS((DdManager *dd, DdNode *f, DdNode **vectorOn, DdNode **vectorOff));
EXTERN DdNode * Cudd_addNonSimCompose ARGS((DdManager *dd, DdNode *f, DdNode **vector));
EXTERN DdNode * Cudd_bddVectorCompose ARGS((DdManager *dd, DdNode *f, DdNode **vector));
EXTERN int Cudd_bddApproxConjDecomp ARGS((DdManager *dd, DdNode *f, DdNode ***conjuncts));
EXTERN int Cudd_bddApproxDisjDecomp ARGS((DdManager *dd, DdNode *f, DdNode ***disjuncts));
EXTERN int Cudd_bddIterConjDecomp ARGS((DdManager *dd, DdNode *f, DdNode ***conjuncts));
EXTERN int Cudd_bddIterDisjDecomp ARGS((DdManager *dd, DdNode *f, DdNode ***disjuncts));
EXTERN int Cudd_bddGenConjDecomp ARGS((DdManager *dd, DdNode *f, DdNode ***conjuncts));
EXTERN int Cudd_bddGenDisjDecomp ARGS((DdManager *dd, DdNode *f, DdNode ***disjuncts));
EXTERN int Cudd_bddVarConjDecomp ARGS((DdManager *dd, DdNode * f, DdNode ***conjuncts));
EXTERN int Cudd_bddVarDisjDecomp ARGS((DdManager *dd, DdNode * f, DdNode ***disjuncts));
EXTERN DdNode * Cudd_FindEssential ARGS((DdManager *dd, DdNode *f));
EXTERN int Cudd_bddIsVarEssential ARGS((DdManager *manager, DdNode *f, int id, int phase));
EXTERN int Cudd_DumpBlif ARGS((DdManager *dd, int n, DdNode **f, char **inames, char **onames, char *mname, FILE *fp));
EXTERN int Cudd_DumpBlifBody ARGS((DdManager *dd, int n, DdNode **f, char **inames, char **onames, FILE *fp));
EXTERN int Cudd_DumpDot ARGS((DdManager *dd, int n, DdNode **f, char **inames, char **onames, FILE *fp));
EXTERN int Cudd_DumpDaVinci ARGS((DdManager *dd, int n, DdNode **f, char **inames, char **onames, FILE *fp));
EXTERN int Cudd_DumpDDcal ARGS((DdManager *dd, int n, DdNode **f, char **inames, char **onames, FILE *fp));
EXTERN int Cudd_DumpFactoredForm ARGS((DdManager *dd, int n, DdNode **f, char **inames, char **onames, FILE *fp));
EXTERN DdNode * Cudd_bddConstrain ARGS((DdManager *dd, DdNode *f, DdNode *c));
EXTERN DdNode * Cudd_bddRestrict ARGS((DdManager *dd, DdNode *f, DdNode *c));
EXTERN DdNode * Cudd_addConstrain ARGS((DdManager *dd, DdNode *f, DdNode *c));
EXTERN DdNode ** Cudd_bddConstrainDecomp ARGS((DdManager *dd, DdNode *f));
EXTERN DdNode * Cudd_addRestrict ARGS((DdManager *dd, DdNode *f, DdNode *c));
EXTERN DdNode ** Cudd_bddCharToVect ARGS((DdManager *dd, DdNode *f));
EXTERN DdNode * Cudd_bddLICompaction ARGS((DdManager *dd, DdNode *f, DdNode *c));
EXTERN DdNode * Cudd_bddSqueeze ARGS((DdManager *dd, DdNode *l, DdNode *u));
EXTERN DdNode * Cudd_bddMinimize ARGS((DdManager *dd, DdNode *f, DdNode *c));
EXTERN DdNode * Cudd_SubsetCompress ARGS((DdManager *dd, DdNode *f, int nvars, int threshold));
EXTERN DdNode * Cudd_SupersetCompress ARGS((DdManager *dd, DdNode *f, int nvars, int threshold));
EXTERN MtrNode * Cudd_MakeTreeNode ARGS((DdManager *dd, unsigned int low, unsigned int size, unsigned int type));
EXTERN int Cudd_addHarwell ARGS((FILE *fp, DdManager *dd, DdNode **E, DdNode ***x, DdNode ***y, DdNode ***xn, DdNode ***yn_, int *nx, int *ny, int *m, int *n, int bx, int sx, int by, int sy, int pr));
EXTERN DdManager * Cudd_Init ARGS((unsigned int numVars, unsigned int numVarsZ, unsigned int numSlots, unsigned int cacheSize, unsigned long maxMemory));
EXTERN void Cudd_Quit ARGS((DdManager *unique));
EXTERN int Cudd_PrintLinear ARGS((DdManager *table));
EXTERN int Cudd_ReadLinear ARGS((DdManager *table, int x, int y));
EXTERN DdNode * Cudd_bddLiteralSetIntersection ARGS((DdManager *dd, DdNode *f, DdNode *g));
EXTERN DdNode * Cudd_addMatrixMultiply ARGS((DdManager *dd, DdNode *A, DdNode *B, DdNode **z, int nz));
EXTERN DdNode * Cudd_addTimesPlus ARGS((DdManager *dd, DdNode *A, DdNode *B, DdNode **z, int nz));
EXTERN DdNode * Cudd_addTriangle ARGS((DdManager *dd, DdNode *f, DdNode *g, DdNode **z, int nz));
EXTERN DdNode * Cudd_addOuterSum ARGS((DdManager *dd, DdNode *M, DdNode *r, DdNode *c));
EXTERN DdNode * Cudd_PrioritySelect ARGS((DdManager *dd, DdNode *R, DdNode **x, DdNode **y, DdNode **z, DdNode *Pi, int n, DdNode * (*)(DdManager *, int, DdNode **, DdNode **, DdNode **)));
EXTERN DdNode * Cudd_Xgty ARGS((DdManager *dd, int N, DdNode **z, DdNode **x, DdNode **y));
EXTERN DdNode * Cudd_Xeqy ARGS((DdManager *dd, int N, DdNode **x, DdNode **y));
EXTERN DdNode * Cudd_addXeqy ARGS((DdManager *dd, int N, DdNode **x, DdNode **y));
EXTERN DdNode * Cudd_Dxygtdxz ARGS((DdManager *dd, int N, DdNode **x, DdNode **y, DdNode **z));
EXTERN DdNode * Cudd_Dxygtdyz ARGS((DdManager *dd, int N, DdNode **x, DdNode **y, DdNode **z));
EXTERN DdNode * Cudd_CProjection ARGS((DdManager *dd, DdNode *R, DdNode *Y));
EXTERN DdNode * Cudd_addHamming ARGS((DdManager *dd, DdNode **xVars, DdNode **yVars, int nVars));
EXTERN int Cudd_MinHammingDist ARGS((DdManager *dd, DdNode *f, int *minterm, int upperBound));
EXTERN DdNode * Cudd_bddClosestCube ARGS((DdManager *dd, DdNode * f, DdNode *g, int *distance));
EXTERN int Cudd_addRead ARGS((FILE *fp, DdManager *dd, DdNode **E, DdNode ***x, DdNode ***y, DdNode ***xn, DdNode ***yn_, int *nx, int *ny, int *m, int *n, int bx, int sx, int by, int sy));
EXTERN int Cudd_bddRead ARGS((FILE *fp, DdManager *dd, DdNode **E, DdNode ***x, DdNode ***y, int *nx, int *ny, int *m, int *n, int bx, int sx, int by, int sy));
EXTERN void Cudd_Ref ARGS((DdNode *n));
EXTERN void Cudd_RecursiveDeref ARGS((DdManager *table, DdNode *n));
EXTERN void Cudd_IterDerefBdd ARGS((DdManager *table, DdNode *n));
EXTERN void Cudd_DelayedDerefBdd ARGS((DdManager * table, DdNode * n));
EXTERN void Cudd_RecursiveDerefZdd ARGS((DdManager *table, DdNode *n));
EXTERN void Cudd_Deref ARGS((DdNode *node));
EXTERN int Cudd_CheckZeroRef ARGS((DdManager *manager));
EXTERN int Cudd_ReduceHeap ARGS((DdManager *table, Cudd_ReorderingType heuristic, int minsize));
EXTERN int Cudd_ShuffleHeap ARGS((DdManager *table, int *permutation));
EXTERN DdNode * Cudd_Eval ARGS((DdManager *dd, DdNode *f, int *inputs));
EXTERN DdNode * Cudd_ShortestPath ARGS((DdManager *manager, DdNode *f, int *weight, int *support, int *length));
EXTERN DdNode * Cudd_LargestCube ARGS((DdManager *manager, DdNode *f, int *length));
EXTERN int Cudd_ShortestLength ARGS((DdManager *manager, DdNode *f, int *weight));
EXTERN DdNode * Cudd_Decreasing ARGS((DdManager *dd, DdNode *f, int i));
EXTERN DdNode * Cudd_Increasing ARGS((DdManager *dd, DdNode *f, int i));
EXTERN int Cudd_EquivDC ARGS((DdManager *dd, DdNode *F, DdNode *G, DdNode *D));
EXTERN int Cudd_bddLeqUnless ARGS((DdManager *dd, DdNode *f, DdNode *g, DdNode *D));
EXTERN int Cudd_EqualSupNorm ARGS((DdManager *dd, DdNode *f, DdNode *g, CUDD_VALUE_TYPE tolerance, int pr));
EXTERN DdNode * Cudd_bddMakePrime ARGS ((DdManager *dd, DdNode *cube, DdNode *f));
EXTERN double * Cudd_CofMinterm ARGS((DdManager *dd, DdNode *node));
EXTERN DdNode * Cudd_SolveEqn ARGS((DdManager * bdd, DdNode *F, DdNode *Y, DdNode **G, int **yIndex, int n));
EXTERN DdNode * Cudd_VerifySol ARGS((DdManager * bdd, DdNode *F, DdNode **G, int *yIndex, int n));
EXTERN DdNode * Cudd_SplitSet ARGS((DdManager *manager, DdNode *S, DdNode **xVars, int n, double m));
EXTERN DdNode * Cudd_SubsetHeavyBranch ARGS((DdManager *dd, DdNode *f, int numVars, int threshold));
EXTERN DdNode * Cudd_SupersetHeavyBranch ARGS((DdManager *dd, DdNode *f, int numVars, int threshold));
EXTERN DdNode * Cudd_SubsetShortPaths ARGS((DdManager *dd, DdNode *f, int numVars, int threshold, int hardlimit));
EXTERN DdNode * Cudd_SupersetShortPaths ARGS((DdManager *dd, DdNode *f, int numVars, int threshold, int hardlimit));
EXTERN void Cudd_SymmProfile ARGS((DdManager *table, int lower, int upper));
EXTERN unsigned int Cudd_Prime ARGS((unsigned int p));
EXTERN int Cudd_PrintMinterm ARGS((DdManager *manager, DdNode *node));
EXTERN int Cudd_bddPrintCover ARGS((DdManager *dd, DdNode *l, DdNode *u));
EXTERN int Cudd_PrintDebug ARGS((DdManager *dd, DdNode *f, int n, int pr));
EXTERN int Cudd_DagSize ARGS((DdNode *node));
EXTERN int Cudd_EstimateCofactor ARGS((DdManager *dd, DdNode * node, int i, int phase));
EXTERN int Cudd_EstimateCofactorSimple ARGS((DdNode * node, int i));
EXTERN int Cudd_SharingSize ARGS((DdNode **nodeArray, int n));
EXTERN double Cudd_CountMinterm ARGS((DdManager *manager, DdNode *node, int nvars));
EXTERN int Cudd_EpdCountMinterm ARGS((DdManager *manager, DdNode *node, int nvars, EpDouble *epd));
EXTERN double Cudd_CountPath ARGS((DdNode *node));
EXTERN double Cudd_CountPathsToNonZero ARGS((DdNode *node));
EXTERN DdNode * Cudd_Support ARGS((DdManager *dd, DdNode *f));
EXTERN int * Cudd_SupportIndex ARGS((DdManager *dd, DdNode *f));
EXTERN int Cudd_SupportSize ARGS((DdManager *dd, DdNode *f));
EXTERN DdNode * Cudd_VectorSupport ARGS((DdManager *dd, DdNode **F, int n));
EXTERN int * Cudd_VectorSupportIndex ARGS((DdManager *dd, DdNode **F, int n));
EXTERN int Cudd_VectorSupportSize ARGS((DdManager *dd, DdNode **F, int n));
EXTERN int Cudd_ClassifySupport ARGS((DdManager *dd, DdNode *f, DdNode *g, DdNode **common, DdNode **onlyF, DdNode **onlyG));
EXTERN int Cudd_CountLeaves ARGS((DdNode *node));
EXTERN int Cudd_bddPickOneCube ARGS((DdManager *ddm, DdNode *node, char *string));
EXTERN DdNode * Cudd_bddPickOneMinterm ARGS((DdManager *dd, DdNode *f, DdNode **vars, int n));
EXTERN DdNode ** Cudd_bddPickArbitraryMinterms ARGS((DdManager *dd, DdNode *f, DdNode **vars, int n, int k));
EXTERN DdNode * Cudd_SubsetWithMaskVars ARGS((DdManager *dd, DdNode *f, DdNode **vars, int nvars, DdNode **maskVars, int mvars));
EXTERN DdGen * Cudd_FirstCube ARGS((DdManager *dd, DdNode *f, int **cube, CUDD_VALUE_TYPE *value));
EXTERN int Cudd_NextCube ARGS((DdGen *gen, int **cube, CUDD_VALUE_TYPE *value));
EXTERN DdNode * Cudd_bddComputeCube ARGS((DdManager *dd, DdNode **vars, int *phase, int n));
EXTERN DdNode * Cudd_addComputeCube ARGS((DdManager *dd, DdNode **vars, int *phase, int n));
EXTERN DdNode * Cudd_CubeArrayToBdd ARGS((DdManager *dd, int *array));
EXTERN int Cudd_BddToCubeArray ARGS((DdManager *dd, DdNode *cube, int *array));
EXTERN DdGen * Cudd_FirstNode ARGS((DdManager *dd, DdNode *f, DdNode **node));
EXTERN int Cudd_NextNode ARGS((DdGen *gen, DdNode **node));
EXTERN int Cudd_GenFree ARGS((DdGen *gen));
EXTERN int Cudd_IsGenEmpty ARGS((DdGen *gen));
EXTERN DdNode * Cudd_IndicesToCube ARGS((DdManager *dd, int *array, int n));
EXTERN void Cudd_PrintVersion ARGS((FILE *fp));
EXTERN double Cudd_AverageDistance ARGS((DdManager *dd));
EXTERN long Cudd_Random ARGS(());
EXTERN void Cudd_Srandom ARGS((long seed));
EXTERN double Cudd_Density ARGS((DdManager *dd, DdNode *f, int nvars));
EXTERN void Cudd_OutOfMem ARGS((long size));
EXTERN int Cudd_zddCount ARGS((DdManager *zdd, DdNode *P));
EXTERN double Cudd_zddCountDouble ARGS((DdManager *zdd, DdNode *P));
EXTERN DdNode	* Cudd_zddProduct ARGS((DdManager *dd, DdNode *f, DdNode *g));
EXTERN DdNode	* Cudd_zddUnateProduct ARGS((DdManager *dd, DdNode *f, DdNode *g));
EXTERN DdNode	* Cudd_zddWeakDiv ARGS((DdManager *dd, DdNode *f, DdNode *g));
EXTERN DdNode	* Cudd_zddDivide ARGS((DdManager *dd, DdNode *f, DdNode *g));
EXTERN DdNode	* Cudd_zddWeakDivF ARGS((DdManager *dd, DdNode *f, DdNode *g));
EXTERN DdNode	* Cudd_zddDivideF ARGS((DdManager *dd, DdNode *f, DdNode *g));
EXTERN DdNode	* Cudd_zddComplement ARGS((DdManager *dd, DdNode *node));
EXTERN MtrNode * Cudd_MakeZddTreeNode ARGS((DdManager *dd, unsigned int low, unsigned int size, unsigned int type));
EXTERN DdNode	* Cudd_zddIsop ARGS((DdManager *dd, DdNode *L, DdNode *U, DdNode **zdd_I));
EXTERN DdNode	* Cudd_bddIsop ARGS((DdManager *dd, DdNode *L, DdNode *U));
EXTERN DdNode	* Cudd_MakeBddFromZddCover ARGS((DdManager *dd, DdNode *node));
EXTERN int Cudd_zddDagSize ARGS((DdNode *p_node));
EXTERN double Cudd_zddCountMinterm ARGS((DdManager *zdd, DdNode *node, int path));
EXTERN void Cudd_zddPrintSubtable ARGS((DdManager *table));
EXTERN DdNode * Cudd_zddPortFromBdd ARGS((DdManager *dd, DdNode *B));
EXTERN DdNode * Cudd_zddPortToBdd ARGS((DdManager *dd, DdNode *f));
EXTERN int Cudd_zddReduceHeap ARGS((DdManager *table, Cudd_ReorderingType heuristic, int minsize));
EXTERN int Cudd_zddShuffleHeap ARGS((DdManager *table, int *permutation));
EXTERN DdNode * Cudd_zddIte ARGS((DdManager *dd, DdNode *f, DdNode *g, DdNode *h));
EXTERN DdNode * Cudd_zddUnion ARGS((DdManager *dd, DdNode *P, DdNode *Q));
EXTERN DdNode * Cudd_zddIntersect ARGS((DdManager *dd, DdNode *P, DdNode *Q));
EXTERN DdNode * Cudd_zddDiff ARGS((DdManager *dd, DdNode *P, DdNode *Q));
EXTERN DdNode * Cudd_zddDiffConst ARGS((DdManager *zdd, DdNode *P, DdNode *Q));
EXTERN DdNode * Cudd_zddSubset1 ARGS((DdManager *dd, DdNode *P, int var));
EXTERN DdNode * Cudd_zddSubset0 ARGS((DdManager *dd, DdNode *P, int var));
EXTERN DdNode * Cudd_zddChange ARGS((DdManager *dd, DdNode *P, int var));
EXTERN void Cudd_zddSymmProfile ARGS((DdManager *table, int lower, int upper));
EXTERN int Cudd_zddPrintMinterm ARGS((DdManager *zdd, DdNode *node));
EXTERN int Cudd_zddPrintCover ARGS((DdManager *zdd, DdNode *node));
EXTERN int Cudd_zddPrintDebug ARGS((DdManager *zdd, DdNode *f, int n, int pr));
EXTERN DdGen * Cudd_zddFirstPath ARGS((DdManager *zdd, DdNode *f, int **path));
EXTERN int Cudd_zddNextPath ARGS((DdGen *gen, int **path));
EXTERN char * Cudd_zddCoverPathToString ARGS((DdManager *zdd, int *path, char *str));
EXTERN int Cudd_zddDumpDot ARGS((DdManager *dd, int n, DdNode **f, char **inames, char **onames, FILE *fp));
EXTERN int Cudd_bddSetPiVar ARGS((DdManager *dd, int index));
EXTERN int Cudd_bddSetPsVar ARGS((DdManager *dd, int index));
EXTERN int Cudd_bddSetNsVar ARGS((DdManager *dd, int index));
EXTERN int Cudd_bddIsPiVar ARGS((DdManager *dd, int index));
EXTERN int Cudd_bddIsPsVar ARGS((DdManager *dd, int index));
EXTERN int Cudd_bddIsNsVar ARGS((DdManager *dd, int index));
EXTERN int Cudd_bddSetPairIndex ARGS((DdManager *dd, int index, int pairIndex));
EXTERN int Cudd_bddReadPairIndex ARGS((DdManager *dd, int index));
EXTERN int Cudd_bddSetVarToBeGrouped ARGS((DdManager *dd, int index));
EXTERN int Cudd_bddSetVarHardGroup ARGS((DdManager *dd, int index));
EXTERN int Cudd_bddResetVarToBeGrouped ARGS((DdManager *dd, int index));
EXTERN int Cudd_bddIsVarToBeGrouped ARGS((DdManager *dd, int index));
EXTERN int Cudd_bddSetVarToBeUngrouped ARGS((DdManager *dd, int index));
EXTERN int Cudd_bddIsVarToBeUngrouped ARGS((DdManager *dd, int index));
EXTERN int Cudd_bddIsVarHardGroup ARGS((DdManager *dd, int index));

/**AutomaticEnd***************************************************************/

#endif /* _CUDD */
