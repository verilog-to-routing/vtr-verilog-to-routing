/**CFile***********************************************************************

  FileName    [cuddZddCount.c]

  PackageName [cudd]

  Synopsis    [Procedures to count the number of minterms of a ZDD.]

  Description [External procedures included in this module:
		    <ul>
		    <li> Cudd_zddCount();
		    <li> Cudd_zddCountDouble();
		    </ul>
	       Internal procedures included in this module:
		    <ul>
		    </ul>
	       Static procedures included in this module:
		    <ul>
       		    <li> cuddZddCountStep();
		    <li> cuddZddCountDoubleStep();
		    <li> st_zdd_count_dbl_free()
		    <li> st_zdd_countfree()
		    </ul>
	      ]

  SeeAlso     []

  Author      [Hyong-Kyoon Shin, In-Ho Moon]

  Copyright [ This file was created at the University of Colorado at
  Boulder.  The University of Colorado at Boulder makes no warranty
  about the suitability of this software for any purpose.  It is
  presented on an AS IS basis.]

******************************************************************************/

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
static char rcsid[] DD_UNUSED = "$Id: cuddZddCount.c,v 1.1.1.1 2003/02/24 22:23:53 wjiang Exp $";
#endif

/*---------------------------------------------------------------------------*/
/* Macro declarations                                                        */
/*---------------------------------------------------------------------------*/


/**AutomaticStart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/

static int cuddZddCountStep ARGS((DdNode *P, st_table *table, DdNode *base, DdNode *empty));
static double cuddZddCountDoubleStep ARGS((DdNode *P, st_table *table, DdNode *base, DdNode *empty));
static enum st_retval st_zdd_countfree ARGS((char *key, char *value, char *arg));
static enum st_retval st_zdd_count_dbl_free ARGS((char *key, char *value, char *arg));

/**AutomaticEnd***************************************************************/


/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/


/**Function********************************************************************

  Synopsis [Counts the number of minterms in a ZDD.]

  Description [Returns an integer representing the number of minterms
  in a ZDD.]

  SideEffects [None]

  SeeAlso     [Cudd_zddCountDouble]

******************************************************************************/
int
Cudd_zddCount(
  DdManager * zdd,
  DdNode * P)
{
    st_table	*table;
    int		res;
    DdNode	*base, *empty;

    base  = DD_ONE(zdd);
    empty = DD_ZERO(zdd);
    table = st_init_table(st_ptrcmp, st_ptrhash);
    if (table == NULL) return(CUDD_OUT_OF_MEM);
    res = cuddZddCountStep(P, table, base, empty);
    if (res == CUDD_OUT_OF_MEM) {
	zdd->errorCode = CUDD_MEMORY_OUT;
    }
    st_foreach(table, st_zdd_countfree, NIL(char));
    st_free_table(table);

    return(res);

} /* end of Cudd_zddCount */


/**Function********************************************************************

  Synopsis [Counts the number of minterms of a ZDD.]

  Description [Counts the number of minterms of a ZDD. The result is
  returned as a double. If the procedure runs out of memory, it
  returns (double) CUDD_OUT_OF_MEM. This procedure is used in
  Cudd_zddCountMinterm.]

  SideEffects [None]

  SeeAlso     [Cudd_zddCountMinterm Cudd_zddCount]

******************************************************************************/
double
Cudd_zddCountDouble(
  DdManager * zdd,
  DdNode * P)
{
    st_table	*table;
    double	res;
    DdNode	*base, *empty;

    base  = DD_ONE(zdd);
    empty = DD_ZERO(zdd);
    table = st_init_table(st_ptrcmp, st_ptrhash);
    if (table == NULL) return((double)CUDD_OUT_OF_MEM);
    res = cuddZddCountDoubleStep(P, table, base, empty);
    if (res == (double)CUDD_OUT_OF_MEM) {
	zdd->errorCode = CUDD_MEMORY_OUT;
    }
    st_foreach(table, st_zdd_count_dbl_free, NIL(char));
    st_free_table(table);

    return(res);

} /* end of Cudd_zddCountDouble */


/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/


/**Function********************************************************************

  Synopsis [Performs the recursive step of Cudd_zddCount.]

  Description []

  SideEffects [None]

  SeeAlso     []

******************************************************************************/
static int
cuddZddCountStep(
  DdNode * P,
  st_table * table,
  DdNode * base,
  DdNode * empty)
{
    int		res;
    int		*dummy;

    if (P == empty)
	return(0);
    if (P == base)
	return(1);

    /* Check cache. */
    if (st_lookup(table, (char *)P, (char **)(&dummy))) {
	res = *dummy;
	return(res);
    }

    res = cuddZddCountStep(cuddE(P), table, base, empty) +
	cuddZddCountStep(cuddT(P), table, base, empty);

    dummy = ALLOC(int, 1);
    if (dummy == NULL) {
	return(CUDD_OUT_OF_MEM);
    }
    *dummy = res;
    if (st_insert(table, (char *)P, (char *)dummy) == ST_OUT_OF_MEM) {
	FREE(dummy);
	return(CUDD_OUT_OF_MEM);
    }

    return(res);

} /* end of cuddZddCountStep */


/**Function********************************************************************

  Synopsis [Performs the recursive step of Cudd_zddCountDouble.]

  Description []

  SideEffects [None]

  SeeAlso     []

******************************************************************************/
static double
cuddZddCountDoubleStep(
  DdNode * P,
  st_table * table,
  DdNode * base,
  DdNode * empty)
{
    double	res;
    double	*dummy;

    if (P == empty)
	return((double)0.0);
    if (P == base)
	return((double)1.0);

    /* Check cache */
    if (st_lookup(table, (char *)P, (char **)(&dummy))) {
	res = *dummy;
	return(res);
    }

    res = cuddZddCountDoubleStep(cuddE(P), table, base, empty) +
	cuddZddCountDoubleStep(cuddT(P), table, base, empty);

    dummy = ALLOC(double, 1);
    if (dummy == NULL) {
	return((double)CUDD_OUT_OF_MEM);
    }
    *dummy = res;
    if (st_insert(table, (char *)P, (char *)dummy) == ST_OUT_OF_MEM) {
	FREE(dummy);
	return((double)CUDD_OUT_OF_MEM);
    }

    return(res);

} /* end of cuddZddCountDoubleStep */


/**Function********************************************************************

  Synopsis [Frees the memory associated with the computed table of
  Cudd_zddCount.]

  Description []

  SideEffects [None]

  SeeAlso     []

******************************************************************************/
static enum st_retval
st_zdd_countfree(
  char * key,
  char * value,
  char * arg)
{
    int	*d;

    d = (int *)value;
    FREE(d);
    return(ST_CONTINUE);

} /* end of st_zdd_countfree */


/**Function********************************************************************

  Synopsis [Frees the memory associated with the computed table of
  Cudd_zddCountDouble.]

  Description []

  SideEffects [None]

  SeeAlso     []

******************************************************************************/
static enum st_retval
st_zdd_count_dbl_free(
  char * key,
  char * value,
  char * arg)
{
    double	*d;

    d = (double *)value;
    FREE(d);
    return(ST_CONTINUE);

} /* end of st_zdd_count_dbl_free */
