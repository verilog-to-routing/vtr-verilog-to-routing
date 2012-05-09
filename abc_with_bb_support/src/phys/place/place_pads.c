/*===================================================================*/
//  
//     place_pads.c
//
//		Aaron P. Hurst, 2003-2007
//              ahurst@eecs.berkeley.edu
//
/*===================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>

#include "place_base.h"

// --------------------------------------------------------------------
// globalPreplace()
//
/// \brief Place pad ring, leaving a core area to meet a desired utilization.
//
/// Sets the position of pads that aren't already fixed.
///
/// Computes g_place_coreBounds and g_place_padBounds.  Determines
/// g_place_rowHeight.
//
// --------------------------------------------------------------------
void globalPreplace(float utilization) {
  int i, c, h, numRows;
  float coreArea = 0, totalArea = 0;
  int padCount = 0;
  float area;
  ConcreteCell **padCells = NULL;
  AbstractCell *padType = NULL;
  ConcreteCell *cell;
  float nextPos;
  int remainingPads, northPads, southPads, eastPads, westPads;

  printf("PLAC-00 : Placing IO pads\n");;

  // identify the pads and compute the total core area
  g_place_coreBounds.x = g_place_coreBounds.y = 0;
  g_place_coreBounds.w = g_place_coreBounds.h = -INT_MAX;

  for(c=0; c<g_place_numCells; c++) if (g_place_concreteCells[c]) {
    cell = g_place_concreteCells[c];
    area = getCellArea(cell);
    if (cell->m_parent->m_pad) {
      padType = cell->m_parent;
    } else {
      coreArea += area;
      g_place_rowHeight = cell->m_parent->m_height;
    }

    if (cell->m_fixed) {
      g_place_coreBounds.x = g_place_coreBounds.x < cell->m_x ? g_place_coreBounds.x : cell->m_x;
      g_place_coreBounds.y = g_place_coreBounds.y < cell->m_y ? g_place_coreBounds.y : cell->m_y;
      g_place_coreBounds.w = g_place_coreBounds.w > cell->m_x ? g_place_coreBounds.w : cell->m_x;
      g_place_coreBounds.h = g_place_coreBounds.h > cell->m_y ? g_place_coreBounds.h : cell->m_y;
    } else if (cell->m_parent->m_pad) {
      padCells = realloc(padCells, sizeof(ConcreteCell **)*(padCount+1));
      padCells[padCount++] = cell;
    }
    totalArea += area;
  }
  if (!padType) {
    printf("ERROR: No pad cells\n");
    exit(1);
  }
  g_place_padBounds.w -= g_place_padBounds.x;
  g_place_padBounds.h -= g_place_padBounds.y;

  coreArea /= utilization;

  // create the design boundaries
  numRows = sqrt(coreArea)/g_place_rowHeight+1;
  h = numRows * g_place_rowHeight;
  g_place_coreBounds.h = g_place_coreBounds.h > h ? g_place_coreBounds.h : h;
  g_place_coreBounds.w = g_place_coreBounds.w > coreArea/g_place_coreBounds.h ? 
    g_place_coreBounds.w : coreArea/g_place_coreBounds.h;
  // increase the dimensions by the width of the padring
  g_place_padBounds = g_place_coreBounds;
  if (padCount) {
    printf("PLAC-05 : \tpreplacing %d pad cells\n", padCount);
    g_place_padBounds.x -= padType->m_width;
    g_place_padBounds.y -= padType->m_height;
    g_place_padBounds.w = g_place_coreBounds.w+2*padType->m_width;
    g_place_padBounds.h = g_place_coreBounds.h+2*padType->m_height;
  }

  printf("PLAC-05 : \tplaceable rows  : %d\n", numRows);
  printf("PLAC-05 : \tcore dimensions : %.0fx%.0f\n",
         g_place_coreBounds.w, g_place_coreBounds.h);
  printf("PLAC-05 : \tchip dimensions : %.0fx%.0f\n",
         g_place_padBounds.w, g_place_padBounds.h);
  
  remainingPads = padCount;
  c = 0;

  // north pads
  northPads = remainingPads/4; remainingPads -= northPads;
  nextPos = 0;
  for(i=0; i<northPads; i++) {
    cell = padCells[c++];
    cell->m_x = g_place_padBounds.x+cell->m_parent->m_width*0.5 + nextPos;
    cell->m_y = g_place_padBounds.y+cell->m_parent->m_height*0.5;
    nextPos += (g_place_padBounds.w-padType->m_width) / northPads;
  }
  
  // south pads
  southPads = remainingPads/3; remainingPads -= southPads;
  nextPos = 0;
  for(i=0; i<southPads; i++) {
    cell = padCells[c++];
    cell->m_x = g_place_padBounds.w+g_place_padBounds.x-cell->m_parent->m_width*0.5 - nextPos;
    cell->m_y = g_place_padBounds.h+g_place_padBounds.y-cell->m_parent->m_height*0.5;
    nextPos += (g_place_padBounds.w-2*padType->m_width) / southPads;
  }

  // east pads
  eastPads = remainingPads/2; remainingPads -= eastPads;
  nextPos = 0;
  for(i=0; i<eastPads; i++) {
    cell = padCells[c++];
    cell->m_x = g_place_padBounds.w+g_place_padBounds.x-cell->m_parent->m_width*0.5;
    cell->m_y = g_place_padBounds.y+cell->m_parent->m_height*0.5 + nextPos;
    nextPos += (g_place_padBounds.h-padType->m_height) / eastPads;
  }

  // west pads
  westPads = remainingPads;
  nextPos = 0;
  for(i=0; i<westPads; i++) {
    cell = padCells[c++];
    cell->m_x = g_place_padBounds.x+cell->m_parent->m_width*0.5;
    cell->m_y = g_place_padBounds.h+g_place_padBounds.y-cell->m_parent->m_height*0.5 - nextPos;
    nextPos += (g_place_padBounds.h-padType->m_height) / westPads;
  }

}

