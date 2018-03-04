/*===================================================================*/
//  
//     place_base.c
//
//		Aaron P. Hurst, 2003-2007
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


// --------------------------------------------------------------------
// Global variables
//
// --------------------------------------------------------------------

int   g_place_numCells = 0;
int   g_place_numNets = 0;
float g_place_rowHeight = 1.0;

Rect  g_place_coreBounds;
Rect  g_place_padBounds;

ConcreteCell **g_place_concreteCells = NULL;
int            g_place_concreteCellsSize = 0;
ConcreteNet  **g_place_concreteNets = NULL;
int            g_place_concreteNetsSize = 0;


// --------------------------------------------------------------------
// getNetBBox()
//
/// \brief Returns the bounding box of a net.
//
// --------------------------------------------------------------------
Rect   getNetBBox(const ConcreteNet *net) {
  int t;
  Rect r;

  assert(net);

  r.x = r.y = INT_MAX;
  r.w = r.h = -INT_MAX;
  for(t=0; t<net->m_numTerms; t++) {
    r.x = net->m_terms[t]->m_x < r.x ? net->m_terms[t]->m_x : r.x;
    r.y = net->m_terms[t]->m_y < r.y ? net->m_terms[t]->m_y : r.y;
    r.w = net->m_terms[t]->m_x > r.w ? net->m_terms[t]->m_x : r.w;
    r.h = net->m_terms[t]->m_y > r.h ? net->m_terms[t]->m_y : r.h;
  }
  r.w -= r.x; r.h -= r.y;
  return r;
}


// --------------------------------------------------------------------
// getNetWirelength()
//
/// \brief Returns the half-perimeter wirelength of a net.
//
// --------------------------------------------------------------------
float getNetWirelength(const ConcreteNet *net) {
  Rect r;

  assert(net);

  r = getNetBBox(net);
  return r.w+r.h;
}


// --------------------------------------------------------------------
// getTotalWirelength()
//
/// \brief Returns the total HPWL of all nets.
//
// --------------------------------------------------------------------
float getTotalWirelength() {
  float r = 0;
  int n;
  for(n=0; n<g_place_numNets; n++) if (g_place_concreteNets[n])
    r += getNetWirelength(g_place_concreteNets[n]);
  return r;
}


// --------------------------------------------------------------------
// getCellArea()
//
// --------------------------------------------------------------------
float getCellArea(const ConcreteCell *cell) {
  assert(cell);
  return cell->m_parent->m_width*cell->m_parent->m_height;
}


// --------------------------------------------------------------------
// addConcreteNet()
//
/// \brief Adds a net to the placement database.
///
/// The net object must already be allocated and the ID must be set
/// appropriately.
//
// --------------------------------------------------------------------
void   addConcreteNet(ConcreteNet *net) {
  assert(net);
  assert(net->m_id >= 0);
  if (net->m_id >= g_place_concreteNetsSize) {
    g_place_concreteNetsSize = (net->m_id > g_place_concreteNetsSize ? 
                               net->m_id : g_place_concreteNetsSize);
    g_place_concreteNetsSize *= 1.5;
    g_place_concreteNetsSize += 20;
    g_place_concreteNets = (ConcreteNet**)realloc(g_place_concreteNets, 
             sizeof(ConcreteNet*)*g_place_concreteNetsSize);
    assert(g_place_concreteNets);
  }
  if (net->m_id >= g_place_numNets) {
    memset(&(g_place_concreteNets[g_place_numNets]), 0,
            sizeof(ConcreteNet*)*(net->m_id+1-g_place_numNets));
    g_place_numNets = net->m_id+1;
    assert(g_place_numNets <= g_place_concreteNetsSize);
  }
  g_place_concreteNets[net->m_id] = net;
}


// --------------------------------------------------------------------
// delConcreteNet()
//
/// Does not deallocate memory.
// --------------------------------------------------------------------
void   delConcreteNet(ConcreteNet *net) {
  assert(net);
  g_place_concreteNets[net->m_id] = 0;
  while(!g_place_concreteNets[g_place_numNets-1]) g_place_numNets--;
}


// --------------------------------------------------------------------
// addConcreteCell()
//
/// The cell object must already be allocated and the ID must be set
/// appropriately.
//
// --------------------------------------------------------------------
void   addConcreteCell(ConcreteCell *cell) {
  assert(cell);
  assert(cell->m_id >= 0);
  if (cell->m_id >= g_place_concreteCellsSize) {
    g_place_concreteCellsSize = (cell->m_id > g_place_concreteCellsSize ? 
                                 cell->m_id : g_place_concreteCellsSize);
    g_place_concreteCellsSize *= 1.5;
    g_place_concreteCellsSize += 20;
    g_place_concreteCells = (ConcreteCell**)realloc(g_place_concreteCells, 
          sizeof(ConcreteCell*)*g_place_concreteCellsSize);
    assert(g_place_concreteCells);
  }
  if (cell->m_id >= g_place_numCells) {
    memset(&(g_place_concreteCells[g_place_numCells]), 0,
          sizeof(ConcreteCell*)*(cell->m_id+1-g_place_numCells));
    g_place_numCells = cell->m_id+1;
  }
  g_place_concreteCells[cell->m_id] = cell;
}


// --------------------------------------------------------------------
// delCellFromPartition()
//
// --------------------------------------------------------------------
void delCellFromPartition(ConcreteCell *cell, Partition *p) {
  int c;
  bool found = false;

  assert(cell);
  assert(p);

  for(c=0; c<p->m_numMembers; c++) 
    if (p->m_members[c] == cell) {
      p->m_members[c] = 0;
      p->m_area -= getCellArea(cell);
      found = true;
      break;
    }
     
  if (!found) return;

  if (!p->m_leaf) {
    delCellFromPartition(cell, p->m_sub1);
    delCellFromPartition(cell, p->m_sub2);
  }
}


// --------------------------------------------------------------------
// delConcreteCell()
//
/// \brief Removes a cell from the placement database.
///
/// Does not deallocate memory.
///
/// Important: does not modify nets that may point to this
/// cell. If these are connections are not removed, segmentation faults 
/// and other nasty errors will occur.
//
// --------------------------------------------------------------------
void   delConcreteCell(ConcreteCell *cell) {
  assert(cell);
  g_place_concreteCells[cell->m_id] = 0;
  while(!g_place_concreteCells[g_place_numCells-1]) g_place_numCells--;

  if (g_place_rootPartition) delCellFromPartition(cell, g_place_rootPartition);
}


// --------------------------------------------------------------------
// netSortByX...
//
/// \brief Sorts nets by position of one of its corners.
//
/// These are for use with qsort().
///
/// Can tolerate pointers to NULL objects.
///
// --------------------------------------------------------------------
int netSortByL(const void *a, const void *b) {
  const ConcreteNet *pa = *(const ConcreteNet **)a;
  const ConcreteNet *pb = *(const ConcreteNet **)b;
  Rect ba, bb;

  if (!pa && !pb) return 0;
  else if (!pa) return -1;
  else if (!pb) return 1;
  ba = getNetBBox(pa), bb = getNetBBox(pb);
  if (ba.x < bb.x) return -1;
  if (ba.x > bb.x) return 1;
  return 0;
}

int netSortByR(const void *a, const void *b) {
  const ConcreteNet *pa = *(const ConcreteNet **)a;
  const ConcreteNet *pb = *(const ConcreteNet **)b;
  Rect ba, bb;

  if (!pa && !pb) return 0;
  else if (!pa) return -1;
  else if (!pb) return 1;
  ba = getNetBBox(pa), bb = getNetBBox(pb);
  if (ba.x + ba.w < bb.x + bb.w) return -1;
  if (ba.x + ba.w > bb.x + bb.w) return 1;
  return 0;
}

int netSortByB(const void *a, const void *b) {
  const ConcreteNet *pa = *(const ConcreteNet **)a;
  const ConcreteNet *pb = *(const ConcreteNet **)b;
  Rect ba, bb;

  if (!pa && !pb) return 0;
  else if (!pa) return -1;
  else if (!pb) return 1;
  ba = getNetBBox(pa), bb = getNetBBox(pb);
  if (ba.y + ba.h < bb.y + bb.h) return -1;
  if (ba.y + ba.h > bb.y + bb.h) return 1;
  return 0;
}

int netSortByT(const void *a, const void *b) {
  const ConcreteNet *pa = *(const ConcreteNet **)a;
  const ConcreteNet *pb = *(const ConcreteNet **)b;
  Rect ba, bb;

  if (!pa && !pb) return 0;
  else if (!pa) return -1;
  else if (!pb) return 1;
  ba = getNetBBox(pa), bb = getNetBBox(pb);
  if (ba.y < bb.y) return -1;
  if (ba.y > bb.y) return 1;
  return 0;
}

int netSortByID(const void *a, const void *b) {
  const ConcreteNet *pa = *(const ConcreteNet **)a;
  const ConcreteNet *pb = *(const ConcreteNet **)b;

  if (!pa && !pb) return 0;
  else if (!pa) return -1;
  else if (!pb) return 1;
  if (pa->m_id < pb->m_id) return -1;
  if (pa->m_id > pb->m_id) return 1;
  return 0;
}


// --------------------------------------------------------------------
// cellSortByX...
//
/// \brief Sorts cells by either position coordinate.
//
/// These are for use with qsort().
///
/// Can tolerate pointers to NULL objects.
//
// --------------------------------------------------------------------
int cellSortByX(const void *a, const void *b) {
  const ConcreteCell *pa = *(const ConcreteCell **)a;
  const ConcreteCell *pb = *(const ConcreteCell **)b;

  if (!pa && !pb) return 0;
  else if (!pa) return -1;
  else if (!pb) return 1;
  if (pa->m_x < pb->m_x) return -1;
  if (pa->m_x > pb->m_x) return 1;
  return 0;
}

int cellSortByY(const void *a, const void *b) {
  const ConcreteCell *pa = *(const ConcreteCell **)a;
  const ConcreteCell *pb = *(const ConcreteCell **)b;

  if (!pa && !pb) return 0;
  else if (!pa) return -1;
  else if (!pb) return 1;
  if (pa->m_y < pb->m_y) return -1;
  if (pa->m_y > pb->m_y) return 1;
  return 0;
}

int cellSortByID(const void *a, const void *b) {
  const ConcreteCell *pa = *(const ConcreteCell **)a;
  const ConcreteCell *pb = *(const ConcreteCell **)b;

  if (!pa && !pb) return 0;
  else if (!pa) return -1;
  else if (!pb) return 1;
  if (pa->m_id < pb->m_id) return -1;
  if (pa->m_id > pb->m_id) return 1;
  return 0;
}
ABC_NAMESPACE_IMPL_END

