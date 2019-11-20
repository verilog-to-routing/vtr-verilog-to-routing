/*===================================================================*/
//  
//     place_bin.c
//
//		Aaron P. Hurst, 2007
//              ahurst@eecs.berkeley.edu
//
/*===================================================================*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <assert.h>

//#define DEBUG

#include "place_base.h"

// --------------------------------------------------------------------
// Global variables
//
// --------------------------------------------------------------------


// --------------------------------------------------------------------
// Function prototypes and local data structures
//
// --------------------------------------------------------------------

void spreadDensityX(int numBins, float maxMovement);
void spreadDensityY(int numBins, float maxMovement);


// --------------------------------------------------------------------
// globalFixDensity()
//
/// Doesn't deal well with fixed cells in the core area.
// --------------------------------------------------------------------
void globalFixDensity(int numBins, float maxMovement) {
  
  printf("QCLN-10 : \tbin-based density correction\n");
    
  spreadDensityX(numBins, maxMovement);
  // spreadDensityY(numBins, maxMovement);
}


// --------------------------------------------------------------------
// spreadDensityX()
//
// --------------------------------------------------------------------
void spreadDensityX(int numBins, float maxMovement) {

  int c, c2, c3, x, y;
  float totalArea = 0;
  int moveableCells = 0;
  float yBinArea = 0,  yCumArea = 0;
  int   yBinStart = 0, yBinCount = 0;
  int  xBinCount, xBinStart;
  float xBinArea, xCumArea;
  float lastOldEdge;
  float lastNewEdge;
  float curOldEdge, curNewEdge;
  float stretch, w;
  ConcreteCell *xCell, *yCell;
  ConcreteCell **binCells;
  ConcreteCell **allCells;

  binCells = (ConcreteCell **)malloc(sizeof(ConcreteCell*)*g_place_numCells);
  allCells = (ConcreteCell **)malloc(sizeof(ConcreteCell*)*g_place_numCells);

  for(c=0; c<g_place_numCells; c++) if (g_place_concreteCells[c]) {
    ConcreteCell *cell =  g_place_concreteCells[c];
    if (!cell->m_fixed && !cell->m_parent->m_pad) {
      allCells[moveableCells++] = cell;
      totalArea += getCellArea(cell);
    }
  }
  
  // spread X
  qsort(allCells, moveableCells, sizeof(ConcreteCell*), cellSortByY);

  y = 0;

  // for each y-bin...
  for(c=0; c<moveableCells; c++) {
    yCell = allCells[c];
    yBinArea += getCellArea(yCell);
    yCumArea += getCellArea(yCell);
    yBinCount++;

    // have we filled up a y-bin?
    if (yCumArea >= totalArea*(y+1)/numBins && yBinArea > 0) {
      memcpy(binCells, &(allCells[yBinStart]), sizeof(ConcreteCell*)*yBinCount);
      qsort(binCells, yBinCount, sizeof(ConcreteCell*), cellSortByX);

#if defined(DEBUG)
      printf("y-bin %d count=%d area=%f\n",y,yBinCount, yBinArea);
#endif

      x = 0;
      xBinCount = 0, xBinStart =  0;
      xBinArea = 0, xCumArea = 0;
      lastOldEdge = g_place_coreBounds.x;
      lastNewEdge = g_place_coreBounds.x;

      // for each x-bin...
      for(c2=0; c2<yBinCount; c2++) {
        xCell = binCells[c2];
        xBinArea += getCellArea(xCell);
        xCumArea += getCellArea(xCell);
        xBinCount++;
        curOldEdge = xCell->m_x;
        
        printf("%.3f ", xCell->m_x);

        // have we filled up an x-bin?
        if (xCumArea >= yBinArea*(x+1)/numBins && xBinArea > 0) {
          curNewEdge = lastNewEdge + g_place_coreBounds.w*xBinArea/yBinArea;

          if (curNewEdge > g_place_coreBounds.x+g_place_coreBounds.w) 
            curNewEdge = g_place_coreBounds.x+g_place_coreBounds.w;
          if ((curNewEdge-curOldEdge)>maxMovement) curNewEdge = curOldEdge + maxMovement;
          if ((curOldEdge-curNewEdge)>maxMovement) curNewEdge = curOldEdge - maxMovement;

#if defined(DEBUG)
          printf("->\tx-bin %d count=%d area=%f (%f,%f)->(%f,%f)\n",x, xBinCount, xBinArea,
                 curOldEdge, lastOldEdge, curNewEdge, lastNewEdge);
#endif
          
          stretch = (curNewEdge-lastNewEdge)/(curOldEdge-lastOldEdge);
          
          // stretch!
          for(c3=xBinStart; c3<xBinStart+xBinCount; c3++) {
            if (curOldEdge == lastOldEdge)
              binCells[c3]->m_x = lastNewEdge+(c3-xBinStart)*(curNewEdge-lastNewEdge);
            else
              binCells[c3]->m_x = lastNewEdge+(binCells[c3]->m_x-lastOldEdge)*stretch;
            
              // force within core
            w = binCells[c3]->m_parent->m_width*0.5;
            if (binCells[c3]->m_x-w < g_place_coreBounds.x)
              binCells[c3]->m_x = g_place_coreBounds.x+w;
            if (binCells[c3]->m_x+w > g_place_coreBounds.x+g_place_coreBounds.w)
              binCells[c3]->m_x = g_place_coreBounds.x+g_place_coreBounds.w-w;
          }
          
          lastOldEdge = curOldEdge;
          lastNewEdge = curNewEdge;
          x++;
          xBinCount = 0;
          xBinArea = 0;
          xBinStart = c2+1;
        }
      }
      
      y++;
      yBinCount = 0;
      yBinArea = 0;
      yBinStart = c+1;
    }
  }

  free(binCells);
  free(allCells);
}


// --------------------------------------------------------------------
// spreadDensityY()
//
// --------------------------------------------------------------------
void spreadDensityY(int numBins, float maxMovement) {

  int c, c2, c3, x, y;
  float totalArea = 0;
  int moveableCells = 0;
  float xBinArea = 0,  xCumArea = 0;
  int   xBinStart = 0, xBinCount = 0;
  int   yBinCount, yBinStart;
  float yBinArea, yCumArea;
  float lastOldEdge;
  float lastNewEdge;
  float curOldEdge, curNewEdge;
  float stretch, h;
  ConcreteCell *xCell, *yCell;
  ConcreteCell **binCells;
  ConcreteCell **allCells;

  binCells = (ConcreteCell **)malloc(sizeof(ConcreteCell*)*g_place_numCells);
  allCells = (ConcreteCell **)malloc(sizeof(ConcreteCell*)*g_place_numCells);

  for(c=0; c<g_place_numCells; c++) if (g_place_concreteCells[c]) {
    ConcreteCell *cell =  g_place_concreteCells[c];
    if (!cell->m_fixed && !cell->m_parent->m_pad) {
      allCells[moveableCells++] = cell;
      totalArea += getCellArea(cell);
    }
  }
  
  // spread Y
  qsort(allCells, moveableCells, sizeof(ConcreteCell*), cellSortByX);

  x = 0;

  // for each x-bin...
  for(c=0; c<moveableCells; c++) {
    xCell = allCells[c];
    xBinArea += getCellArea(xCell);
    xCumArea += getCellArea(xCell);
    xBinCount++;

    // have we filled up an x-bin?
    if (xCumArea >= totalArea*(x+1)/numBins && xBinArea > 0) {
      memcpy(binCells, &(allCells[xBinStart]), sizeof(ConcreteCell*)*xBinCount);
      qsort(binCells, xBinCount, sizeof(ConcreteCell*), cellSortByY);

      // printf("x-bin %d count=%d area=%f\n",y,yBinCount, yBinArea);

      y = 0;
      yBinCount = 0, yBinStart =  0;
      yBinArea = 0, yCumArea = 0;
      lastOldEdge = g_place_coreBounds.y;
      lastNewEdge = g_place_coreBounds.y;
      
      // for each y-bin...
      for(c2=0; c2<xBinCount; c2++) {
        yCell = binCells[c2];
        yBinArea += getCellArea(yCell);
        yCumArea += getCellArea(yCell);
        yBinCount++;
        curOldEdge = yCell->m_y;
        
        // have we filled up an x-bin?
        if (yCumArea >= xBinArea*(y+1)/numBins && yBinArea > 0) {
          curNewEdge = lastNewEdge + g_place_coreBounds.h*yBinArea/xBinArea;

          if (curNewEdge > g_place_coreBounds.y+g_place_coreBounds.h) 
            curNewEdge = g_place_coreBounds.y+g_place_coreBounds.h;
          if ((curNewEdge-curOldEdge)>maxMovement) curNewEdge = curOldEdge + maxMovement;
          if ((curOldEdge-curNewEdge)>maxMovement) curNewEdge = curOldEdge - maxMovement;

          if (curOldEdge == lastOldEdge) continue; // hmmm
          stretch = (curNewEdge-lastNewEdge)/(curOldEdge-lastOldEdge);

          // stretch!
          for(c3=yBinStart; c3<yBinStart+yBinCount; c3++) {
            binCells[c3]->m_y = lastNewEdge+(binCells[c3]->m_y-lastOldEdge)*stretch;

            // force within core
            h = binCells[c3]->m_parent->m_height;
            if (binCells[c3]->m_y-h < g_place_coreBounds.y)
              binCells[c3]->m_y = g_place_coreBounds.y+h;
            if (binCells[c3]->m_y+h > g_place_coreBounds.y+g_place_coreBounds.h)
              binCells[c3]->m_y = g_place_coreBounds.y+g_place_coreBounds.h-h;
          }

          lastOldEdge = curOldEdge;
          lastNewEdge = curNewEdge;
          y++;
          yBinCount = 0;
          yBinArea = 0;
          yBinStart = c2+1;
        }
      }

      x++;
      xBinCount = 0;
      xBinArea = 0;
      xBinStart = c+1;
    }
  }

  free(binCells);
  free(allCells);
}
