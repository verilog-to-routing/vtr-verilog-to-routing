/*===================================================================*/
//  
//     place_inc.c
//
//        Aaron P. Hurst, 2003-2007
//              ahurst@eecs.berkeley.edu
//
/*===================================================================*/

#include <stdlib.h>
#include <limits.h>
#include <assert.h>
#include <string.h>

#include "place_base.h"
#include "place_gordian.h"

ABC_NAMESPACE_IMPL_START


inline int sqHashId(int id, int max) {
  return ((id * (id+17)) % max);
}

#if 0
// --------------------------------------------------------------------
// fastPlace()
//
/// The first cell is assumed to be the "output".
// --------------------------------------------------------------------
float fastPlace(int numCells, ConcreteCell *cells[], 
                int numNets, ConcreteNet *nets[]) {
  
  int n, t, c, i, local_id = 0, pass;
  const int NUM_PASSES = 4;
  int *cell_numTerms = calloc(numCells, sizeof(int));
  ConcreteNet **cell_terms;
  ConcreteNet  *net;
  Rect outputBox;

  outputBox = getNetBBox(nets[0]);

  // assign local ids
  // put cells in reasonable initial location
  for(n=0; n<numNets; n++)
    for(t=0; nets[n]->m_numTerms; t++)
      nets[n]->m_terms[t]->m_data = -1;

  for(c=0; c<numCells; c++) {
    cells[c]->m_data = local_id;
    cells[c]->m_x = outputBox.x + 0.5*outputBox.w;
    cells[c]->m_y = outputBox.y + 0.5*outputBox.h;
  }

  // build reverse map of cells to nets
  for(n=0; n<numNets; n++)
    for(t=0; nets[n]->m_numTerms; t++) {
      local_id = nets[n]->m_terms[t]->m_data;
      if (local_id >= 0)
        cell_numTerms[local_id]++;
    }

  for(c=0; c<numCells; c++) {
    cell_terms[c] = malloc(sizeof(ConcreteNet*)*cell_numTerms[c]);
    cell_numTerms[c] = 0;
  }

  for(n=0; n<numNets; n++)
    for(t=0; nets[n]->m_numTerms; t++) {
      local_id = nets[n]->m_terms[t]->m_data;
      if (local_id >= 0)
        cell_terms[cell_numTerms[local_id]++] = nets[n];
    }

  // topological order?
  
  // iterative linear 
  for(pass=0; pass<NUM_PASSES; pass++)
    for(c=0; c<numCells; c++) {
      for(n=0; n<cell_numTerms[c]; n++) {
        net = cell_terms[c];
        for(t=0; t<net->m_numTerms; t++);
      }
    }
}
#endif

// --------------------------------------------------------------------
// fastEstimate()
//
// --------------------------------------------------------------------
float fastEstimate(ConcreteCell *cell, 
                   int numNets, ConcreteNet *nets[]) {
  float len = 0;
  int n;
  Rect box;

  assert(cell);

  for(n=0; n<numNets; n++) {
    box = getNetBBox(nets[n]);
    if (cell->m_x < box.x) len += (box.x - cell->m_x);
    if (cell->m_x > box.x+box.w) len += (cell->m_x-box.x-box.w);
    if (cell->m_y < box.y) len += (box.x - cell->m_y);
    if (cell->m_y > box.y+box.h) len += (cell->m_y-box.y-box.h);
  }
  
  return len;
}
ABC_NAMESPACE_IMPL_END

