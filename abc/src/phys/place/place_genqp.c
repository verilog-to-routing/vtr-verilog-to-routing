/*===================================================================*/
//  
//     place_genqp.c
//
//        Aaron P. Hurst, 2003-2007
//              ahurst@eecs.berkeley.edu
//
/*===================================================================*/

#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "place_base.h"
#include "place_qpsolver.h"
#include "place_gordian.h"

ABC_NAMESPACE_IMPL_START


// --------------------------------------------------------------------
// Global variables
//
// --------------------------------------------------------------------

qps_problem_t *g_place_qpProb = NULL;


// --------------------------------------------------------------------
// splitPenalty()
//
/// \brief Returns a weight for all of the edges in the clique for a multipin net.
//
// --------------------------------------------------------------------
float splitPenalty(int pins) {

  if (pins > 1) {
    return 1.0 + CLIQUE_PENALTY/(pins - 1);
    // return pow(pins - 1, CLIQUE_PENALTY);
  }
  return 1.0 + CLIQUE_PENALTY;
}


// --------------------------------------------------------------------
// constructQuadraticProblem()
//
/// \brief Constructs the matrices necessary to do analytical placement.
//
// --------------------------------------------------------------------
void constructQuadraticProblem() {
  int maxConnections = 1;
  int ignoreNum = 0;
  int n,t,c,c2,p;
  ConcreteCell  *cell;
  ConcreteNet   *net;
  int           *cell_numTerms = calloc(g_place_numCells, sizeof(int));
  ConcreteNet ***cell_terms = calloc(g_place_numCells, sizeof(ConcreteNet**));
  bool incremental = false;
  int nextIndex = 1;
  int *seen = calloc(g_place_numCells, sizeof(int));
  float weight;
  int last_index;

  // create problem object
  if (!g_place_qpProb) {
    g_place_qpProb = malloc(sizeof(qps_problem_t));
    g_place_qpProb->area = NULL;
    g_place_qpProb->x = NULL;
    g_place_qpProb->y = NULL;
    g_place_qpProb->fixed = NULL;
    g_place_qpProb->connect = NULL;
    g_place_qpProb->edge_weight = NULL;
  }

  // count the maximum possible number of non-sparse entries
  for(n=0; n<g_place_numNets; n++) if (g_place_concreteNets[n]) {
    ConcreteNet *net = g_place_concreteNets[n];
    if (net->m_numTerms > IGNORE_NETSIZE) {
      ignoreNum++;
    }
    else {
      maxConnections += net->m_numTerms*(net->m_numTerms-1);
      for(t=0; t<net->m_numTerms; t++) {
        c = net->m_terms[t]->m_id;
        p = ++cell_numTerms[c];
        cell_terms[c] = (ConcreteNet**)realloc(cell_terms[c], p*sizeof(ConcreteNet*));
        cell_terms[c][p-1] = net;
      }
    }
  }
  if(ignoreNum) {
    printf("QMAN-10 : \t\t%d large nets ignored\n", ignoreNum);
  }

  // initialize the data structures
  g_place_qpProb->num_cells = g_place_numCells;
  maxConnections += g_place_numCells + 1;

  g_place_qpProb->area        = realloc(g_place_qpProb->area,
                                       sizeof(float)*g_place_numCells);// "area" matrix
  g_place_qpProb->edge_weight = realloc(g_place_qpProb->edge_weight,
                                       sizeof(float)*maxConnections);  // "weight" matrix
  g_place_qpProb->connect     = realloc(g_place_qpProb->connect,
                                       sizeof(int)*maxConnections);    // "connectivity" matrix
  g_place_qpProb->fixed       = realloc(g_place_qpProb->fixed,
                                       sizeof(int)*g_place_numCells);  // "fixed" matrix

  // initialize or keep preexisting locations
  if (g_place_qpProb->x != NULL && g_place_qpProb->y != NULL) {
    printf("QMAN-10 :\tperforming incremental placement\n");
    incremental = true;
  }
  g_place_qpProb->x = (float*)realloc(g_place_qpProb->x, sizeof(float)*g_place_numCells);
  g_place_qpProb->y = (float*)realloc(g_place_qpProb->y, sizeof(float)*g_place_numCells);

  // form a row for each cell
  // build data
  for(c = 0; c < g_place_numCells; c++) if (g_place_concreteCells[c]) {
    cell = g_place_concreteCells[c];
    
    // fill in the characteristics for this cell
    g_place_qpProb->area[c] = getCellArea(cell);
    if (cell->m_fixed || cell->m_parent->m_pad) {
      g_place_qpProb->x[c] = cell->m_x;
      g_place_qpProb->y[c] = cell->m_y;
      g_place_qpProb->fixed[c] = 1;
    } else {
      if (!incremental) {
        g_place_qpProb->x[c] = g_place_coreBounds.x+g_place_coreBounds.w*0.5;
        g_place_qpProb->y[c] = g_place_coreBounds.y+g_place_coreBounds.h*0.5;
      }
      g_place_qpProb->fixed[c] = 0;
    }

    // update connectivity matrices
    last_index = nextIndex;
    for(n=0; n<cell_numTerms[c]; n++) {
      net = cell_terms[c][n];
      weight = net->m_weight / splitPenalty(net->m_numTerms);
      for(t=0; t<net->m_numTerms; t++) {
        c2 = net->m_terms[t]->m_id;
        if (c2 == c) continue;
        if (seen[c2] < last_index) {
          // not seen
          g_place_qpProb->connect[nextIndex-1] = c2;
          g_place_qpProb->edge_weight[nextIndex-1] = weight;
          seen[c2] = nextIndex;
          nextIndex++;
        } else {
          // seen
          g_place_qpProb->edge_weight[seen[c2]-1] += weight;
        }
      }
    }
    g_place_qpProb->connect[nextIndex-1] = -1;
    g_place_qpProb->edge_weight[nextIndex-1] = -1.0;    
    nextIndex++;
  } else {
    // fill in dummy values for connectivity matrices
    g_place_qpProb->connect[nextIndex-1] = -1;
    g_place_qpProb->edge_weight[nextIndex-1] = -1.0;    
    nextIndex++;    
  }

  free(cell_numTerms);
  free(cell_terms);
  free(seen);
}

typedef struct reverseCOG {
  float      x,y;
  Partition *part;
  float      delta;
} reverseCOG;


// --------------------------------------------------------------------
// generateCoGConstraints()
//
/// \brief Generates center of gravity constraints.
//
// --------------------------------------------------------------------
int generateCoGConstraints(reverseCOG COG_rev[]) {
  int numConstraints = 0; // actual num constraints
  int cogRevNum = 0;
  Partition **stack = malloc(sizeof(Partition*)*g_place_numPartitions*2);
  int stackPtr = 0;
  Partition *p;
  float cgx, cgy;
  int next_index = 0, last_constraint = 0;
  bool isTrueConstraint = false;
  int i, m;
  float totarea;
  ConcreteCell *cell;

  // each partition may give rise to a center-of-gravity constraint
  stack[stackPtr] = g_place_rootPartition;
  while(stackPtr >= 0) {
    p = stack[stackPtr--];
    assert(p);

    // traverse down the partition tree to leaf nodes-only
    if (!p->m_leaf) {
      stack[++stackPtr] = p->m_sub1;
      stack[++stackPtr] = p->m_sub2;
    } else {
      /*
      cout << "adding a COG constraint for box "
       << p->bounds.x << ","
       << p->bounds.y << " of size"
       << p->bounds.w << "x"
       << p->bounds.h
       << endl;
      */
      cgx = p->m_bounds.x + p->m_bounds.w*0.5;
      cgy = p->m_bounds.y + p->m_bounds.h*0.5;
      COG_rev[cogRevNum].x = cgx;
      COG_rev[cogRevNum].y = cgy;
      COG_rev[cogRevNum].part = p;
      COG_rev[cogRevNum].delta = 0;

      cogRevNum++;
    }
  }

  assert(cogRevNum == g_place_numPartitions);
  
  for (i = 0; i < g_place_numPartitions; i++) {
    p = COG_rev[i].part;
    assert(p);
    g_place_qpProb->cog_x[numConstraints] = COG_rev[i].x;
    g_place_qpProb->cog_y[numConstraints] = COG_rev[i].y;
    totarea = 0.0;
    for(m=0; m<p->m_numMembers; m++) if (p->m_members[m]) {
      cell = p->m_members[m];
      assert(cell);

      if (!cell->m_fixed && !cell->m_parent->m_pad) {
    isTrueConstraint = true;
      }
      else {
    continue;
      }
      g_place_qpProb->cog_list[next_index++] = cell->m_id;
      totarea += getCellArea(cell);
    }
    if (totarea == 0.0) {
      isTrueConstraint = false;
    }
    if (isTrueConstraint) {
      numConstraints++;
      g_place_qpProb->cog_list[next_index++] = -1;
      last_constraint = next_index;
    }
    else {
      next_index = last_constraint;
    }
  }

  free(stack);

  return --numConstraints;
}


// --------------------------------------------------------------------
// solveQuadraticProblem()
//
/// \brief Calls quadratic solver.
//
// --------------------------------------------------------------------
void solveQuadraticProblem(bool useCOG) {
  int c;

  reverseCOG *COG_rev = malloc(sizeof(reverseCOG)*g_place_numPartitions);

  g_place_qpProb->cog_list = malloc(sizeof(int)*(g_place_numPartitions+g_place_numCells));
  g_place_qpProb->cog_x = malloc(sizeof(float)*g_place_numPartitions);
  g_place_qpProb->cog_y = malloc(sizeof(float)*g_place_numPartitions);

  // memset(g_place_qpProb->x, 0, sizeof(float)*g_place_numCells);
  // memset(g_place_qpProb->y, 0, sizeof(float)*g_place_numCells);

  qps_init(g_place_qpProb);

  if (useCOG)
      g_place_qpProb->cog_num = generateCoGConstraints(COG_rev);
  else
      g_place_qpProb->cog_num = 0;

  g_place_qpProb->loop_num = 0;

  qps_solve(g_place_qpProb);

  qps_clean(g_place_qpProb);

  // set the positions
  for(c = 0; c < g_place_numCells; c++) if (g_place_concreteCells[c]) {
    g_place_concreteCells[c]->m_x = g_place_qpProb->x[c];
    g_place_concreteCells[c]->m_y = g_place_qpProb->y[c];
  }
  
  // clean up
  free(g_place_qpProb->cog_list);
  free(g_place_qpProb->cog_x);
  free(g_place_qpProb->cog_y);

  free(COG_rev);
}
ABC_NAMESPACE_IMPL_END

