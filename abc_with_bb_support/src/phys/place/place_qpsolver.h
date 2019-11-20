/*===================================================================*/
//  
//     place_qpsolver.h
//
//		Philip Chong
//              pchong@cadence.com
//
/*===================================================================*/

#if !defined(_QPS_H)
#define _QPS_H

#include <stdio.h>

#if defined(__cplusplus)
extern "C" {
#endif				/* __cplusplus */

  typedef float qps_float_t;

  typedef struct qps_problem {

    /* Basic stuff */
    int num_cells;		/* Total number of cells (both fixed and
				   floating) to be placed. */
    int *connect;		/* Connectivity array.  Must contain at least 
				   num_cells elements with value -1. The
				   entries which precede the first element
				   with value -1 are the indices of the cells 
				   which connect to cell 0; the entries
				   which lie between the first and second
				   elements with value -1 are the indices of
				   the cells which connect to cell 1; etc.
				   Example: cells 0 and 1 are connected
				   together, and 1 and 2 are connected as
				   well.  *connect = { 1, -1, 0, 2, -1, 1, -1
				   }. */
    qps_float_t *edge_weight;	/* Same structure as connectivity array, but
				   giving the weights assigned to each edge
				   instead. */
    qps_float_t *x;		/* num_cells element array which contains the
				   x-coordinates of the cells. This is used
				   for the initial values in the iterative
				   solution of floating cells, and for the
				   fixed location of fixed cells. */
    qps_float_t *y;		/* num_cells element array of
				   y-coordinates. */
    int *fixed;			/* num_cells element array with value 1 if
				   corresponding cell is fixed, 0 if
				   floating. */
    qps_float_t f;		/* return value for sum-of-square
				   wirelengths. */

    /* COG stuff */
    int cog_num;		/* Number of COG constraints. */
    int *cog_list;		/* Array indicating for each COG constraint
				   which cells belong to that constraint.
				   Format is similar to c array: there must
				   be at least cog_num elements with value
				   -1.  The entries of cog_list preceding the 
				   first -1 element are the indices of the
				   cells which belong to the first COG
				   constraint; etc.  Example: cells 0 and 1
				   belong to one COG constraint, cells 4 and
				   5 belong to another.  *cog_list= { 0, 1,
				   -1, 4, 5, -1 }. */
    qps_float_t *cog_x;		/* cog_num element array whose values are the
				   x-coordinates for the corresponding COG
				   constraints. */
    qps_float_t *cog_y;		/* cog_num element array whose values are the
				   y-coordinates for the corresponding COG
				   constraints. */
    qps_float_t *area;     	/* num_cells element array whose values are
				   the areas for the corresponding cells;
				   only useful with COG constraints. */

    /* Loop constraint stuff */
    int loop_num;		/* Number of loop constraints. */
    int *loop_list;		/* Array with list of cells for each loop
				   constraint. Format is similar to cog_list. 
				 */
    qps_float_t *loop_max;	/* loop_num element array indicating maximum
				   distance for each loop. */
    qps_float_t *loop_penalty;	/* loop_num element array indicating penalty
				   for each loop. */
    int loop_k;			/* Current iteration for loop optimization. */
    int loop_done;		/* Done flag for loop optimization. */
    int loop_fail;

    /* max_x/max_y stuff */
    qps_float_t max_x;		/* max x location;  only used in
				   constrained optimization. */
    qps_float_t max_y;		/* max y location;  only used in
				   constrained optimization. */
    int max_enable;		/* Set to 1 after qps_init() to enable
				   max_x/max_y. */
    int max_done;		/* Done flag for max optimization. */

    /* Private stuff */
    int *priv_ii;
    int *priv_cc, *priv_cr;
    qps_float_t *priv_cw, *priv_ct;
    int priv_cm;
    int *priv_gt;
    int *priv_la;
    int priv_lm;
    qps_float_t *priv_gm, *priv_gw;
    qps_float_t *priv_g, *priv_h, *priv_xi;
    qps_float_t *priv_tp, *priv_tp2;
    int priv_n;
    qps_float_t *priv_cp;
    qps_float_t priv_f;
    qps_float_t *priv_lt;
    qps_float_t *priv_pcg, *priv_pcgt;
    qps_float_t priv_fmax;
    qps_float_t priv_fprev;
    qps_float_t priv_fopt;
    qps_float_t priv_eps;
    int priv_pn;
    qps_float_t *priv_mxl, *priv_mxh, *priv_myl, *priv_myh;
    int priv_ik;
    FILE *priv_fp;

  } qps_problem_t;

  /* call qps_init() as soon as the qps_problem_t has been set up */
  /* this initializes some private data structures */
  extern void qps_init(qps_problem_t *);

  /* call qps_solve() to solve the given qp problem */
  extern void qps_solve(qps_problem_t *);

  /* call qps_clean() when finished with the qps_problem_t */
  /* this discards the private data structures assigned by qps_init() */
  extern void qps_clean(qps_problem_t *);

#if defined(__cplusplus)
}
#endif				/* __cplusplus */
#endif				/* _QPS_H */
