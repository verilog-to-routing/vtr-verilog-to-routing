/**CFile***********************************************************************

  FileName    [cuddInteract.c]

  PackageName [cudd]

  Synopsis    [Functions to manipulate the variable interaction matrix.]

  Description [Internal procedures included in this file:
	<ul>
	<li> cuddSetInteract()
	<li> cuddTestInteract()
	<li> cuddInitInteract()
	</ul>
  Static procedures included in this file:
	<ul>
	<li> ddSuppInteract()
	<li> ddClearLocal()
	<li> ddUpdateInteract()
	<li> ddClearGlobal()
	</ul>
  The interaction matrix tells whether two variables are
  both in the support of some function of the DD. The main use of the
  interaction matrix is in the in-place swapping. Indeed, if two
  variables do not interact, there is no arc connecting the two layers;
  therefore, the swap can be performed in constant time, without
  scanning the subtables. Another use of the interaction matrix is in
  the computation of the lower bounds for sifting. Finally, the
  interaction matrix can be used to speed up aggregation checks in
  symmetric and group sifting.<p>
  The computation of the interaction matrix is done with a series of
  depth-first searches. The searches start from those nodes that have
  only external references. The matrix is stored as a packed array of bits;
  since it is symmetric, only the upper triangle is kept in memory.
  As a final remark, we note that there may be variables that do
  intercat, but that for a given variable order have no arc connecting
  their layers when they are adjacent.]

  SeeAlso     []

  Author      [Fabio Somenzi]

  Copyright [ This file was created at the University of Colorado at
  Boulder.  The University of Colorado at Boulder makes no warranty
  about the suitability of this software for any purpose.  It is
  presented on an AS IS basis.]

******************************************************************************/

#include "util_hack.h"
#include "cuddInt.h"

/*---------------------------------------------------------------------------*/
/* Constant declarations                                                     */
/*---------------------------------------------------------------------------*/

#if SIZEOF_LONG == 8
#define BPL 64
#define LOGBPL 6
#else
#define BPL 32
#define LOGBPL 5
#endif

/*---------------------------------------------------------------------------*/
/* Stucture declarations                                                     */
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
/* Type declarations                                                         */
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
/* Variable declarations                                                     */
/*---------------------------------------------------------------------------*/

#ifndef lint
static char rcsid[] DD_UNUSED = "$Id: cuddInteract.c,v 1.1.1.1 2003/02/24 22:23:52 wjiang Exp $";
#endif

/*---------------------------------------------------------------------------*/
/* Macro declarations                                                        */
/*---------------------------------------------------------------------------*/


/**AutomaticStart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/

static void ddSuppInteract ARGS((DdNode *f, int *support));
static void ddClearLocal ARGS((DdNode *f));
static void ddUpdateInteract ARGS((DdManager *table, int *support));
static void ddClearGlobal ARGS((DdManager *table));

/**AutomaticEnd***************************************************************/


/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/


/**Function********************************************************************

  Synopsis    [Set interaction matrix entries.]

  Description [Given a pair of variables 0 <= x < y < table->size,
  sets the corresponding bit of the interaction matrix to 1.]

  SideEffects [None]

  SeeAlso     []

******************************************************************************/
void
cuddSetInteract(
  DdManager * table,
  int  x,
  int  y)
{
    int posn, word, bit;

#ifdef DD_DEBUG
    assert(x < y);
    assert(y < table->size);
    assert(x >= 0);
#endif

    posn = ((((table->size << 1) - x - 3) * x) >> 1) + y - 1;
    word = posn >> LOGBPL;
    bit = posn & (BPL-1);
    table->interact[word] |= 1L << bit;

} /* end of cuddSetInteract */


/**Function********************************************************************

  Synopsis    [Test interaction matrix entries.]

  Description [Given a pair of variables 0 <= x < y < table->size,
  tests whether the corresponding bit of the interaction matrix is 1.
  Returns the value of the bit.]

  SideEffects [None]

  SeeAlso     []

******************************************************************************/
int
cuddTestInteract(
  DdManager * table,
  int  x,
  int  y)
{
    int posn, word, bit, result;

    if (x > y) {
	int tmp = x;
	x = y;
	y = tmp;
    }
#ifdef DD_DEBUG
    assert(x < y);
    assert(y < table->size);
    assert(x >= 0);
#endif

    posn = ((((table->size << 1) - x - 3) * x) >> 1) + y - 1;
    word = posn >> LOGBPL;
    bit = posn & (BPL-1);
    result = (table->interact[word] >> bit) & 1L;
    return(result);

} /* end of cuddTestInteract */


/**Function********************************************************************

  Synopsis    [Initializes the interaction matrix.]

  Description [Initializes the interaction matrix. The interaction
  matrix is implemented as a bit vector storing the upper triangle of
  the symmetric interaction matrix. The bit vector is kept in an array
  of long integers. The computation is based on a series of depth-first
  searches, one for each root of the DAG. Two flags are needed: The
  local visited flag uses the LSB of the then pointer. The global
  visited flag uses the LSB of the next pointer.
  Returns 1 if successful; 0 otherwise.]

  SideEffects [None]

  SeeAlso     []

******************************************************************************/
int
cuddInitInteract(
  DdManager * table)
{
    int i,j,k;
    int words;
    long *interact;
    int *support;
    DdNode *f;
    DdNode *sentinel = &(table->sentinel);
    DdNodePtr *nodelist;
    int slots;
    int n = table->size;

    words = ((n * (n-1)) >> (1 + LOGBPL)) + 1;
    table->interact = interact = ALLOC(long,words);
    if (interact == NULL) {
	table->errorCode = CUDD_MEMORY_OUT;
	return(0);
    }
    for (i = 0; i < words; i++) {
	interact[i] = 0;
    }

    support = ALLOC(int,n);
    if (support == NULL) {
	table->errorCode = CUDD_MEMORY_OUT;
	FREE(interact);
	return(0);
    }

    for (i = 0; i < n; i++) {
	nodelist = table->subtables[i].nodelist;
	slots = table->subtables[i].slots;
	for (j = 0; j < slots; j++) {
	    f = nodelist[j];
	    while (f != sentinel) {
		/* A node is a root of the DAG if it cannot be
		** reached by nodes above it. If a node was never
		** reached during the previous depth-first searches,
		** then it is a root, and we start a new depth-first
		** search from it.
		*/
		if (!Cudd_IsComplement(f->next)) {
		    for (k = 0; k < n; k++) {
			support[k] = 0;
		    }
		    ddSuppInteract(f,support);
		    ddClearLocal(f);
		    ddUpdateInteract(table,support);
		}
		f = Cudd_Regular(f->next);
	    }
	}
    }
    ddClearGlobal(table);

    FREE(support);
    return(1);

} /* end of cuddInitInteract */


/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/


/**Function********************************************************************

  Synopsis    [Find the support of f.]

  Description [Performs a DFS from f. Uses the LSB of the then pointer
  as visited flag.]

  SideEffects [Accumulates in support the variables on which f depends.]

  SeeAlso     []

******************************************************************************/
static void
ddSuppInteract(
  DdNode * f,
  int * support)
{
    if (cuddIsConstant(f) || Cudd_IsComplement(cuddT(f))) {
	return;
    }

    support[f->index] = 1;
    ddSuppInteract(cuddT(f),support);
    ddSuppInteract(Cudd_Regular(cuddE(f)),support);
    /* mark as visited */
    cuddT(f) = Cudd_Complement(cuddT(f));
    f->next = Cudd_Complement(f->next);
    return;

} /* end of ddSuppInteract */


/**Function********************************************************************

  Synopsis    [Performs a DFS from f, clearing the LSB of the then pointers.]

  Description []

  SideEffects [None]

  SeeAlso     []

******************************************************************************/
static void
ddClearLocal(
  DdNode * f)
{
    if (cuddIsConstant(f) || !Cudd_IsComplement(cuddT(f))) {
	return;
    }
    /* clear visited flag */
    cuddT(f) = Cudd_Regular(cuddT(f));
    ddClearLocal(cuddT(f));
    ddClearLocal(Cudd_Regular(cuddE(f)));
    return;

} /* end of ddClearLocal */


/**Function********************************************************************

  Synopsis [Marks as interacting all pairs of variables that appear in
  support.]

  Description [If support[i] == support[j] == 1, sets the (i,j) entry
  of the interaction matrix to 1.]

  SideEffects [None]

  SeeAlso     []

******************************************************************************/
static void
ddUpdateInteract(
  DdManager * table,
  int * support)
{
    int i,j;
    int n = table->size;

    for (i = 0; i < n-1; i++) {
	if (support[i] == 1) {
	    for (j = i+1; j < n; j++) {
		if (support[j] == 1) {
		    cuddSetInteract(table,i,j);
		}
	    }
	}
    }

} /* end of ddUpdateInteract */


/**Function********************************************************************

  Synopsis    [Scans the DD and clears the LSB of the next pointers.]

  Description [The LSB of the next pointers are used as markers to tell
  whether a node was reached by at least one DFS. Once the interaction
  matrix is built, these flags are reset.]

  SideEffects [None]

  SeeAlso     []

******************************************************************************/
static void
ddClearGlobal(
  DdManager * table)
{
    int i,j;
    DdNode *f;
    DdNode *sentinel = &(table->sentinel);
    DdNodePtr *nodelist;
    int slots;

    for (i = 0; i < table->size; i++) {
	nodelist = table->subtables[i].nodelist;
	slots = table->subtables[i].slots;
	for (j = 0; j < slots; j++) {
	    f = nodelist[j];
	    while (f != sentinel) {
		f->next = Cudd_Regular(f->next);
		f = f->next;
	    }
	}
    }

} /* end of ddClearGlobal */

