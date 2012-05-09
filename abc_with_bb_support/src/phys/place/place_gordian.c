/*===================================================================*/
//  
//     place_gordian.c
//
//		Aaron P. Hurst, 2003-2007
//              ahurst@eecs.berkeley.edu
//
/*===================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include <limits.h>

#include "place_gordian.h"
#include "place_base.h"


// --------------------------------------------------------------------
// Global variables
//
// --------------------------------------------------------------------

int g_place_numPartitions;


// --------------------------------------------------------------------
// globalPlace()
//
/// \brief Performs analytic placement using a GORDIAN-like algorithm.
//
/// Updates the positions of all non-fixed non-pad cells.
///
// --------------------------------------------------------------------
void globalPlace() {
  bool completionFlag = false;
  int iteration = 0;  

  printf("PLAC-10 : Global placement (wirelength-driven Gordian)\n");

  initPartitioning();

  // build matrices representing interconnections
  printf("QMAN-00 : \tconstructing initial quadratic problem...\n");
  constructQuadraticProblem();

  // iterate placement until termination condition is met
  while(!completionFlag) {
    printf("QMAN-01 : \titeration %d numPartitions = %d\n",iteration,g_place_numPartitions);
    
    // do the global optimization in each direction
    printf("QMAN-01 : \t\tglobal optimization\n");
    solveQuadraticProblem(!IGNORE_COG);
      
    // -------- PARTITIONING BASED CELL SPREADING ------

    // bisection
    printf("QMAN-01 : \t\tpartition refinement\n");
    if (REALLOCATE_PARTITIONS) reallocPartitions();
    completionFlag |= refinePartitions();
      
    printf("QMAN-01 : \t\twirelength = %e\n", getTotalWirelength());
      
    iteration++;
  }
  
  // final global optimization
  printf("QMAN-02 : \t\tfinal pass\n");
  if (FINAL_REALLOCATE_PARTITIONS) reallocPartitions();
  solveQuadraticProblem(!IGNORE_COG);
  printf("QMAN-01 : \t\twirelength = %e\n", getTotalWirelength());

  // clean up
  sanitizePlacement();
  printf("QMAN-01 : \t\twirelength = %e\n", getTotalWirelength());
  globalFixDensity(25, g_place_rowHeight*5);
  printf("QMAN-01 : \t\twirelength = %e\n", getTotalWirelength());
}


// --------------------------------------------------------------------
// globalIncremental()
//
/// \brief Performs analytic placement using a GORDIAN-like algorithm.
//
/// Requires a valid set of partitions.
///
// --------------------------------------------------------------------

void   globalIncremental() {
  if (!g_place_rootPartition) {
    printf("WARNING: Can not perform incremental placement\n");
    globalPlace();
    return;
  }

  printf("PLAC-10 : Incremental global placement\n");

  incrementalPartition();

  printf("QMAN-00 : \tconstructing initial quadratic problem...\n");
  constructQuadraticProblem();

  solveQuadraticProblem(!IGNORE_COG);
  printf("QMAN-01 : \t\twirelength = %e\n", getTotalWirelength());
  
  // clean up
  sanitizePlacement();
  printf("QMAN-01 : \t\twirelength = %e\n", getTotalWirelength());
  globalFixDensity(25, g_place_rowHeight*5);
  printf("QMAN-01 : \t\twirelength = %e\n", getTotalWirelength());
}


// --------------------------------------------------------------------
// sanitizePlacement()
//
/// \brief Moves any cells that are outside of the core bounds to the nearest location within.
//
// --------------------------------------------------------------------
void sanitizePlacement() { 
  int c;
  float order_width = g_place_rowHeight;
  float x, y, edge, w, h;
  
  printf("QCLN-10 : \tsanitizing placement\n");

  for(c=0; c<g_place_numCells; c++) if (g_place_concreteCells[c]) {
    ConcreteCell *cell = g_place_concreteCells[c];
    if (cell->m_fixed || cell->m_parent->m_pad) {
      continue;
    }
    // the new locations of the cells will be distributed within
    // a small margin inside the border so that ordering is preserved
    order_width = g_place_rowHeight;

    x = cell->m_x, y = cell->m_y,
      w = cell->m_parent->m_width, h = cell->m_parent->m_height;

    if ((edge=x-w*0.5) < g_place_coreBounds.x) {
      x = g_place_coreBounds.x+w*0.5 +
        order_width/(1.0+g_place_coreBounds.x-edge);
    }
    else if ((edge=x+w*0.5) > g_place_coreBounds.x+g_place_coreBounds.w) {
      x = g_place_coreBounds.x+g_place_coreBounds.w-w*0.5 -
        order_width/(1.0+edge-g_place_coreBounds.x-g_place_coreBounds.w);
    }
    if ((edge=y-h*0.5) < g_place_coreBounds.y) {
      y = g_place_coreBounds.y+h*0.5 +
        order_width/(1.0+g_place_coreBounds.y-edge);
    }
    else if ((edge=y+h*0.5) > g_place_coreBounds.y+g_place_coreBounds.h) {
      y = g_place_coreBounds.y+g_place_coreBounds.h-h*0.5 -
        order_width/(1.0+edge-g_place_coreBounds.x-g_place_coreBounds.w);
    }
    cell->m_x = x;
    cell->m_y = y;
  }
}
