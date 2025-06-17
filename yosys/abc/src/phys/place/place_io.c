/*===================================================================*/
//  
//     place_io.c
//
//        Aaron P. Hurst, 2003-2007
//              ahurst@eecs.berkeley.edu
//
/*===================================================================*/

#include <stdlib.h>
#include <limits.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>

#include "place_base.h"

ABC_NAMESPACE_IMPL_START



// --------------------------------------------------------------------
// writeBookshelfNodes()
//
// --------------------------------------------------------------------
void writeBookshelfNodes(const char *filename) {
  
  int c = 0;
  int numNodes, numTerms;

  FILE *nodesFile = fopen(filename, "w");
  if (!nodesFile) {
    printf("ERROR: Could not open .nodes file\n");
    exit(1);
  }

  numNodes = numTerms = 0;
  for(c=0; c<g_place_numCells; c++) if (g_place_concreteCells[c]) {
      numNodes++;
      if (g_place_concreteCells[c]->m_parent->m_pad)
        numTerms++;
    }



  fprintf(nodesFile, "UCLA nodes 1.0\n");
  fprintf(nodesFile, "NumNodes : %d\n", numNodes);
  fprintf(nodesFile, "NumTerminals : %d\n", numTerms);

  for(c=0; c<g_place_numCells; c++) if (g_place_concreteCells[c]) {
    fprintf(nodesFile, "CELL%d %f %f %s\n", 
            g_place_concreteCells[c]->m_id,
            g_place_concreteCells[c]->m_parent->m_width,
            g_place_concreteCells[c]->m_parent->m_height,
            (g_place_concreteCells[c]->m_parent->m_pad ? " terminal" : ""));
  }

  fclose(nodesFile);  
}


// --------------------------------------------------------------------
// writeBookshelfPl()
//
// --------------------------------------------------------------------
void writeBookshelfPl(const char *filename) {
  
  int c = 0;

  FILE *plFile = fopen(filename, "w");
  if (!plFile) {
    printf("ERROR: Could not open .pl file\n");
    exit(1);
  }

  fprintf(plFile, "UCLA pl 1.0\n");
  for(c=0; c<g_place_numCells; c++)  if (g_place_concreteCells[c]) {
    fprintf(plFile, "CELL%d %f %f : N %s\n", 
            g_place_concreteCells[c]->m_id,
            g_place_concreteCells[c]->m_x,
            g_place_concreteCells[c]->m_y,
            (g_place_concreteCells[c]->m_fixed ? "\\FIXED" : ""));
  }

  fclose(plFile);  

}


// --------------------------------------------------------------------
// writeBookshelf()
//
// --------------------------------------------------------------------
void writeBookshelf(const char *filename) {
  writeBookshelfNodes("out.nodes");
  writeBookshelfPl("out.pl");
}
ABC_NAMESPACE_IMPL_END

