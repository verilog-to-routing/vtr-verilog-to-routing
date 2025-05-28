/*===================================================================*/
//  
//     place_test.c
//
//        Aaron P. Hurst, 2003-2007
//              ahurst@eecs.berkeley.edu
//
/*===================================================================*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "place_base.h"

ABC_NAMESPACE_IMPL_START



// --------------------------------------------------------------------
// Hash type/functions
//
// --------------------------------------------------------------------

struct hash_element {
  ConcreteCell        *obj;
  struct hash_element *next;
} hash_element;

int hash_string(int hash_max, const char *str) {
  unsigned int hash = 0;
  int p;
  for(p = 0; p<strlen(str); p++)
    hash += str[p]*p;
  return hash % hash_max;
}

void hash_add(struct hash_element **hash, int hash_max,
              ConcreteCell *cell) {
  int key = hash_string(hash_max, cell->m_label);
  // printf("adding %s key = %d\n", cell->m_label, key);
  struct hash_element *element = malloc(sizeof(struct hash_element));
  assert(element);
  element->obj = cell;
  element->next = hash[key];
  hash[key] = element;
}

ConcreteCell *hash_find(struct hash_element **hash, int hash_max, const char *str) {
  int key = hash_string(hash_max, str);
  // printf("looking for %s key = %d\n", str, key);
  struct hash_element *next = hash[key];
  while(next) {
    if (!strcmp(str, next->obj->m_label))
      return next->obj;
    next = next->next;
  }
  return 0;
}

// --------------------------------------------------------------------
// Global variables
//
// --------------------------------------------------------------------

struct hash_element **hash_cellname;

int numCells = 0, numNets = 0;

AbstractCell *abstractCells;
ConcreteCell *concreteCells;
ConcreteNet  *concreteNets;

// --------------------------------------------------------------------
// Function implementations
//
// --------------------------------------------------------------------

void readBookshelfNets(char *filename) {
  char *tok;
  char buf[1024];
  const char *DELIMITERS = " \n\t:";
  int id = 0;
  int t;
  ConcreteCell *cell;

  FILE *netsFile = fopen(filename, "r");
  if (!netsFile) {
    printf("ERROR: Could not open .nets file\n");
    exit(1);
  }

  // line 1 : version
  while (fgets(buf, 1024, netsFile) && (buf[0] == '\n' || buf[0] == '#'));

  // line 2 : number of nets
  while (fgets(buf, 1024, netsFile) && (buf[0] == '\n' || buf[0] == '#'));
  tok = strtok(buf, DELIMITERS);
  tok = strtok(NULL, DELIMITERS);
  numNets = atoi(tok);
  printf("READ-20 : number of nets = %d\n", numNets);
  concreteNets = malloc(sizeof(ConcreteNet)*numNets);

  // line 3 : number of pins
  while (fgets(buf, 1024, netsFile) && (buf[0] == '\n' || buf[0] == '#'));

  // line XXX : net definitions
  while(fgets(buf, 1024, netsFile)) {
    if (buf[0] == '\n' || buf[0] == '#') continue;

    concreteNets[id].m_id = id;
    concreteNets[id].m_weight = 1.0;

    tok = strtok(buf, DELIMITERS);
    if (!!strcmp(tok, "NetDegree")) {
      printf("%s\n",buf);
      printf("ERROR: Incorrect format in .nets file\n");
      exit(1);
    }

    tok = strtok(NULL, DELIMITERS);
    concreteNets[id].m_numTerms = atoi(tok);
    if (concreteNets[id].m_numTerms < 0 ||
        concreteNets[id].m_numTerms > 100000) {
      printf("ERROR: Bad net degree\n");
      exit(1); 
    }
    concreteNets[id].m_terms = malloc(sizeof(ConcreteCell*)*
         concreteNets[id].m_numTerms);

    // read terms
    t = 0;
    while(t < concreteNets[id].m_numTerms &&
          fgets(buf, 1024, netsFile)) {
      if (buf[0] == '\n' || buf[0] == '#') continue;
      
      // cell name
      tok = strtok(buf, DELIMITERS);
      cell = hash_find(hash_cellname, numCells, tok);
      if (!cell) {
        printf("ERROR: Could not find cell %s in .nodes file\n", tok);
        exit(1);
      }
      concreteNets[id].m_terms[t] = cell;
      t++;
    }

    // add!
    addConcreteNet(&(concreteNets[id]));

    id++;
  }

  fclose(netsFile);
}

void readBookshelfNodes(char *filename) {
  char *tok;
  char buf[1024];
  const char *DELIMITERS = " \n\t:";
  int id = 0;

  FILE *nodesFile = fopen(filename, "r");
  if (!nodesFile) {
    printf("ERROR: Could not open .nodes file\n");
    exit(1);
  }

  // line 1 : version
  while (fgets(buf, 1024, nodesFile) && (buf[0] == '\n' || buf[0] == '#'));

  // line 2 : num nodes
  while (fgets(buf, 1024, nodesFile) && (buf[0] == '\n' || buf[0] == '#'));
  tok = strtok(buf, DELIMITERS);
  tok = strtok(NULL, DELIMITERS);
  numCells = atoi(tok);
  printf("READ-10 : number of cells = %d\n", numCells);
  concreteCells = malloc(sizeof(ConcreteCell)*numCells);
  abstractCells = malloc(sizeof(AbstractCell)*numCells);
  hash_cellname = calloc(numCells, sizeof(struct hash_element*));

  // line 3 : num terminals
  while (fgets(buf, 1024, nodesFile) && (buf[0] == '\n' || buf[0] == '#'));

  // line XXX : cell definitions
  while(fgets(buf, 1024, nodesFile)) {
    if (buf[0] == '\n' || buf[0] == '#') continue;

    tok = strtok(buf, DELIMITERS);
    concreteCells[id].m_id = id;;
    
    // label
    concreteCells[id].m_parent = &(abstractCells[id]);
    concreteCells[id].m_label = malloc(sizeof(char)*strlen(tok)+1);
    strcpy(concreteCells[id].m_label, tok);
    abstractCells[id].m_label = concreteCells[id].m_label;
    hash_add(hash_cellname, numCells, 
             &(concreteCells[id]));

    // dimensions
    tok = strtok(NULL, DELIMITERS);
    abstractCells[id].m_width = atof(tok);
    tok = strtok(NULL, DELIMITERS);
    abstractCells[id].m_height = atof(tok);
    tok = strtok(NULL, DELIMITERS);
    // terminal
    abstractCells[id].m_pad = tok && !strcmp(tok, "terminal");

    // add!
    addConcreteCell(&(concreteCells[id]));

    // DEBUG
    /*
    printf("\"%s\" : %f x %f\n", concreteCells[id].m_label,
           abstractCells[id].m_width,
           abstractCells[id].m_height);
    */
    id++;
  }

  fclose(nodesFile);
}

void readBookshelfPlacement(char *filename) {
  char *tok;
  char buf[1024];
  const char *DELIMITERS = " \n\t:";
  ConcreteCell *cell;

  FILE *plFile = fopen(filename, "r");
  FILE *netsFile = fopen(filename, "r");
  if (!plFile) {
    printf("ERROR: Could not open .pl file\n");
    exit(1);
  }
  if (!netsFile) {
    printf("ERROR: Could not open .nets file\n");
    exit(1);
  }

  // line 1 : version
  while (fgets(buf, 1024, plFile) && (buf[0] == '\n' || buf[0] == '#'));

  // line XXX : placement definitions
  while(fgets(buf, 1024, plFile)) {
    if (buf[0] == '\n' || buf[0] == '#') continue;

    tok = strtok(buf, DELIMITERS);

    // cell name
    cell = hash_find(hash_cellname, numCells, tok);
    if (!cell) {
      printf("ERROR: Could not find cell %s in .nodes file\n",tok);
      exit(1);
    }

    // position
    tok = strtok(NULL, DELIMITERS);
    cell->m_x = atof(tok);
    tok = strtok(NULL, DELIMITERS);
    cell->m_y = atof(tok);

    // hfixed
    cell->m_fixed = strtok(NULL, DELIMITERS) && 
      (tok = strtok(NULL, DELIMITERS)) &&
      !strcmp(tok, "\\FIXED");
  }
  
  fclose(plFile);
}

void writeBookshelfPlacement(char *filename) {
  int c = 0;

  FILE *plFile = fopen(filename, "w");
  if (!plFile) {
    printf("ERROR: Could not open .pl file\n");
    exit(1);
  }

  fprintf(plFile, "UCLA pl 1.0\n");
  for(c=0; c<numCells; c++) {
    fprintf(plFile, "%s %f %f : N %s\n", 
            concreteCells[c].m_label,
            concreteCells[c].m_x,
            concreteCells[c].m_y,
            (concreteCells[c].m_fixed ? "\\FIXED" : ""));
  }

  fclose(plFile);
}

// deletes all connections to a cell
void delNetConnections(ConcreteCell *cell) {
  int n, t, t2, count = 0;
  ConcreteCell **old = malloc(sizeof(ConcreteCell*)*g_place_numCells);

  for(n=0; n<g_place_numNets; n++) if (g_place_concreteNets[n]) {
    ConcreteNet *net = g_place_concreteNets[n];
    count = 0;
    for(t=0; t<net->m_numTerms; t++)
      if (net->m_terms[t] == cell) count++;
    if (count) {
      memcpy(old, net->m_terms, sizeof(ConcreteCell*)*net->m_numTerms);
      net->m_terms = realloc(net->m_terms, 
                             sizeof(ConcreteCell*)*(net->m_numTerms-count));
      t2 = 0;
      for(t=0; t<net->m_numTerms; t++)
        if (old[t] != cell) net->m_terms[t2++] = old[t];
      net->m_numTerms -= count;
    }
  }
  free(old);
}

int main(int argc, char **argv) {

  if (argc != 4) {
    printf("Usage: %s [nodes] [nets] [pl]\n", argv[0]);
    exit(1);
  }

  readBookshelfNodes(argv[1]);
  readBookshelfNets(argv[2]);
  readBookshelfPlacement(argv[3]);

  globalPreplace(0.8);
  globalPlace();

  // DEBUG net/cell removal/addition
  /*
  int i;
  for(i=1000; i<2000; i++) {
    delConcreteNet(g_place_concreteNets[i]);
    delNetConnections(g_place_concreteCells[i]);
    delConcreteCell(g_place_concreteCells[i]);
  }
  
  ConcreteCell newCell[2];
  newCell[0].m_id = g_place_numCells+1;
  newCell[0].m_x = 1000;
  newCell[0].m_y = 1000;
  newCell[0].m_fixed = false;
  newCell[0].m_parent = &(abstractCells[1000]);
  newCell[0].m_label = " ";
  addConcreteCell(&newCell[0]);
  newCell[1].m_id = g_place_numCells+3;
  newCell[1].m_x = 1000;
  newCell[1].m_y = 1000;
  newCell[1].m_fixed = false;
  newCell[1].m_parent = &(abstractCells[1000]);
  newCell[1].m_label = " ";
  addConcreteCell(&newCell[1]);
  */

  globalIncremental();

  writeBookshelfPlacement(argv[3]);

  free(hash_cellname);

  return 0;
}
ABC_NAMESPACE_IMPL_END

