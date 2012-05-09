/**CFile***********************************************************************

  FileName    [cuddZddMisc.c]

  PackageName [cudd]

  Synopsis    [.]

  Description [External procedures included in this module:
		    <ul>
		    <li> Cudd_zddDagSize()
		    <li> Cudd_zddCountMinterm()
		    <li> Cudd_zddPrintSubtable()
		    </ul>
	       Internal procedures included in this module:
		    <ul>
		    </ul>
	       Static procedures included in this module:
		    <ul>
		    <li> cuddZddDagInt()
		    </ul>
	      ]

  SeeAlso     []

  Author      [Hyong-Kyoon Shin, In-Ho Moon]

  Copyright [ This file was created at the University of Colorado at
  Boulder.  The University of Colorado at Boulder makes no warranty
  about the suitability of this software for any purpose.  It is
  presented on an AS IS basis.]

******************************************************************************/

#include    <math.h>
#include    "util_hack.h"
#include    "cuddInt.h"

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

#ifndef lint
static char rcsid[] DD_UNUSED = "$Id: cuddZddMisc.c,v 1.1.1.1 2003/02/24 22:23:54 wjiang Exp $";
#endif

/*---------------------------------------------------------------------------*/
/* Macro declarations                                                        */
/*---------------------------------------------------------------------------*/


/**AutomaticStart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/

static int cuddZddDagInt ARGS((DdNode *n, st_table *tab));

/**AutomaticEnd***************************************************************/


/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/


/**Function********************************************************************

  Synopsis    [Counts the number of nodes in a ZDD.]

  Description [Counts the number of nodes in a ZDD. This function
  duplicates Cudd_DagSize and is only retained for compatibility.]

  SideEffects [None]

  SeeAlso     [Cudd_DagSize]

******************************************************************************/
int
Cudd_zddDagSize(
  DdNode * p_node)
{

    int		i;	
    st_table	*table;

    table = st_init_table(st_ptrcmp, st_ptrhash);
    i = cuddZddDagInt(p_node, table);
    st_free_table(table);
    return(i);

} /* end of Cudd_zddDagSize */


/**Function********************************************************************

  Synopsis    [Counts the number of minterms of a ZDD.]

  Description [Counts the number of minterms of the ZDD rooted at
  <code>node</code>. This procedure takes a parameter
  <code>path</code> that specifies how many variables are in the
  support of the function. If the procedure runs out of memory, it
  returns (double) CUDD_OUT_OF_MEM.]

  SideEffects [None]

  SeeAlso     [Cudd_zddCountDouble]

******************************************************************************/
double
Cudd_zddCountMinterm(
  DdManager * zdd,
  DdNode * node,
  int  path)
{
    double	dc_var, minterms;	

    dc_var = (double)((double)(zdd->sizeZ) - (double)path);
    minterms = Cudd_zddCountDouble(zdd, node) / pow(2.0, dc_var);
    return(minterms);

} /* end of Cudd_zddCountMinterm */


/**Function********************************************************************

  Synopsis    [Prints the ZDD table.]

  Description [Prints the ZDD table for debugging purposes.]

  SideEffects [None]

  SeeAlso     []

******************************************************************************/
void
Cudd_zddPrintSubtable(
  DdManager * table)
{
    int		i, j;
    DdNode 	*z1, *z1_next, *base;
    DdSubtable	*ZSubTable;

    base = table->one;
    for (i = table->sizeZ - 1; i >= 0; i--) {
	ZSubTable = &(table->subtableZ[i]);
	printf("subtable[%d]:\n", i);
	for (j = ZSubTable->slots - 1; j >= 0; j--) {
	    z1 = ZSubTable->nodelist[j];
	    while (z1 != NIL(DdNode)) {
		(void) fprintf(table->out,
#if SIZEOF_VOID_P == 8
		    "ID = 0x%lx\tindex = %d\tr = %d\t",
		    (unsigned long) z1 / (unsigned long) sizeof(DdNode),
		    z1->index, z1->ref);
#else
		    "ID = 0x%x\tindex = %d\tr = %d\t",
		    (unsigned) z1 / (unsigned) sizeof(DdNode),
		    z1->index, z1->ref);
#endif
		z1_next = cuddT(z1);
		if (Cudd_IsConstant(z1_next)) {
		    (void) fprintf(table->out, "T = %d\t\t",
			(z1_next == base));
		}
		else {
#if SIZEOF_VOID_P == 8
		    (void) fprintf(table->out, "T = 0x%lx\t",
			(unsigned long) z1_next / (unsigned long) sizeof(DdNode));
#else
		    (void) fprintf(table->out, "T = 0x%x\t",
			(unsigned) z1_next / (unsigned) sizeof(DdNode));
#endif
		}
		z1_next = cuddE(z1);
		if (Cudd_IsConstant(z1_next)) {
		    (void) fprintf(table->out, "E = %d\n",
			(z1_next == base));
		}
		else {
#if SIZEOF_VOID_P == 8
		    (void) fprintf(table->out, "E = 0x%lx\n",
			(unsigned long) z1_next / (unsigned long) sizeof(DdNode));
#else
		    (void) fprintf(table->out, "E = 0x%x\n",
			(unsigned) z1_next / (unsigned) sizeof(DdNode));
#endif
		}

		z1_next = z1->next;
		z1 = z1_next;
	    }
	}
    }
    putchar('\n');

} /* Cudd_zddPrintSubtable */


/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/


/**Function********************************************************************

  Synopsis    [Performs the recursive step of Cudd_zddDagSize.]

  Description [Performs the recursive step of Cudd_zddDagSize. Does
  not check for out-of-memory conditions.]

  SideEffects [None]

  SeeAlso     []

******************************************************************************/
static int
cuddZddDagInt(
  DdNode * n,
  st_table * tab)
{
    if (n == NIL(DdNode))
	return(0);

    if (st_is_member(tab, (char *)n) == 1)
	return(0);

    if (Cudd_IsConstant(n))
	return(0);

    (void)st_insert(tab, (char *)n, NIL(char));
    return(1 + cuddZddDagInt(cuddT(n), tab) +
	cuddZddDagInt(cuddE(n), tab));

} /* cuddZddDagInt */

