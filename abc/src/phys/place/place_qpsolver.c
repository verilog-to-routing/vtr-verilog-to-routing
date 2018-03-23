/*===================================================================*/
//  
//     place_qpsolver.c
//
//		Philip Chong
//              pchong@cadence.com
//
/*===================================================================*/

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "place_qpsolver.h"

ABC_NAMESPACE_IMPL_START


#undef  QPS_DEBUG

#define QPS_TOL 1.0e-3
#define QPS_EPS (QPS_TOL * QPS_TOL)

#define QPS_MAX_TOL 0.1
#define QPS_LOOP_TOL 1.0e-3

#define QPS_RELAX_ITER 180
#define QPS_MAX_ITER 200
#define QPS_STEPSIZE_RETRIES 2
#define QPS_MINSTEP 1.0e-6
#define QPS_DEC_CHANGE 0.01

#define QPS_PRECON
#define QPS_PRECON_EPS 1.0e-9

#undef QPS_HOIST

#if defined(QPS_DEBUG)
#define QPS_DEBUG_FILE "/tmp/qps_debug.log"
#endif

#if 0
  /* ii is an array [0..p->num_cells-1] of indices from cells of original
     problem to modified problem variables.  If ii[k] >= 0, cell is an
     independent cell; ii[k], ii[k]+1 are the indices of the corresponding
     variables for the modified problem.  If ii[k] == -1, cell is a fixed
     cell.  If ii[k] <= -2, cell is a dependent cell; -(ii[k]+2) is the index 
     of the corresponding COG constraint. */
int *ii;

  /* gt is an array [0..p->cog_num-1] of indices from COG constraints to
     locations in the gl array (in qps_problem_t).  gt[k] is the offset into
     gl where the kth constraint begins. */
int *gt;

  /* n is the number of variables in the modified problem.  n should be twice
     the number of independent cells. */
int n;

qps_float_t *cp;		/* current location during CG iteration */
qps_float_t f;			/* value of cost function at p */

#endif

/**********************************************************************/

static void
qps_settp(qps_problem_t * p)
{
  /* Fill in the p->priv_tp array with the current locations of all cells
     (independent, dependent and fixed). */

  int i;
  int t, u;
  int pr;
  qps_float_t rx, ry;
  qps_float_t ta;

  int *ii = p->priv_ii;
  qps_float_t *tp = p->priv_tp;
  qps_float_t *cp = p->priv_cp;

  /* do independent and fixed cells first */
  for (i = p->num_cells; i--;) {
    t = ii[i];
    if (t >= 0) {		/* indep cell */
      tp[i * 2] = cp[t];
      tp[i * 2 + 1] = cp[t + 1];
    }
    else if (t == -1) {		/* fixed cell */
      tp[i * 2] = p->x[i];
      tp[i * 2 + 1] = p->y[i];
    }
  }
  /* now do dependent cells */
  for (i = p->num_cells; i--;) {
    if (ii[i] < -1) {
      t = -(ii[i] + 2);		/* index of COG constraint */
      ta = 0.0;
      rx = 0.0;
      ry = 0.0;
      pr = p->priv_gt[t];
      while ((u = p->cog_list[pr++]) >= 0) {
	ta += p->area[u];
	if (u != i) {
	  rx -= p->area[u] * tp[u * 2];
	  ry -= p->area[u] * tp[u * 2 + 1];
	}
      }
      rx += p->cog_x[t] * ta;
      ry += p->cog_y[t] * ta;
      tp[i * 2] = rx / p->area[i];
      tp[i * 2 + 1] = ry / p->area[i];
    }
  }

#if (QPS_DEBUG > 5)
  fprintf(p->priv_fp, "### qps_settp()\n");
  for (i = 0; i < p->num_cells; i++) {
    fprintf(p->priv_fp, "%f %f\n", tp[i * 2], tp[i * 2 + 1]);
  }
#endif
}

/**********************************************************************/

static qps_float_t
qps_func(qps_problem_t * p)
{
  /* Return f(p).  qps_settp() should have already been called before
     entering here */

  int j, k;
  int pr;
  qps_float_t jx, jy, tx, ty;
  qps_float_t f;
  qps_float_t w;

#if !defined(QPS_HOIST)
  int i;
  int st;
  qps_float_t kx, ky, sx, sy;
  qps_float_t t;
#endif

  qps_float_t *tp = p->priv_tp;

  f = 0.0;
  pr = 0;
  for (j = 0; j < p->num_cells; j++) {
    jx = tp[j * 2];
    jy = tp[j * 2 + 1];
    while ((k = p->priv_cc[pr]) >= 0) {
      w = p->priv_cw[pr];
      tx = tp[k * 2] - jx;
      ty = tp[k * 2 + 1] - jy;
      f += w * (tx * tx + ty * ty);
      pr++;
    }
    pr++;
  }
  p->f = f;

#if !defined(QPS_HOIST)
  /* loop penalties */
  pr = 0;
  for (i = 0; i < p->loop_num; i++) {
    t = 0.0;
    j = st = p->loop_list[pr++];
    jx = sx = tp[j * 2];
    jy = sy = tp[j * 2 + 1];
    while ((k = p->loop_list[pr]) >= 0) {
      kx = tp[k * 2];
      ky = tp[k * 2 + 1];
      tx = jx - kx;
      ty = jy - ky;
      t += tx * tx + ty * ty;
      j = k;
      jx = kx;
      jy = ky;
      pr++;
    }
    tx = jx - sx;
    ty = jy - sy;
    t += tx * tx + ty * ty;
    t -= p->loop_max[i];
#if (QPS_DEBUG > 5)
    fprintf(p->priv_fp, "### qps_penalty() %d %f %f\n",
	    i, p->loop_max[i], t);
#endif
    p->priv_lt[i] = t;
    f += p->loop_penalty[i] * t;
    pr++;
  }
#endif /* QPS_HOIST */

  if (p->max_enable) {
    for (j = p->num_cells; j--;) {
      f += p->priv_mxl[j] * (-tp[j * 2]);
      f += p->priv_mxh[j] * (tp[j * 2] - p->max_x);
      f += p->priv_myl[j] * (-tp[j * 2 + 1]);
      f += p->priv_myh[j] * (tp[j * 2 + 1] - p->max_y);
    }
  }

#if (QPS_DEBUG > 5)
  fprintf(p->priv_fp, "### qps_func() %f %f\n", f, p->f);
#endif
  return f;
}

/**********************************************************************/

static void
qps_dfunc(qps_problem_t * p, qps_float_t * d)
{
  /* Set d to grad f(p).  First computes partial derivatives wrt all cells
     then finds gradient wrt only the independent cells.  qps_settp() should
     have already been called before entering here */

  int i, j, k;
  int pr = 0;
  qps_float_t jx, jy, kx, ky, tx, ty;
  int ji, ki;
  qps_float_t w;

#if !defined(QPS_HOIST)
  qps_float_t sx, sy;
  int st;
#endif

  qps_float_t *tp = p->priv_tp;
  qps_float_t *tp2 = p->priv_tp2;

  /* compute partials and store in tp2 */
  for (i = p->num_cells; i--;) {
    tp2[i * 2] = 0.0;
    tp2[i * 2 + 1] = 0.0;
  }
  for (j = 0; j < p->num_cells; j++) {
    jx = tp[j * 2];
    jy = tp[j * 2 + 1];
    while ((k = p->priv_cc[pr]) >= 0) {
      w = 2.0 * p->priv_cw[pr];
      kx = tp[k * 2];
      ky = tp[k * 2 + 1];
      tx = w * (jx - kx);
      ty = w * (jy - ky);
      tp2[j * 2] += tx;
      tp2[k * 2] -= tx;
      tp2[j * 2 + 1] += ty;
      tp2[k * 2 + 1] -= ty;
      pr++;
    }
    pr++;
  }

#if !defined(QPS_HOIST)
  /* loop penalties */
  pr = 0;
  for (i = 0; i < p->loop_num; i++) {
    j = st = p->loop_list[pr++];
    jx = sx = tp[j * 2];
    jy = sy = tp[j * 2 + 1];
    w = 2.0 * p->loop_penalty[i];
    while ((k = p->loop_list[pr]) >= 0) {
      kx = tp[k * 2];
      ky = tp[k * 2 + 1];
      tx = w * (jx - kx);
      ty = w * (jy - ky);
      tp2[j * 2] += tx;
      tp2[k * 2] -= tx;
      tp2[j * 2 + 1] += ty;
      tp2[k * 2 + 1] -= ty;
      j = k;
      jx = kx;
      jy = ky;
      pr++;
    }
    tx = w * (jx - sx);
    ty = w * (jy - sy);
    tp2[j * 2] += tx;
    tp2[st * 2] -= tx;
    tp2[j * 2 + 1] += ty;
    tp2[st * 2 + 1] -= ty;
    pr++;
  }
#endif /* QPS_HOIST */

  if (p->max_enable) {
    for (j = p->num_cells; j--;) {
      tp2[j * 2] += p->priv_mxh[j] - p->priv_mxl[j];
      tp2[j * 2 + 1] += p->priv_myh[j] - p->priv_myl[j];
    }
  }

#if (QPS_DEBUG > 5)
  fprintf(p->priv_fp, "### qps_dfunc() partials\n");
  for (j = 0; j < p->num_cells; j++) {
    fprintf(p->priv_fp, "%f %f\n", tp2[j * 2], tp2[j * 2 + 1]);
  }
#endif

  /* translate partials to independent variables */
  for (j = p->priv_n; j--;) {
    d[j] = 0.0;
  }
  for (j = p->num_cells; j--;) {
    ji = p->priv_ii[j];
    if (ji >= 0) {		/* indep var */
      d[ji] += tp2[j * 2];
      d[ji + 1] += tp2[j * 2 + 1];
    }
    else if (ji < -1) {		/* dependent variable */
      ji = -(ji + 2);		/* get COG index */
      pr = p->priv_gt[ji];
      while ((k = p->cog_list[pr]) >= 0) {
	ki = p->priv_ii[k];
	if (ki >= 0) {
	  w = p->priv_gw[pr];
#if (QPS_DEBUG > 0)
	  assert(fabs(w - p->area[k] / p->area[j]) < 1.0e-6);
#endif
	  d[ki] -= tp2[j * 2] * w;
	  d[ki + 1] -= tp2[j * 2 + 1] * w;
	}
	pr++;
      }
    }
  }

#if (QPS_DEBUG > 5)
  fprintf(p->priv_fp, "### qps_dfunc() gradient\n");
  for (j = 0; j < p->priv_n; j++) {
    fprintf(p->priv_fp, "%f\n", d[j]);
  }
#endif
}

/**********************************************************************/

static void
qps_linmin(qps_problem_t * p, qps_float_t dgg, qps_float_t * h)
{
  /* Perform line minimization.  p->priv_cp is the current location, h is
     direction of the gradient.  Updates p->priv_cp to line minimal position
     based on formulas from "Handbook of Applied Optimization", Pardalos and
     Resende, eds., Oxford Univ. Press, 2002.  qps_settp() should have
     already been called before entering here.  Since p->priv_cp is changed,
     p->priv_tp array becomes invalid following this routine. */

  int i, j, k;
  int pr;
  int ji, ki;
  qps_float_t jx, jy, kx, ky;
  qps_float_t f = 0.0;
  qps_float_t w;

#if !defined(QPS_HOIST)
  int st;
  qps_float_t sx, sy, tx, ty;
  qps_float_t t;
#endif

  qps_float_t *tp = p->priv_tp;

  /* translate h vector to partials over all variables and store in tp */
  for (i = p->num_cells; i--;) {
    tp[i * 2] = 0.0;
    tp[i * 2 + 1] = 0.0;
  }
  for (j = p->num_cells; j--;) {
    ji = p->priv_ii[j];
    if (ji >= 0) {		/* indep cell */
      tp[j * 2] = h[ji];
      tp[j * 2 + 1] = h[ji + 1];
    }
    else if (ji < -1) {		/* dep cell */
      ji = -(ji + 2);		/* get COG index */
      pr = p->priv_gt[ji];
      while ((k = p->cog_list[pr]) >= 0) {
	ki = p->priv_ii[k];
	if (ki >= 0) {
	  w = p->priv_gw[pr];
#if (QPS_DEBUG > 0)
	  assert(fabs(w - p->area[k] / p->area[j]) < 1.0e-6);
#endif
	  tp[j * 2] -= h[ki] * w;
	  tp[j * 2 + 1] -= h[ki + 1] * w;
	}
	pr++;
      }
    }
  }

  /* take product x^T Z^T C Z x */
  pr = 0;
  for (j = 0; j < p->num_cells; j++) {
    jx = tp[j * 2];
    jy = tp[j * 2 + 1];
    while ((k = p->priv_cc[pr]) >= 0) {
      w = p->priv_cw[pr];
      kx = tp[k * 2] - jx;
      ky = tp[k * 2 + 1] - jy;
      f += w * (kx * kx + ky * ky);
      pr++;
    }
    pr++;
  }

#if !defined(QPS_HOIST)
  /* add loop penalties */
  pr = 0;
  for (i = 0; i < p->loop_num; i++) {
    t = 0.0;
    j = st = p->loop_list[pr++];
    jx = sx = tp[j * 2];
    jy = sy = tp[j * 2 + 1];
    while ((k = p->loop_list[pr]) >= 0) {
      kx = tp[k * 2];
      ky = tp[k * 2 + 1];
      tx = jx - kx;
      ty = jy - ky;
      t += tx * tx + ty * ty;
      j = k;
      jx = kx;
      jy = ky;
      pr++;
    }
    tx = jx - sx;
    ty = jy - sy;
    t += tx * tx + ty * ty;
    f += p->loop_penalty[i] * t;
    pr++;
  }
#endif /* QPS_HOIST */

#if (QPS_DEBUG > 0)
  assert(f);
#endif

  /* compute step size */
  f = (dgg / f) / 2.0;
  for (j = p->priv_n; j--;) {
    p->priv_cp[j] += f * h[j];
  }
#if (QPS_DEBUG > 5)
  fprintf(p->priv_fp, "### qps_linmin() step %f\n", f);
  for (j = 0; j < p->priv_n; j++) {
    fprintf(p->priv_fp, "%f\n", p->priv_cp[j]);
  }
#endif
}

/**********************************************************************/

static void
qps_cgmin(qps_problem_t * p)
{
  /* Perform CG minimization. Mostly from "Numerical Recipes", Press et al.,
     Cambridge Univ. Press, 1992, with some changes to help performance in
     our restricted problem domain. */

  qps_float_t fp, gg, dgg, gam;
  qps_float_t t;
  int i, j;

  int n = p->priv_n;
  qps_float_t *g = p->priv_g;
  qps_float_t *h = p->priv_h;
  qps_float_t *xi = p->priv_xi;

  qps_settp(p);
  fp = qps_func(p);
  qps_dfunc(p, g);

  dgg = 0.0;
  for (j = n; j--;) {
    g[j] = -g[j];
    h[j] = g[j];
#if defined(QPS_PRECON)
    h[j] *= p->priv_pcgt[j];
#endif
    dgg += g[j] * h[j];
  }

  for (i = 0; i < 2 * n; i++) {

#if (QPS_DEBUG > 5)
    fprintf(p->priv_fp, "### qps_cgmin() top\n");
    for (j = 0; j < p->priv_n; j++) {
      fprintf(p->priv_fp, "%f\n", p->priv_cp[j]);
    }
#endif

    if (dgg == 0.0) {
      break;
    }
    qps_linmin(p, dgg, h);
    qps_settp(p);
    p->priv_f = qps_func(p);
    if (fabs((p->priv_f) - fp) <=
	(fabs(p->priv_f) + fabs(fp) + QPS_EPS) * QPS_TOL / 2.0) {
      break;
    }
    fp = p->priv_f;
    qps_dfunc(p, xi);
    gg = dgg;
    dgg = 0.0;
    for (j = n; j--;) {
      t = xi[j] * xi[j];
#if defined(QPS_PRECON)
      t *= p->priv_pcgt[j];
#endif
      dgg += t;
    }
    gam = dgg / gg;
    for (j = n; j--;) {
      g[j] = -xi[j];
      t = g[j];
#if defined(QPS_PRECON)
      t *= p->priv_pcgt[j];
#endif
      h[j] = t + gam * h[j];
    }
  }
#if (QPS_DEBUG > 0)
  fprintf(p->priv_fp, "### CG ITERS=%d %d %d\n", i, p->cog_num, p->loop_num);
#endif
  if (i == 2 * n) {
    fprintf(stderr, "### Too many iterations in qps_cgmin()\n");
#if defined(QPS_DEBUG)
    fprintf(p->priv_fp, "### Too many iterations in qps_cgmin()\n");
#endif
  }
}

/**********************************************************************/

void
qps_init(qps_problem_t * p)
{
  int i, j;
  int pr, pw;

#if defined(QPS_DEBUG)
  p->priv_fp = fopen(QPS_DEBUG_FILE, "a");
  assert(p->priv_fp);
#endif

#if (QPS_DEBUG > 5)
  fprintf(p->priv_fp, "### n=%d gn=%d ln=%d\n", p->num_cells, p->cog_num,
	  p->loop_num);
  pr = 0;
  fprintf(p->priv_fp, "### (c w) values\n");
  for (i = 0; i < p->num_cells; i++) {
    fprintf(p->priv_fp, "net %d: ", i);
    while (p->connect[pr] >= 0) {
      fprintf(p->priv_fp, "(%d %f) ", p->connect[pr], p->edge_weight[pr]);
      pr++;
    }
    fprintf(p->priv_fp, "(-1 -1.0)\n");
    pr++;
  }
  fprintf(p->priv_fp, "### (x y f) values\n");
  for (i = 0; i < p->num_cells; i++) {
    fprintf(p->priv_fp, "cell %d: (%f %f %d)\n", i, p->x[i], p->y[i],
	    p->fixed[i]);
  }
#if 0
  if (p->cog_num) {
    fprintf(p->priv_fp, "### ga values\n");
    for (i = 0; i < p->num_cells; i++) {
      fprintf(p->priv_fp, "cell %d: (%f)\n", i, p->area[i]);
    }
  }
  pr = 0;
  fprintf(p->priv_fp, "### gl values\n");
  for (i = 0; i < p->cog_num; i++) {
    fprintf(p->priv_fp, "cog %d: ", i);
    while (p->cog_list[pr] >= 0) {
      fprintf(p->priv_fp, "%d ", p->cog_list[pr]);
      pr++;
    }
    fprintf(p->priv_fp, "-1\n");
    pr++;
  }
  fprintf(p->priv_fp, "### (gx gy) values\n");
  for (i = 0; i < p->cog_num; i++) {
    fprintf(p->priv_fp, "cog %d: (%f %f)\n", i, p->cog_x[i], p->cog_y[i]);
  }
#endif
#endif /* QPS_DEBUG */

  p->priv_ii = (int *)malloc(p->num_cells * sizeof(int));
  assert(p->priv_ii);

  p->max_enable = 0;

  p->priv_fopt = 0.0;

  /* canonify c and w */
  pr = pw = 0;
  for (i = 0; i < p->num_cells; i++) {
    while ((j = p->connect[pr]) >= 0) {
      if (j > i) {
	pw++;
      }
      pr++;
    }
    pw++;
    pr++;
  }
  p->priv_cc = (int *)malloc(pw * sizeof(int));
  assert(p->priv_cc);
  p->priv_cr = (int *)malloc(p->num_cells * sizeof(int));
  assert(p->priv_cr);
  p->priv_cw = (qps_float_t*)malloc(pw * sizeof(qps_float_t));
  assert(p->priv_cw);
  p->priv_ct = (qps_float_t*)malloc(pw * sizeof(qps_float_t));
  assert(p->priv_ct);
  p->priv_cm = pw;
  pr = pw = 0;
  for (i = 0; i < p->num_cells; i++) {
    p->priv_cr[i] = pw;
    while ((j = p->connect[pr]) >= 0) {
      if (j > i) {
	p->priv_cc[pw] = p->connect[pr];
	p->priv_ct[pw] = p->edge_weight[pr];
	pw++;
      }
      pr++;
    }
    p->priv_cc[pw] = -1;
    p->priv_ct[pw] = -1.0;
    pw++;
    pr++;
  }
  assert(pw == p->priv_cm);

  /* temp arrays for function eval */
  p->priv_tp = (qps_float_t *) malloc(4 * p->num_cells * sizeof(qps_float_t));
  assert(p->priv_tp);
  p->priv_tp2 = p->priv_tp + 2 * p->num_cells;
}

/**********************************************************************/

static qps_float_t
qps_estopt(qps_problem_t * p)
{
  int i, j, cell;
  qps_float_t r;
  qps_float_t *t1, *t2;
  qps_float_t t;

  if (p->max_enable) {
    r = 0.0;
    t1 = (qps_float_t *) malloc(2 * p->num_cells * sizeof(qps_float_t));
#if (QPS_DEBUG > 0)
    assert(t1);
#endif
    for (i = 2 * p->num_cells; i--;) {
      t1[i] = 0.0;
    }
    j = 0;
    for (i = 0; i < p->cog_num; i++) {
      while ((cell = p->cog_list[j]) >= 0) {
	t1[cell * 2] = p->cog_x[i];
	t1[cell * 2 + 1] = p->cog_y[i];
	j++;
      }
      j++;
    }
    t2 = p->priv_tp;
    p->priv_tp = t1;
    r = qps_func(p);
    p->priv_tp = t2;
    free(t1);
    t = (p->max_x * p->max_x + p->max_y * p->max_y);
    t *= p->num_cells;
    for (i = p->num_cells; i--;) {
      if (p->fixed[i]) {
	r += t;
      }
    }
  }
  else {
    r = p->priv_f;
  }
  if (p->loop_num) {
    /* FIXME hacky */
    r *= 8.0;
  }
  return r;
}

/**********************************************************************/

static void
qps_solve_inner(qps_problem_t * p)
{
  int i;
  qps_float_t t;
  qps_float_t z;
  qps_float_t pm1, pm2, tp;
  qps_float_t *tw;
#if defined(QPS_HOIST)
  int j, k;
  qps_float_t jx, jy, kx, ky, sx, sy, tx, ty;
  int pr, st;
#endif

  tw = p->priv_cw;
#if defined(QPS_HOIST)
  if (!p->loop_num) {
    p->priv_cw = p->priv_ct;
  }
  else {
    for(i=p->priv_cm; i--;) {
      p->priv_cw[i] = p->priv_ct[i];
    }
    /* augment with loop penalties */
    pr = 0;
    for (i = 0; i < p->loop_num; i++) {
      while ((j = p->priv_la[pr++]) != -1) {
	if (j >= 0) {
	  p->priv_cw[j] += p->loop_penalty[i];
	}
      }
      pr++;
    }
  }
#else /* !QPS_HOIST */
  p->priv_cw = p->priv_ct;
#endif /* QPS_HOIST */

  qps_cgmin(p);

  if (p->max_enable || p->loop_num) {
    if (p->max_enable == 1 || (p->loop_num && p->loop_k == 0)) {
      p->priv_eps = 2.0;
      p->priv_fmax = p->priv_f;
      p->priv_fprev = p->priv_f;
      p->priv_fopt = qps_estopt(p);
      p->priv_pn = 0;
      p->loop_fail = 0;
    }
    else {
      if (p->priv_f < p->priv_fprev &&
	  (p->priv_fprev - p->priv_f) >
	  QPS_DEC_CHANGE * fabs(p->priv_fprev)) {
	if (p->priv_pn++ >= QPS_STEPSIZE_RETRIES) {
	  p->priv_eps /= 2.0;
	  p->priv_pn = 0;
	}
      }
      p->priv_fprev = p->priv_f;
      if (p->priv_fmax < p->priv_f) {
	p->priv_fmax = p->priv_f;
      }
      if (p->priv_f >= p->priv_fopt) {
	p->priv_fopt = p->priv_fmax * 2.0;
	p->loop_fail |= 2;
#if (QPS_DEBUG > 0)
	fprintf(p->priv_fp, "### warning: changing fopt\n");
#endif
      }
    }
#if (QPS_DEBUG > 0)
    fprintf(p->priv_fp, "### max_stat %.2e %.2e %.2e %.2e\n",
	    p->priv_f, p->priv_eps, p->priv_fmax, p->priv_fopt);
    fflush(p->priv_fp);
#endif
  }

  p->loop_done = 1;
  if (p->loop_num) {
#if (QPS_DEBUG > 0)
    fprintf(p->priv_fp, "### begin_update %d\n", p->loop_k);
#endif
    p->loop_k++;

#if defined(QPS_HOIST)
    /* calc loop penalties */
    pr = 0;
    for (i = 0; i < p->loop_num; i++) {
      t = 0.0;
      j = st = p->loop_list[pr++];
      jx = sx = p->priv_tp[j * 2];
      jy = sy = p->priv_tp[j * 2 + 1];
      while ((k = p->loop_list[pr]) >= 0) {
	kx = p->priv_tp[k * 2];
	ky = p->priv_tp[k * 2 + 1];
	tx = jx - kx;
	ty = jy - ky;
	t += tx * tx + ty * ty;
	j = k;
	jx = kx;
	jy = ky;
	pr++;
      }
      tx = jx - sx;
      ty = jy - sy;
      t += tx * tx + ty * ty;
      p->priv_lt[i] = t - p->loop_max[i];
      pr++;
    }
#endif /* QPS_HOIST */

    /* check KKT conditions */
#if (QPS_DEBUG > 1)
    for (i = p->loop_num; i--;) {
      if (p->loop_penalty[i] != 0.0) {
	fprintf(p->priv_fp, "### penalty %d %.2e\n", i, p->loop_penalty[i]);
      }
    }
#endif
    t = 0.0;
    for (i = p->loop_num; i--;) {
      if (p->priv_lt[i] > 0.0 || p->loop_penalty[i] > 0.0) {
	t += p->priv_lt[i] * p->priv_lt[i];
      }
      if (fabs(p->priv_lt[i]) < QPS_LOOP_TOL) {
#if (QPS_DEBUG > 4)
	fprintf(p->priv_fp, "### skip %d %f\n", i, p->priv_lt[i]);
#endif
	continue;
      }
      z = QPS_LOOP_TOL * p->loop_max[i];
      if (p->priv_lt[i] > z || (p->loop_k < QPS_RELAX_ITER &&
				p->loop_penalty[i] * p->priv_lt[i] < -z)) {
	p->loop_done = 0;
#if (QPS_DEBUG > 1)
	fprintf(p->priv_fp, "### not_done %d %f %f %f %f\n", i,
		p->priv_lt[i], z, p->loop_max[i], p->loop_penalty[i]);
#endif
      }
#if (QPS_DEBUG > 5)
      else {
	fprintf(p->priv_fp, "### done %d %f %f %f %f\n", i,
		p->priv_lt[i], z, p->loop_max[i], p->loop_penalty[i]);
      }
#endif
    }
    /* update penalties */
    if (!p->loop_done) {
      t = p->priv_eps * (p->priv_fopt - p->priv_f) / t;
      tp = 0.0;
      for (i = p->loop_num; i--;) {
	pm1 = p->loop_penalty[i];
#if (QPS_DEBUG > 5)
	fprintf(p->priv_fp, "### update %d %.2e %.2e %.2e %.2e %.2e\n", i,
		t, p->priv_lt[i], t * p->priv_lt[i], pm1, p->loop_max[i]);
#endif
	p->loop_penalty[i] += t * p->priv_lt[i];
	if (p->loop_penalty[i] < 0.0) {
	  p->loop_penalty[i] = 0.0;
	}
	pm2 = p->loop_penalty[i];
	tp += fabs(pm1 - pm2);
      }
#if (QPS_DEBUG > 4)
      fprintf(p->priv_fp, "### penalty mag %f\n", tp);
#endif
    }
  }

  p->max_done = 1;
  if (p->max_enable) {
#if (QPS_DEBUG > 4)
    fprintf(p->priv_fp, "### begin_max_update %d\n", p->max_enable);
#endif
    t = 0.0;
    for (i = p->num_cells; i--;) {
      z = -(p->x[i]);
      t += z * z;
      if (z > QPS_TOL || (p->max_enable < QPS_RELAX_ITER &&
			  p->priv_mxl[i] * z < -QPS_MAX_TOL)) {
	p->max_done = 0;
#if (QPS_DEBUG > 4)
	fprintf(p->priv_fp, "### nxl %d %f %f\n", i, z, p->priv_mxl[i]);
#endif
      }
      z = (p->x[i] - p->max_x);
      t += z * z;
      if (z > QPS_TOL || (p->max_enable < QPS_RELAX_ITER &&
			  p->priv_mxh[i] * z < -QPS_MAX_TOL)) {
	p->max_done = 0;
#if (QPS_DEBUG > 4)
	fprintf(p->priv_fp, "### nxh %d %f %f\n", i, z, p->priv_mxh[i]);
#endif
      }
      z = -(p->y[i]);
      t += z * z;
      if (z > QPS_TOL || (p->max_enable < QPS_RELAX_ITER &&
			  p->priv_myl[i] * z < -QPS_MAX_TOL)) {
	p->max_done = 0;
#if (QPS_DEBUG > 4)
	fprintf(p->priv_fp, "### nyl %d %f %f\n", i, z, p->priv_myl[i]);
#endif
      }
      z = (p->y[i] - p->max_y);
      t += z * z;
      if (z > QPS_TOL || (p->max_enable < QPS_RELAX_ITER &&
			  p->priv_myh[i] * z < -QPS_MAX_TOL)) {
	p->max_done = 0;
#if (QPS_DEBUG > 4)
	fprintf(p->priv_fp, "### nyh %d %f %f\n", i, z, p->priv_myh[i]);
#endif
      }
    }
#if (QPS_DEBUG > 4)
    fprintf(p->priv_fp, "### max_done %d %f\n", p->max_done, t);
#endif
    if (!p->max_done) {
      t = p->priv_eps * (p->priv_fopt - p->priv_f) / t;
      tp = 0.0;
      for (i = p->num_cells; i--;) {
	z = -(p->x[i]);
	pm1 = p->priv_mxl[i];
	p->priv_mxl[i] += t * z;
	if (p->priv_mxl[i] < 0.0) {
	  p->priv_mxl[i] = 0.0;
	}
	pm2 = p->priv_mxl[i];
	tp += fabs(pm1 - pm2);

	z = (p->x[i] - p->max_x);
	pm1 = p->priv_mxh[i];
	p->priv_mxh[i] += t * z;
	if (p->priv_mxh[i] < 0.0) {
	  p->priv_mxh[i] = 0.0;
	}
	pm2 = p->priv_mxh[i];
	tp += fabs(pm1 - pm2);

	z = -(p->y[i]);
	pm1 = p->priv_myl[i];
	p->priv_myl[i] += t * z;
	if (p->priv_myl[i] < 0.0) {
	  p->priv_myl[i] = 0.0;
	}
	pm2 = p->priv_myl[i];
	tp += fabs(pm1 - pm2);

	z = (p->y[i] - p->max_y);
	pm1 = p->priv_myh[i];
	p->priv_myh[i] += t * z;
	if (p->priv_myh[i] < 0.0) {
	  p->priv_myh[i] = 0.0;
	}
	pm2 = p->priv_myh[i];
	tp += fabs(pm1 - pm2);
      }
    }
#if (QPS_DEBUG > 4)
    for (i = p->num_cells; i--;) {
      fprintf(p->priv_fp, "### max_penalty %d %f %f %f %f\n", i,
	      p->priv_mxl[i], p->priv_mxh[i], p->priv_myl[i], p->priv_myh[i]);
    }
#endif
    p->max_enable++;
  }

  if (p->loop_k >= QPS_MAX_ITER || p->priv_eps < QPS_MINSTEP) {
    p->loop_fail |= 1;
  }

  if (p->loop_fail) {
    p->loop_done = 1;
  }

  p->priv_cw = tw;
}

/**********************************************************************/

void
qps_solve(qps_problem_t * p)
{
  int i, j;
  int pr, pw;
  qps_float_t bk;
  int tk;

#if defined(QPS_PRECON)
  int c;
  qps_float_t t;
#endif

#if defined(QPS_HOIST)
  int k;
  int st;
  int m1, m2;
#endif

  if (p->max_enable) {
    p->priv_mxl = (qps_float_t *)
	malloc(4 * p->num_cells * sizeof(qps_float_t));
    assert(p->priv_mxl);
    p->priv_mxh = p->priv_mxl + p->num_cells;
    p->priv_myl = p->priv_mxl + 2 * p->num_cells;
    p->priv_myh = p->priv_mxl + 3 * p->num_cells;
    for (i = 4 * p->num_cells; i--;) {
      p->priv_mxl[i] = 0.0;
    }
  }

  /* flag fixed cells with -1 */
  for (i = p->num_cells; i--;) {
    p->priv_ii[i] = (p->fixed[i]) ? (-1) : (0);
  }

  /* read gl and set up dependent variables */
  if (p->cog_num) {
    p->priv_gt = (int *)malloc(p->cog_num * sizeof(int));
    assert(p->priv_gt);
    p->priv_gm = (qps_float_t*)malloc(p->cog_num * sizeof(qps_float_t));
    assert(p->priv_gm);
    pr = 0;
    for (i = 0; i < p->cog_num; i++) {
      tk = -1;
      bk = -1.0;
      pw = pr;
      while ((j = p->cog_list[pr++]) >= 0) {
	if (!p->fixed[j]) {
	  /* use largest entry for numerical stability; see Gordian paper */
	  if (p->area[j] > bk) {
	    tk = j;
	    bk = p->area[j];
	  }
	}
      }
      assert(bk > 0.0);
      /* dependent variables have index=(-2-COG_constraint) */
      p->priv_ii[tk] = -2 - i;
      p->priv_gt[i] = pw;
      p->priv_gm[i] = bk;
    }
    p->priv_gw = (qps_float_t*)malloc(pr * sizeof(qps_float_t));
    assert(p->priv_gw);
    pr = 0;
    for (i = 0; i < p->cog_num; i++) {
      while ((j = p->cog_list[pr]) >= 0) {
	p->priv_gw[pr] = p->area[j] / p->priv_gm[i];
	pr++;
      }
      p->priv_gw[pr] = -1.0;
      pr++;
    }
  }

  /* set up indexes from independent floating cells to variables */
  p->priv_n = 0;
  for (i = p->num_cells; i--;) {
    if (!p->priv_ii[i]) {
      p->priv_ii[i] = 2 * (p->priv_n++);
    }
  }
  p->priv_n *= 2;

#if (QPS_DEBUG > 5)
  for (i = 0; i < p->num_cells; i++) {
    fprintf(p->priv_fp, "### ii %d %d\n", i, p->priv_ii[i]);
  }
#endif

#if defined(QPS_PRECON)
  p->priv_pcg = (qps_float_t *) malloc(p->num_cells * sizeof(qps_float_t));
  assert(p->priv_pcg);
  p->priv_pcgt = (qps_float_t *) malloc(p->priv_n * sizeof(qps_float_t));
  assert(p->priv_pcgt);
  for (i = p->num_cells; i--;) {
    p->priv_pcg[i] = 0.0;
  }
  pr = 0;
  for (i = 0; i < p->num_cells; i++) {
    while ((c = p->priv_cc[pr]) >= 0) {
      t = p->priv_ct[pr];
      p->priv_pcg[i] += t;
      p->priv_pcg[c] += t;
      pr++;
    }
    pr++;
  }
  pr = 0;
  for (i = 0; i < p->loop_num; i++) {
    t = 2.0 * p->loop_penalty[i];
    while ((c = p->loop_list[pr++]) >= 0) {
      p->priv_pcg[c] += t;
    }
    pr++;
  }
#if (QPS_DEBUG > 6)
  for (i = p->num_cells; i--;) {
    fprintf(p->priv_fp, "### precon %d %.2e\n", i, p->priv_pcg[i]);
  }
#endif
  for (i = p->priv_n; i--;) {
      p->priv_pcgt[i] = 0.0;
  }
  for (i = 0; i < p->num_cells; i++) {
    c = p->priv_ii[i];
    if (c >= 0) {
      t = p->priv_pcg[i];
      p->priv_pcgt[c] += t;
      p->priv_pcgt[c + 1] += t;
    }
#if 0
    else if (c < -1) {
      pr = p->priv_gt[-(c+2)];
      while ((j = p->cog_list[pr++]) >= 0) {
	ji = p->priv_ii[j];
	if (ji >= 0) {
	  w = p->area[j] / p->area[i];
	  t = w * w * p->priv_pcg[i];
	  p->priv_pcgt[ji] += t;
	  p->priv_pcgt[ji + 1] += t;
	}
      }
    }
#endif
  }
  for (i = 0; i < p->priv_n; i++) {
    t = p->priv_pcgt[i];
    if (fabs(t) < QPS_PRECON_EPS || fabs(t) > 1.0/QPS_PRECON_EPS) {
      p->priv_pcgt[i] = 1.0;
    }
    else {
      p->priv_pcgt[i] = 1.0 / p->priv_pcgt[i];
    }
  }
#endif

  /* allocate variable storage */
  p->priv_cp = (qps_float_t *) malloc(4 * p->priv_n * sizeof(qps_float_t));
  assert(p->priv_cp);

  /* temp arrays for cg */
  p->priv_g = p->priv_cp + p->priv_n;
  p->priv_h = p->priv_cp + 2 * p->priv_n;
  p->priv_xi = p->priv_cp + 3 * p->priv_n;

  /* set values */
  for (i = p->num_cells; i--;) {
    if (p->priv_ii[i] >= 0) {
      p->priv_cp[p->priv_ii[i]] = p->x[i];
      p->priv_cp[p->priv_ii[i] + 1] = p->y[i];
    }
  }

  if (p->loop_num) {
    p->priv_lt = (qps_float_t *) malloc(p->loop_num * sizeof(qps_float_t));
    assert(p->priv_lt);
#if defined(QPS_HOIST)
    pr = 0;
    for (i=p->loop_num; i--;) {
      while (p->loop_list[pr++] >= 0) {
      }
      pr++;
    }
    p->priv_lm = pr;
    p->priv_la = (int *) malloc(pr * sizeof(int));
    assert(p->priv_la);
    pr = 0;
    for (i = 0; i < p->loop_num; i++) {
      j = st = p->loop_list[pr++];
      while ((k = p->loop_list[pr]) >= 0) {
	if (j > k) {
	  m1 = k;
	  m2 = j;
	}
	else {
	  assert(k > j);
	  m1 = j;
	  m2 = k;
	}
	pw = p->priv_cr[m1];
	while (p->priv_cc[pw] != m2) {
/*	  assert(p->priv_cc[pw] >= 0); */
	  if (p->priv_cc[pw] < 0) {
	    pw = -2;
	    break;
	  }
	  pw++;
	}
	p->priv_la[pr-1] = pw;
	j = k;
	pr++;
      }
      if (j > st) {
	m1 = st;
	m2 = j;
      }
      else {
	assert(st > j);
	m1 = j;
	m2 = st;
      }
      pw = p->priv_cr[m1];
      while (p->priv_cc[pw] != m2) {
/*	assert(p->priv_cc[pw] >= 0); */
	if (p->priv_cc[pw] < 0) {
	  pw = -2;
	  break;
	}
	pw++;
      }
      p->priv_la[pr-1] = pw;
      p->priv_la[pr] = -1;
      pr++;
    }
#endif /* QPS_HOIST */
  }

  do {
    qps_solve_inner(p);
  } while (!p->loop_done || !p->max_done);

  /* retrieve values */
  /* qps_settp() should have already been called at this point */
  for (i = p->num_cells; i--;) {
    p->x[i] = p->priv_tp[i * 2];
    p->y[i] = p->priv_tp[i * 2 + 1];
  }
#if (QPS_DEBUG > 5)
  for (i = p->num_cells; i--;) {
    fprintf(p->priv_fp, "### cloc %d %f %f\n", i, p->x[i], p->y[i]);
  }
#endif

  free(p->priv_cp);
  if (p->max_enable) {
    free(p->priv_mxl);
  }
  if (p->cog_num) {
    free(p->priv_gt);
    free(p->priv_gm);
    free(p->priv_gw);
  }
  if(p->loop_num) {
    free(p->priv_lt);
#if defined(QPS_HOIST)
    free(p->priv_la);
#endif
  }

#if defined(QPS_PRECON)
  free(p->priv_pcg);
  free(p->priv_pcgt);
#endif
}

/**********************************************************************/

void
qps_clean(qps_problem_t * p)
{
  free(p->priv_tp);
  free(p->priv_ii);
  free(p->priv_cc);
  free(p->priv_cr);
  free(p->priv_cw);
  free(p->priv_ct);

#if defined(QPS_DEBUG)
  fclose(p->priv_fp);
#endif /* QPS_DEBUG */
}

/**********************************************************************/
ABC_NAMESPACE_IMPL_END

