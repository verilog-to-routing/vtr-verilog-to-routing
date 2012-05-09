/**CHeaderFile*****************************************************************

  FileName    [mtr.h]

  PackageName [mtr]

  Synopsis    [Multiway-branch tree manipulation]

  Description [This package provides two layers of functions. Functions
  of the lower level manipulate multiway-branch trees, implemented
  according to the classical scheme whereby each node points to its
  first child and its previous and next siblings. These functions are
  collected in mtrBasic.c.<p>
  Functions of the upper layer deal with group trees, that is the trees
  used by group sifting to represent the grouping of variables. These
  functions are collected in mtrGroup.c.]

  SeeAlso     [The CUDD package documentation; specifically on group
  sifting.]

  Author      [Fabio Somenzi]

  Copyright   [This file was created at the University of Colorado at
  Boulder.  The University of Colorado at Boulder makes no warranty
  about the suitability of this software for any purpose.  It is
  presented on an AS IS basis.]

  Revision    [$Id: mtr.h,v 1.1.1.1 2003/02/24 22:24:02 wjiang Exp $]

******************************************************************************/

#ifndef __MTR
#define __MTR

/*---------------------------------------------------------------------------*/
/* Nested includes                                                           */
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
/* Constant declarations                                                     */
/*---------------------------------------------------------------------------*/

#ifndef SIZEOF_VOID_P
#define SIZEOF_VOID_P 4
#endif
#ifndef SIZEOF_INT
#define SIZEOF_INT 4
#endif

#undef CONST
#if defined(__STDC__) || defined(__cplusplus)
#define CONST           const
#else /* !(__STDC__ || __cplusplus) */
#define CONST
#endif /* !(__STDC__ || __cplusplus) */

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
#	define ARGS(protos)    protos          /* ANSI C */
#   else /* !(__STDC__ || __cplusplus) */
#	define ARGS(protos)    ()              /* K&R C */
#   endif /* !(__STDC__ || __cplusplus) */
#endif

#if defined(__GNUC__)
#define MTR_INLINE __inline__
# if (__GNUC__ >2 || __GNUC_MINOR__ >=7)
#   define MTR_UNUSED __attribute__ ((unused))
# else
#   define MTR_UNUSED
# endif
#else
#define MTR_INLINE
#define MTR_UNUSED
#endif
 
/* Flag definitions */
#define MTR_DEFAULT	0x00000000
#define MTR_TERMINAL 	0x00000001
#define MTR_SOFT	0x00000002
#define MTR_FIXED	0x00000004
#define MTR_NEWNODE	0x00000008

/* MTR_MAXHIGH is defined in such a way that on 32-bit and 64-bit
** machines one can cast a value to (int) without generating a negative
** number.
*/
#if SIZEOF_VOID_P == 8 && SIZEOF_INT == 4
#define MTR_MAXHIGH	(((MtrHalfWord) ~0) >> 1)
#else
#define MTR_MAXHIGH	((MtrHalfWord) ~0)
#endif


/*---------------------------------------------------------------------------*/
/* Stucture declarations                                                     */
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
/* Type declarations                                                         */
/*---------------------------------------------------------------------------*/

#if SIZEOF_VOID_P == 8 && SIZEOF_INT == 4
typedef unsigned int   MtrHalfWord;
#else
typedef unsigned short MtrHalfWord;
#endif

typedef struct MtrNode {
    MtrHalfWord flags;
    MtrHalfWord low;
    MtrHalfWord size;
    MtrHalfWord index;
    struct MtrNode *parent;
    struct MtrNode *child;
    struct MtrNode *elder;
    struct MtrNode *younger;
} MtrNode;


/*---------------------------------------------------------------------------*/
/* Variable declarations                                                     */
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
/* Macro declarations                                                        */
/*---------------------------------------------------------------------------*/

/* Flag manipulation macros */
#define MTR_SET(node, flag)		(node->flags |= (flag))
#define MTR_RESET(node, flag)	(node->flags &= ~ (flag))
#define MTR_TEST(node, flag)	(node->flags & (flag))


/**AutomaticStart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Function prototypes                                                       */
/*---------------------------------------------------------------------------*/

EXTERN MtrNode * Mtr_AllocNode ARGS(());
EXTERN void Mtr_DeallocNode ARGS((MtrNode *node));
EXTERN MtrNode * Mtr_InitTree ARGS(());
EXTERN void Mtr_FreeTree ARGS((MtrNode *node));
EXTERN MtrNode * Mtr_CopyTree ARGS((MtrNode *node, int expansion));
EXTERN void Mtr_MakeFirstChild ARGS((MtrNode *parent, MtrNode *child));
EXTERN void Mtr_MakeLastChild ARGS((MtrNode *parent, MtrNode *child));
EXTERN MtrNode * Mtr_CreateFirstChild ARGS((MtrNode *parent));
EXTERN MtrNode * Mtr_CreateLastChild ARGS((MtrNode *parent));
EXTERN void Mtr_MakeNextSibling ARGS((MtrNode *first, MtrNode *second));
EXTERN void Mtr_PrintTree ARGS((MtrNode *node));
EXTERN MtrNode * Mtr_InitGroupTree ARGS((int lower, int size));
EXTERN MtrNode * Mtr_MakeGroup ARGS((MtrNode *root, unsigned int low, unsigned int high, unsigned int flags));
EXTERN MtrNode * Mtr_DissolveGroup ARGS((MtrNode *group));
EXTERN MtrNode * Mtr_FindGroup ARGS((MtrNode *root, unsigned int low, unsigned int high));
EXTERN int Mtr_SwapGroups ARGS((MtrNode *first, MtrNode *second));
EXTERN void Mtr_PrintGroups ARGS((MtrNode *root, int silent));
EXTERN MtrNode * Mtr_ReadGroups ARGS((FILE *fp, int nleaves));

/**AutomaticEnd***************************************************************/

#endif /* __MTR */
