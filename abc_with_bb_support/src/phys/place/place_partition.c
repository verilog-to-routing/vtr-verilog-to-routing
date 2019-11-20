/*===================================================================*/
//  
//     place_partition.c
//
//		Aaron P. Hurst, 2003-2007
//              ahurst@eecs.berkeley.edu
//
/*===================================================================*/

#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>
#include <assert.h>
//#include <sys/stat.h>
//#include <unistd.h>

#include "place_base.h"
#include "place_gordian.h"

#if !defined(NO_HMETIS)
#include "libhmetis.h"
#endif

// --------------------------------------------------------------------
// Global variables
//
// --------------------------------------------------------------------

Partition *g_place_rootPartition = NULL;
ConcreteNet **allNetsR2 = NULL, 
  **allNetsL2 = NULL, 
  **allNetsB2 = NULL, 
  **allNetsT2 = NULL;


// --------------------------------------------------------------------
// Function prototypes and local data structures
//
// --------------------------------------------------------------------

typedef struct FM_cell {
    int loc;
    int gain;
    ConcreteCell *cell;
    struct FM_cell *next, *prev;
    bool locked;
} FM_cell;

void FM_updateGains(ConcreteNet *net, int partition, int inc, 
                    FM_cell target [], FM_cell *bin [], 
                    int count_1 [], int count_2 []);


// --------------------------------------------------------------------
// initPartitioning()
//
/// \brief Initializes data structures necessary for partitioning.
//
/// Creates a valid g_place_rootPartition.
///
// --------------------------------------------------------------------
void initPartitioning() {
  int i;
  float area;

  // create root partition
  g_place_numPartitions = 1;
  if (g_place_rootPartition) free(g_place_rootPartition);
  g_place_rootPartition = malloc(sizeof(Partition));
  g_place_rootPartition->m_level = 0;
  g_place_rootPartition->m_area = 0;
  g_place_rootPartition->m_bounds = g_place_coreBounds;
  g_place_rootPartition->m_vertical = false;
  g_place_rootPartition->m_done = false;
  g_place_rootPartition->m_leaf = true;
      
  // add all of the cells to this partition
  g_place_rootPartition->m_members = malloc(sizeof(ConcreteCell*)*g_place_numCells);
  g_place_rootPartition->m_numMembers = 0;
  for (i=0; i<g_place_numCells; i++) 
    if (g_place_concreteCells[i]) {
      if (!g_place_concreteCells[i]->m_fixed) {
        area = getCellArea(g_place_concreteCells[i]);
        g_place_rootPartition->m_members[g_place_rootPartition->m_numMembers++] =
          g_place_concreteCells[i];
        g_place_rootPartition->m_area += area;
      }
    }
}


// --------------------------------------------------------------------
// presortNets()
//
/// \brief Sorts nets by corner positions.
//
/// Allocates allNetsX2 structures.
///
// --------------------------------------------------------------------
void presortNets() {
  allNetsL2 = (ConcreteNet**)realloc(allNetsL2, sizeof(ConcreteNet*)*g_place_numNets);
  allNetsR2 = (ConcreteNet**)realloc(allNetsR2, sizeof(ConcreteNet*)*g_place_numNets);
  allNetsB2 = (ConcreteNet**)realloc(allNetsB2, sizeof(ConcreteNet*)*g_place_numNets);
  allNetsT2 = (ConcreteNet**)realloc(allNetsT2, sizeof(ConcreteNet*)*g_place_numNets);
  memcpy(allNetsL2, g_place_concreteNets, sizeof(ConcreteNet*)*g_place_numNets);
  memcpy(allNetsR2, g_place_concreteNets, sizeof(ConcreteNet*)*g_place_numNets);
  memcpy(allNetsB2, g_place_concreteNets, sizeof(ConcreteNet*)*g_place_numNets);
  memcpy(allNetsT2, g_place_concreteNets, sizeof(ConcreteNet*)*g_place_numNets);
  qsort(allNetsL2, g_place_numNets, sizeof(ConcreteNet*), netSortByL);
  qsort(allNetsR2, g_place_numNets, sizeof(ConcreteNet*), netSortByR);
  qsort(allNetsB2, g_place_numNets, sizeof(ConcreteNet*), netSortByB);
  qsort(allNetsT2, g_place_numNets, sizeof(ConcreteNet*), netSortByT);
}

// --------------------------------------------------------------------
// refinePartitions()
//
/// \brief Splits large leaf partitions.
//
// --------------------------------------------------------------------
bool refinePartitions() {

  return refinePartition(g_place_rootPartition);
}


// --------------------------------------------------------------------
// reallocPartitions()
//
/// \brief Reallocates the partitions based on placement information.
//
// --------------------------------------------------------------------
void reallocPartitions() {

  reallocPartition(g_place_rootPartition);
}


// --------------------------------------------------------------------
// refinePartition()
//
/// \brief Splits any large leaves within a partition.
//
// --------------------------------------------------------------------
bool refinePartition(Partition *p) {
  bool degenerate = false;
  int nonzeroCount = 0;
  int i;

  assert(p);

  // is this partition completed?
  if (p->m_done) return true;

  // is this partition a non-leaf node?
  if (!p->m_leaf) {
    p->m_done = refinePartition(p->m_sub1);
    p->m_done &= refinePartition(p->m_sub2);
    return p->m_done;
  }
  
  // leaf...
  // create two new subpartitions
  g_place_numPartitions++;
  p->m_sub1 = malloc(sizeof(Partition));
  p->m_sub1->m_level = p->m_level+1;
  p->m_sub1->m_leaf = true;
  p->m_sub1->m_done = false;
  p->m_sub1->m_area = 0;
  p->m_sub1->m_vertical = !p->m_vertical;
  p->m_sub1->m_numMembers = 0;
  p->m_sub1->m_members = NULL;
  p->m_sub2 = malloc(sizeof(Partition));
  p->m_sub2->m_level = p->m_level+1;
  p->m_sub2->m_leaf = true;
  p->m_sub2->m_done = false;
  p->m_sub2->m_area = 0;
  p->m_sub2->m_vertical = !p->m_vertical;
  p->m_sub2->m_numMembers = 0;
  p->m_sub2->m_members = NULL;
  p->m_leaf = false;

  // --- INITIAL PARTITION

  if (PARTITION_AREA_ONLY)
    partitionEqualArea(p);
  else 
    partitionScanlineMincut(p);

  resizePartition(p);

  // --- PARTITION IMPROVEMENT

  if (p->m_level < REPARTITION_LEVEL_DEPTH) {
    if (REPARTITION_FM)
      repartitionFM(p);
    else if (REPARTITION_HMETIS)
      repartitionHMetis(p);
  }
    
  resizePartition(p);
  
  // fix imbalances due to zero-area cells
  for(i=0; i<p->m_sub1->m_numMembers; i++)
    if (p->m_sub1->m_members[i]) 
      if (getCellArea(p->m_sub1->m_members[i]) > 0) {
        nonzeroCount++;
      }
  
  // is this leaf now done?
  if (nonzeroCount <= LARGEST_FINAL_SIZE)
      p->m_sub1->m_done = true;
  if (nonzeroCount == 0)
      degenerate = true;

  nonzeroCount = 0;
  for(i=0; i<p->m_sub2->m_numMembers; i++)
    if (p->m_sub2->m_members[i])
      if (getCellArea(p->m_sub2->m_members[i]) > 0) {
        nonzeroCount++;
      }

  // is this leaf now done?
  if (nonzeroCount <= LARGEST_FINAL_SIZE)
      p->m_sub2->m_done = true;
  if (nonzeroCount == 0)
      degenerate = true;

  // have we found a degenerate partitioning?
  if (degenerate) {
    printf("QPART-35 : WARNING: degenerate partition generated\n");
    partitionEqualArea(p);
    resizePartition(p);
    p->m_sub1->m_done = true;
    p->m_sub2->m_done = true;
  }
  
  // is this parent now finished?
  if (p->m_sub1->m_done && p->m_sub2->m_done) p->m_done = true;
  
  return p->m_done;
}


// --------------------------------------------------------------------
// repartitionHMetis()
//
/// \brief Repartitions the two subpartitions using the hMetis min-cut library.
///
/// The number of cut nets between the two partitions will be minimized.
//
// --------------------------------------------------------------------
void repartitionHMetis(Partition *parent) {
#if defined(NO_HMETIS)
  printf("QPAR_02 : \t\tERROR: hMetis not available.  Ignoring.\n");
#else

  int n,c,t, i;
  float area;
  int *edgeConnections = NULL;
  int *partitionAssignment = (int *)calloc(g_place_numCells, sizeof(int));
  int *vertexWeights = (int *)calloc(g_place_numCells, sizeof(int));
  int *edgeDegree = (int *)malloc(sizeof(int)*(g_place_numNets+1));
  int numConnections = 0;
  int numEdges = 0;
  float initial_cut;
  int targets = 0;
  ConcreteCell *cell = NULL;
  int options[9];
  int afterCuts = 0;

  assert(parent);
  assert(parent->m_sub1);
  assert(parent->m_sub2);

  printf("QPAR-02 : \t\trepartitioning with hMetis\n");

  // count edges
  edgeDegree[0] = 0;
  for(n=0; n<g_place_numNets; n++) if (g_place_concreteNets[n])
    if (g_place_concreteNets[n]->m_numTerms > 1) {
      numConnections += g_place_concreteNets[n]->m_numTerms;
      edgeDegree[++numEdges] = numConnections;
    }
  
  if (parent->m_vertical) {
    // vertical
    initial_cut = parent->m_sub2->m_bounds.x;
    
    // initialize all cells
    for(c=0; c<g_place_numCells; c++) if (g_place_concreteCells[c]) {
      if (g_place_concreteCells[c]->m_x < initial_cut)
        partitionAssignment[c] = 0;
      else
        partitionAssignment[c] = 1;
    }
  
    // initialize cells in partition 1
    for(t=0; t<parent->m_sub1->m_numMembers; t++) if (parent->m_sub1->m_members[t]) {
      cell = parent->m_sub1->m_members[t];
      vertexWeights[cell->m_id] = getCellArea(cell);
      // pay attention to cells that are close to the cut
      if (abs(cell->m_x-initial_cut) < parent->m_bounds.w*REPARTITION_TARGET_FRACTION) {
        targets++;
        partitionAssignment[cell->m_id] = -1;
      }
    }
    
    // initialize cells in partition 2
    for(t=0; t<parent->m_sub2->m_numMembers; t++) if (parent->m_sub2->m_members[t]) {
      cell = parent->m_sub2->m_members[t];
      vertexWeights[cell->m_id] = getCellArea(cell);
      // pay attention to cells that are close to the cut
      if (abs(cell->m_x-initial_cut) < parent->m_bounds.w*REPARTITION_TARGET_FRACTION) {
        targets++;
        partitionAssignment[cell->m_id] = -1;
      }		
    }
    
  } else {
    // horizontal
    initial_cut = parent->m_sub2->m_bounds.y;
    
    // initialize all cells
    for(c=0; c<g_place_numCells; c++) if (g_place_concreteCells[c]) {
      if (g_place_concreteCells[c]->m_y < initial_cut)
        partitionAssignment[c] = 0;
      else
        partitionAssignment[c] = 1;
    }
    
    // initialize cells in partition 1
    for(t=0; t<parent->m_sub1->m_numMembers; t++) if (parent->m_sub1->m_members[t]) {
      cell = parent->m_sub1->m_members[t];
      vertexWeights[cell->m_id] = getCellArea(cell);
      // pay attention to cells that are close to the cut
      if (abs(cell->m_y-initial_cut) < parent->m_bounds.h*REPARTITION_TARGET_FRACTION) {
        targets++;
        partitionAssignment[cell->m_id] = -1;
      }
    }
    
    // initialize cells in partition 2
    for(t=0; t<parent->m_sub2->m_numMembers; t++) if (parent->m_sub2->m_members[t]) {
      cell = parent->m_sub2->m_members[t];
      vertexWeights[cell->m_id] = getCellArea(cell);
      // pay attention to cells that are close to the cut
      if (abs(cell->m_y-initial_cut) < parent->m_bounds.h*REPARTITION_TARGET_FRACTION) {
        targets++;
        partitionAssignment[cell->m_id] = -1;
      }		
    }
  }

  options[0] = 1;  // any non-default values?
  options[1] = 3; // num bisections
  options[2] = 1;  // grouping scheme
  options[3] = 1;  // refinement scheme
  options[4] = 1;  // cycle refinement scheme
  options[5] = 0;  // reconstruction scheme
  options[6] = 0;  // fixed assignments?
  options[7] = 12261980; // random seed
  options[8] = 0;  // debugging level

  edgeConnections = (int *)malloc(sizeof(int)*numConnections);

  i = 0;
  for(n=0; n<g_place_numNets; n++) if (g_place_concreteNets[n]) {
    if (g_place_concreteNets[n]->m_numTerms > 1)
      for(t=0; t<g_place_concreteNets[n]->m_numTerms; t++)
        edgeConnections[i++] = g_place_concreteNets[n]->m_terms[t]->m_id;
  }

  HMETIS_PartRecursive(g_place_numCells, numEdges, vertexWeights,
		       edgeDegree, edgeConnections, NULL,
		       2, (int)(100*MAX_PARTITION_NONSYMMETRY),
		       options, partitionAssignment, &afterCuts);
	
  /*
  printf("HMET-20 : \t\t\tbalance before %d / %d ... ", parent->m_sub1->m_numMembers,
         parent->m_sub2->m_numMembers);
  */

  // reassign members to subpartitions
  parent->m_sub1->m_numMembers = 0;
  parent->m_sub1->m_area = 0;
  parent->m_sub2->m_numMembers = 0;
  parent->m_sub2->m_area = 0;
  parent->m_sub1->m_members = (ConcreteCell**)realloc(parent->m_sub1->m_members, 
       sizeof(ConcreteCell*)*parent->m_numMembers); 
  parent->m_sub2->m_members = (ConcreteCell**)realloc(parent->m_sub2->m_members, 
       sizeof(ConcreteCell*)*parent->m_numMembers); 
 
  for(t=0; t<parent->m_numMembers; t++) if (parent->m_members[t]) {
    cell = parent->m_members[t];
    area = getCellArea(cell);
    if (partitionAssignment[cell->m_id] == 0) {
      parent->m_sub1->m_members[parent->m_sub1->m_numMembers++] = cell;
      parent->m_sub1->m_area += area;
    }
    else {
      parent->m_sub2->m_members[parent->m_sub2->m_numMembers++] = cell;
      parent->m_sub2->m_area += area;
    }
  }
  /*
  printf("after %d / %d\n", parent->m_sub1->m_numMembers,
         parent->m_sub2->m_numMembers);
  */

  // cout << "HMET-21 : \t\t\tloc: " << initial_cut <<  " targetting: " << targets*100/parent->m_members.length() << "%" << endl;
  // cout << "HMET-22 : \t\t\tstarting cuts= " << beforeCuts << " final cuts= " << afterCuts << endl;

  free(edgeConnections);
  free(vertexWeights);
  free(edgeDegree);
  free(partitionAssignment);
#endif
}


// --------------------------------------------------------------------
// repartitionFM()
//
/// \brief Fiduccia-Matheyses mincut partitioning algorithm.
//
/// UNIMPLEMENTED (well, un-C-ified)
//
// --------------------------------------------------------------------
void repartitionFM(Partition *parent) {
#if 0
    assert(!parent->leaf && parent->m_sub1->leaf && parent->m_sub2->leaf);

    // count of each net's number of cells in each bipartition
    int count_1[m_design->nets.length()];
    memset(count_1, 0, sizeof(int)*m_design->nets.length());
    int count_2[m_design->nets.length()];
    memset(count_2, 0, sizeof(int)*m_design->nets.length());

    FM_cell target[m_design->cells.length()];
    memset(target, 0, sizeof(FM_cell)*m_design->cells.length());
    FM_cell *bin[FM_MAX_BIN+1];
    FM_cell *locked = 0;
    memset(bin, 0, sizeof(FM_cell *)*(FM_MAX_BIN+1));

    int max_gain = 0;
    int before_cuts = 0, current_cuts = 0;
    double initial_cut;
    int targets = 0;
    long cell_id;
    double halfArea = parent->m_area / 2.0;
    double areaFlexibility = parent->m_area * MAX_PARTITION_NONSYMMETRY;
    ConcreteNet *net;

    // INITIALIZATION
    //   select cells to partition

    if (parent->vertical) {
	// vertical

	initial_cut = parent->m_sub2->m_bounds.x;

	// initialize all cells
	for(h::list<ConcreteCell *>::iterator it = rootPartition->m_members.begin(); !it; it++) {
	    cell_id = (*it)->getID();
	    if ((*it)->temp_x < initial_cut)
		target[cell_id].loc = -1;
	    else
		target[cell_id].loc = -2;
	    target[cell_id].cell = *it;
	    target[cell_id].gain = 0;
	}

	// initialize cells in partition 1
	for(h::list<ConcreteCell *>::iterator it = parent->m_sub1->m_members.begin(); !it; it++) {
	    cell_id = (*it)->getID();
	    // pay attention to cells that are close to the cut
	    if (abs((*it)->temp_x-initial_cut) < parent->m_bounds.w*REPARTITION_TARGET_FRACTION) {
		targets++;
		target[cell_id].loc = 1;
	    }
	}

	// initialize cells in partition 2
	for(h::list<ConcreteCell *>::iterator it = parent->m_sub2->m_members.begin(); !it; it++) {
	    cell_id = (*it)->getID();
	    // pay attention to cells that are close to the cut
	    if (abs((*it)->temp_x-initial_cut) < parent->m_bounds.w*REPARTITION_TARGET_FRACTION) {
		targets++;
		target[cell_id].loc = 2;
	    }		
	}

   	// count the number of cells on each side of the partition for every net 
	for(h::hash_map<ConcreteNet *>::iterator n_it = m_design->nets.begin(); !n_it; n_it++) {
	    for(ConcretePinList::iterator p_it = (net = *n_it)->getPins().begin(); !p_it; p_it++)
		if (abs(target[(*p_it)->getCell()->getID()].loc) == 1)
		    count_1[net->getID()]++;
		else if (abs(target[(*p_it)->getCell()->getID()].loc) == 2)
		    count_2[net->getID()]++;
		else if ((*p_it)->getCell()->temp_x < initial_cut) 
		    count_1[net->getID()]++;
		else
		    count_2[net->getID()]++;
	    if (count_1[net->getID()] > 0 && count_2[net->getID()] > 0) before_cuts++;
	}
	
    } else {
	// horizontal

	initial_cut = parent->m_sub2->m_bounds.y;

	// initialize all cells
	for(h::list<ConcreteCell *>::iterator it = rootPartition->m_members.begin(); !it; it++) {
	    cell_id = (*it)->getID();
	    if ((*it)->temp_y < initial_cut)
		target[cell_id].loc = -1;
	    else
		target[cell_id].loc = -2;
	    target[cell_id].cell = *it;
	    target[cell_id].gain = 0;
	}

	// initialize cells in partition 1
	for(h::list<ConcreteCell *>::iterator it = parent->m_sub1->m_members.begin(); !it; it++) {
	    cell_id = (*it)->getID();
	    // pay attention to cells that are close to the cut
	    if (abs((*it)->temp_y-initial_cut) < parent->m_bounds.h*REPARTITION_TARGET_FRACTION) {
		targets++;
		target[cell_id].loc = 1;
	    }
	}

	// initialize cells in partition 2
	for(h::list<ConcreteCell *>::iterator it = parent->m_sub2->m_members.begin(); !it; it++) {
	    cell_id = (*it)->getID();
	    // pay attention to cells that are close to the cut
	    if (abs((*it)->temp_y-initial_cut) < parent->m_bounds.h*REPARTITION_TARGET_FRACTION) {
		targets++;
		target[cell_id].loc = 2;
	    }		
	}

   	// count the number of cells on each side of the partition for every net 
	for(h::hash_map<ConcreteNet *>::iterator n_it = m_design->nets.begin(); !n_it; n_it++) {
	    for(ConcretePinList::iterator p_it = (net = *n_it)->getPins().begin(); !p_it; p_it++)
		if (abs(target[(*p_it)->getCell()->getID()].loc) == 1)
		    count_1[net->getID()]++;
		else if (abs(target[(*p_it)->getCell()->getID()].loc) == 2)
		    count_2[net->getID()]++;
		else if ((*p_it)->getCell()->temp_y < initial_cut) 
		    count_1[net->getID()]++;
		else
		    count_2[net->getID()]++;
	    if (count_1[net->getID()] > 0 && count_2[net->getID()] > 0) before_cuts++;
	}
    }

    // INITIAL GAIN CALCULATION
    for(long id=0; id < m_design->cells.length(); id++)
	if (target[id].loc > 0) {
	    assert(target[id].cell != 0);
	    assert(target[id].gain == 0);

	    // examine counts for the net on each pin
	    for(ConcretePinMap::iterator p_it = target[id].cell->getPins().begin(); !p_it; p_it++)
		if ((*p_it)->isAttached()) {
		    int n_id = (*p_it)->getNet()->getID();
		    if (target[id].loc == 1 && count_1[n_id] == 1) target[id].gain++;
		    if (target[id].loc == 1 && count_2[n_id] == 0) target[id].gain--;
		    if (target[id].loc == 2 && count_1[n_id] == 0) target[id].gain--;
		    if (target[id].loc == 2 && count_2[n_id] == 1) target[id].gain++;
		}

	    assert(target[id].cell->getPins().length() >= abs(target[id].gain));

	    // add it to a bin
	    int bin_num = min(max(0, target[id].gain),FM_MAX_BIN);
	    max_gain = max(max_gain, bin_num);

	    assert(bin_num >= 0 && bin_num <= FM_MAX_BIN);
	    target[id].next = bin[bin_num];
	    target[id].prev = 0;
	    if (bin[bin_num] != 0)
		bin[bin_num]->prev = &target[id];
	    bin[bin_num] = &target[id];
	}

    // OUTER F-M LOOP
    current_cuts = before_cuts;
    int num_moves = 1;
    int pass = 0;
    while(num_moves > 0 && pass < FM_MAX_PASSES) {
	pass++;
	num_moves = 0;	

	// check_list(bin, locked, targets); // DEBUG

	// move all locked cells back
	int moved_back = 0;
	while(locked != 0) {
	    FM_cell *current = locked;
	    current->locked = false;

	    int bin_num = min(max(0, current->gain),FM_MAX_BIN);	   
	    max_gain = max(max_gain, bin_num);

	    locked = current->next;
	    if (locked != 0)
		locked->prev = 0;

	    if (bin[bin_num] != 0)
		bin[bin_num]->prev = current;
	    current->next = bin[bin_num];
	    bin[bin_num] = current;

	    moved_back++;
	}
	// cout << "\tmoved back: " << moved_back << endl;
	// check_list(bin, locked, targets); // DEBUG	
	
	max_gain = FM_MAX_BIN;
	while(bin[max_gain] == 0 && max_gain > 0) max_gain--;

	// INNER F-M LOOP (single pass)
	while(1) {

	    int bin_num = FM_MAX_BIN;
	    FM_cell *current = bin[bin_num];

	    // look for next cell to move
	    while (bin_num > 0 && (current == 0 || 
		(current->loc==1 && current->cell->getArea()+parent->m_sub2->m_area > halfArea+areaFlexibility) ||
		(current->loc==2 && current->cell->getArea()+parent->m_sub1->m_area > halfArea+areaFlexibility))) {

		if (current == 0) current = bin[--bin_num]; else current = current->next;		
	    }
	    if (bin_num == 0)
		break;

	    num_moves++;
	    current->locked = true;
	    // cout << "moving cell " << current->cell->getID() << " gain=" << current->gain << " pins= " << current->cell->getPins().length() << " from " << current->loc;

	    // change partition marking and areas
	    if (current->loc == 1) {
		current->loc = 2;
		parent->m_sub1->m_area -= current->cell->getArea();
		parent->m_sub2->m_area += current->cell->getArea();

		// update partition counts on all nets attached to this cell
		for(ConcretePinMap::iterator p_it = current->cell->getPins().begin(); 
		    !p_it; p_it++) {
		    
		    if (!(*p_it)->isAttached()) // ignore unattached pins
			continue;
		    net = (*p_it)->getNet();
		    
		    count_1[net->getID()]--;
		    count_2[net->getID()]++;

		    // cout << "\tnet " << net->getID() << " was " << count_1[net->getID()]+1 << "/" << count_2[net->getID()]-1 << " now " << count_1[net->getID()] << "/" << count_2[net->getID()] << endl;

		    // if net becomes critical, update gains on attached cells and resort bins
		    if (count_1[net->getID()] == 0) { current_cuts--; FM_updateGains(net, 2, -1, target, bin, count_1, count_2); }
		    if (count_2[net->getID()] == 1) { current_cuts++; FM_updateGains(net, 1, -1, target, bin, count_1, count_2); }
		    
		    // check_list(bin, locked, targets); // DEBUG
		}

	    } else {
		current->loc = 1;
		parent->m_sub2->m_area -= current->cell->getArea();
		parent->m_sub1->m_area += current->cell->getArea();

		// update gains on all nets attached to this cell
		for(ConcretePinMap::iterator p_it = current->cell->getPins().begin(); 
		    !p_it; p_it++) {
		    
		    if (!(*p_it)->isAttached()) // ignore unattached pins
			continue;
		    net = (*p_it)->getNet();
		    count_2[net->getID()]--;
		    count_1[net->getID()]++;

		    // cout << "\tnet " << net->getID() << " was " << count_1[net->getID()]-1 << "/" << count_2[net->getID()]+1 << " now " << count_1[net->getID()] << "/" << count_2[net->getID()] << endl;

		    if (count_2[net->getID()] == 0) { current_cuts--; FM_updateGains(net, 2, -1, target, bin, count_1, count_2); }
		    if (count_1[net->getID()] == 1) { current_cuts++; FM_updateGains(net, 1, -1, target, bin, count_1, count_2); }
		
		    // check_list(bin, locked, targets); // DEBUG
		}
	    }

	    //cout << " cuts=" << current_cuts << endl;

	    // move current to locked

/*
	    cout << "b=" << bin[bin_num] << " ";
	    cout << current->prev << "-> ";
	    if (current->prev == 0)
		cout << "X";
	    else cout << current->prev->next;
	    cout  << "=" << current << "=";
	    if (current->next == 0)
		cout << "X";
	    else
		cout << current->next->prev;
	    cout << " ->" << current->next << endl;
*/

	    if (bin[bin_num] == current)
		bin[bin_num] = current->next;
	    if (current->prev != 0)
		current->prev->next = current->next;
	    if (current->next != 0)
		current->next->prev = current->prev;

/*
	    cout << "b=" << bin[bin_num] << " ";
	    cout << current->prev << "-> ";
	    if (current->prev == 0)
		cout << "X";
	    else cout << current->prev->next;
	    cout  << "=" << current << "=";
	    if (current->next == 0)
		cout << "X";
	    else
		cout << current->next->prev;
	    cout << " ->" << current->next << endl;
*/

	    current->prev = 0;
	    current->next = locked;
	    if (locked != 0)
		locked->prev = current;
	    locked = current;
	    
	    // check_list(bin, locked, targets); // DEBUG	

	    // update max_gain
	    max_gain = FM_MAX_BIN;
	    while(bin[max_gain] == 0 && max_gain > 0) max_gain--;
	}

	// cout << "\tcurrent cuts= " << current_cuts << " moves= " << num_moves << endl;
    }

    // reassign members to subpartitions
    cout << "FIDM-20 : \tbalance before " << parent->m_sub1->m_members.length() << "/"
       << parent->m_sub2->m_members.length() << " ";
    parent->m_sub1->m_members.clear();
    parent->m_sub1->m_area = 0;
    parent->m_sub2->m_members.clear();
    parent->m_sub2->m_area = 0;
    for(h::list<ConcreteCell *>::iterator it = parent->m_members.begin(); !it; it++) {
	if (target[(*it)->getID()].loc == 1 || target[(*it)->getID()].loc == -1) {
	    parent->m_sub1->m_members.push_back(*it);
	    parent->m_sub1->m_area += (*it)->getArea();
	}
	else {
	    parent->m_sub2->m_members.push_back(*it);
	    parent->m_sub2->m_area += (*it)->getArea();
	}
    }
    cout << " after " << parent->m_sub1->m_members.length() << "/"
       << parent->m_sub2->m_members.length() << endl;


    cout << "FIDM-21 : \tloc: " << initial_cut <<  " targetting: " << targets*100/parent->m_members.length() << "%" << endl;
    cout << "FIDM-22 : \tstarting cuts= " << before_cuts << " final cuts= " << current_cuts << endl;
#endif
}

// ----- FM_updateGains()
//   moves a cell between bins
#if 0
void FM_updateGains(ConcreteNet *net, int partition, int inc, 
                    FM_cell target [], FM_cell *bin [], 
                    int count_1 [], int count_2 []) {

    for(ConcretePinList::iterator it = net->getPins().begin(); !it; it++) {
	FM_cell *current = &(target[(*it)->getCell()->getID()]);
	assert(current->cell != 0);
	
	int old_gain = current->gain;
	current->gain = 0;

	// examine counts for the net on each pin
	for(ConcretePinMap::iterator p_it = current->cell->getPins().begin(); !p_it; p_it++)
	    if ((*p_it)->isAttached()) {
		int n_id = (*p_it)->getNet()->getID();
		if (current->loc == 1 && count_1[n_id] == 1) current->gain++;
		if (current->loc == 1 && count_2[n_id] == 0) current->gain--;
		if (current->loc == 2 && count_1[n_id] == 0) current->gain--;
		if (current->loc == 2 && count_2[n_id] == 1) current->gain++;
	    }

	if (!current->locked) {
	    // remove cell from existing bin
	    int bin_num =  min(max(0, old_gain),FM_MAX_BIN);
	    if (bin[bin_num] == current)
		bin[bin_num] = current->next;
	    if (current->prev != 0)
		current->prev->next = current->next;
	    if (current->next != 0)
		current->next->prev = current->prev;
	    // add cell to correct bin
	    bin_num =  min(max(0, current->gain),FM_MAX_BIN);
	    current->prev = 0;
	    current->next = bin[bin_num];
	    if (bin[bin_num] != 0)
		bin[bin_num]->prev = current;
	    bin[bin_num] = current;
	}
    }
    
}
#endif


// --------------------------------------------------------------------
// partitionEqualArea()
//
/// \brief Splits a partition into two halves of equal area.
//
// --------------------------------------------------------------------
void partitionEqualArea(Partition *parent) {
  float halfArea, area;
  int i=0;
  
  // which way to sort?
  if (parent->m_vertical)
    // sort by X position
    qsort(parent->m_members, parent->m_numMembers, sizeof(ConcreteCell*), cellSortByX);
  else
    // sort by Y position
    qsort(parent->m_members, parent->m_numMembers, sizeof(ConcreteCell*), cellSortByY);

  // split the list
  halfArea = parent->m_area*0.5;
  parent->m_sub1->m_area = 0.0;
  parent->m_sub1->m_numMembers = 0;
  parent->m_sub1->m_members = (ConcreteCell**)realloc(parent->m_sub1->m_members, 
                                  sizeof(ConcreteCell*)*parent->m_numMembers);
  parent->m_sub2->m_area = 0.0;
  parent->m_sub2->m_numMembers = 0;
  parent->m_sub2->m_members = (ConcreteCell**)realloc(parent->m_sub2->m_members, 
                                  sizeof(ConcreteCell*)*parent->m_numMembers);

  for(; parent->m_sub1->m_area < halfArea; i++) 
    if (parent->m_members[i]) {
      area = getCellArea(parent->m_members[i]);
      parent->m_sub1->m_members[parent->m_sub1->m_numMembers++] = parent->m_members[i];
      parent->m_sub1->m_area += area;
  }
  for(; i<parent->m_numMembers; i++) 
    if (parent->m_members[i]) {
      area = getCellArea(parent->m_members[i]);
      parent->m_sub2->m_members[parent->m_sub2->m_numMembers++] = parent->m_members[i];
      parent->m_sub2->m_area += area;
    }
  
}


// --------------------------------------------------------------------
// partitionScanlineMincut()
//
/// \brief Scans the cells within a partition from left to right and chooses the min-cut.
//
// --------------------------------------------------------------------
void partitionScanlineMincut(Partition *parent) {
#if 0
  int current_cuts = 0;
  int minimum_cuts = INT_MAX;
  ConcreteCell *minimum_location = NULL;
  double currentArea = 0, halfArea = parent->m_area * 0.5;
  double areaFlexibility = parent->m_area * MAX_PARTITION_NONSYMMETRY;
  double newLine, oldLine = -DBL_MAX;

  for(ConcreteNetList::iterator n_it = m_design->nets.begin(); !n_it; n_it++)
    (*n_it)->m_mark = 0;
  for(h::list<ConcreteCell *>::iterator i = parent->m_members.begin();
      !i.isDone(); i++) {
    assert(*i);
    for(ConcretePinMap::iterator j = (*i)->getPins().begin();
	!j.isDone(); j++) {
      assert(*j);
      if((*j)->isAttached()) {
	(*j)->getNet()->m_mark = 1;
      }
    }
  }

  if (parent->vertical) {
    parent->m_members.sort(sortByX);
    int all1 = 0, all2 = 0;
    h::list<ConcreteCell *>::iterator local = parent->m_members.begin();
    for(; !local.isDone(); local++) {
      currentArea += (*local)->getArea();
      if (currentArea < halfArea-areaFlexibility)
	continue;
      if (currentArea > halfArea+areaFlexibility)
	break;
      newLine = (*local)->temp_x;
      while(all1 < g_place_numNets && allNetsL2[all1]->getBoundingBox().left() <= newLine) {
	if(allNetsL2[all1]->m_mark) {
	  current_cuts++;
	}
	all1++;
      }
      while(all2 < g_place_numNets && allNetsR2[all2]->getBoundingBox().right() <= newLine) {
	if(allNetsR2[all2]->m_mark) {
	  current_cuts--;
	}
	all2++;
      }
      if (current_cuts < minimum_cuts) {
	minimum_cuts = current_cuts;
	minimum_location = *local;
      }
      oldLine = newLine;
    }
  }
  else {
    parent->m_members.sort(sortByY);
    int all1 = 0, all2 = 0;
    h::list<ConcreteCell *>::iterator local = parent->m_members.begin();
    for(; !local.isDone(); local++) {
      currentArea += (*local)->getArea();
      if (currentArea < halfArea-areaFlexibility)
	continue;
      if (currentArea > halfArea+areaFlexibility)
	break;
      newLine = (*local)->temp_y;
      while(all1 < g_place_numNets && allNetsB2[all1]->getBoundingBox().top() <= newLine) {
	if(allNetsB2[all1]->m_mark) {
	  current_cuts++;
	}
	all1++;
      }
      while(all2 < g_place_numNets && allNetsT2[all2]->getBoundingBox().bottom() <= newLine) {
	if(allNetsT2[all2]->m_mark) {
	  current_cuts--;
	}
	all2++;
      }
      if (current_cuts < minimum_cuts) {
	minimum_cuts = current_cuts;
	minimum_location = *local;
      }
      oldLine = newLine;
    }
  }
  if (minimum_location == NULL) {
    return partitionEqualArea(parent);
  }
  h::list<ConcreteCell *>::iterator it = parent->m_members.begin();
  parent->m_sub1->m_members.clear();
  parent->m_sub1->m_area = 0;
  for(; *it != minimum_location; it++) {
    parent->m_sub1->m_members.push_front(*it);
    parent->m_sub1->m_area += (*it)->getArea();
  }
  parent->m_sub2->m_members.clear();
  parent->m_sub2->m_area = 0;
  for(; !it; it++) {
    parent->m_sub2->m_members.push_front(*it);
    parent->m_sub2->m_area += (*it)->getArea();
  }
#endif
}


// --------------------------------------------------------------------
// reallocPartition()
//
/// \brief Reallocates a partition and all of its children.
//
// --------------------------------------------------------------------
void reallocPartition(Partition *p) {

  if (p->m_leaf) {
    return;
  }

  // --- INITIAL PARTITION

  if (PARTITION_AREA_ONLY)
    partitionEqualArea(p);
  else 
    partitionScanlineMincut(p);

  resizePartition(p);

  // --- PARTITION IMPROVEMENT
  if (p->m_level < REPARTITION_LEVEL_DEPTH) {
    if (REPARTITION_HMETIS)
      repartitionHMetis(p);
    
    resizePartition(p);
  }

  reallocPartition(p->m_sub1);
  reallocPartition(p->m_sub2);
}


// --------------------------------------------------------------------
// resizePartition()
//
/// \brief Recomputes the bounding boxes of the child partitions based on their relative areas.
//
// --------------------------------------------------------------------
void resizePartition(Partition *p) {
  // compute the new bounding box
  p->m_sub1->m_bounds.x = p->m_bounds.x;
  p->m_sub1->m_bounds.y = p->m_bounds.y;
  if (p->m_vertical) {
    p->m_sub1->m_bounds.w = p->m_bounds.w*(p->m_sub1->m_area/p->m_area);
    p->m_sub1->m_bounds.h = p->m_bounds.h;
    p->m_sub2->m_bounds.x = p->m_bounds.x + p->m_sub1->m_bounds.w;
    p->m_sub2->m_bounds.w = p->m_bounds.w*(p->m_sub2->m_area/p->m_area);
    p->m_sub2->m_bounds.y = p->m_bounds.y;
    p->m_sub2->m_bounds.h = p->m_bounds.h;
  } else {
    p->m_sub1->m_bounds.h = p->m_bounds.h*(p->m_sub1->m_area/p->m_area);
    p->m_sub1->m_bounds.w = p->m_bounds.w;
    p->m_sub2->m_bounds.y = p->m_bounds.y + p->m_sub1->m_bounds.h;
    p->m_sub2->m_bounds.h = p->m_bounds.h*(p->m_sub2->m_area/p->m_area);
    p->m_sub2->m_bounds.x = p->m_bounds.x;
    p->m_sub2->m_bounds.w = p->m_bounds.w;
  }
}


// --------------------------------------------------------------------
// incrementalSubpartition()
//
/// \brief Adds new cells to an existing partition.  Partition sizes/locations are unchanged.
///
/// The function recurses, adding new cells to appropriate subpartitions.
//
// --------------------------------------------------------------------
void incrementalSubpartition(Partition *p, ConcreteCell *newCells [], const int numNewCells) {
  int c;
  ConcreteCell **newCells1 = (ConcreteCell **)malloc(sizeof(ConcreteCell*)*numNewCells), 
    **newCells2 = (ConcreteCell **)malloc(sizeof(ConcreteCell*)*numNewCells);
  int numNewCells1 = 0, numNewCells2 = 0;
  float cut_loc;

  assert(p);

  // add new cells to partition list
  p->m_members = (ConcreteCell**)realloc(p->m_members, 
       sizeof(ConcreteCell*)*(p->m_numMembers+numNewCells));
  memcpy(&(p->m_members[p->m_numMembers]), newCells, sizeof(ConcreteCell*)*numNewCells);
  p->m_numMembers += numNewCells;

  // if is a leaf partition, finished
  if (p->m_leaf) return;

  // split new cells into sub-partitions based on location
  if (p->m_vertical) {
    cut_loc = p->m_sub2->m_bounds.x;
    for(c=0; c<numNewCells; c++)
      if (newCells[c]->m_x < cut_loc)
        newCells1[numNewCells1++] = newCells[c];
      else
        newCells2[numNewCells2++] = newCells[c];
  } else {
    cut_loc = p->m_sub2->m_bounds.y;
    for(c=0; c<numNewCells; c++)
      if (newCells[c]->m_y < cut_loc)
        newCells1[numNewCells1++] = newCells[c];
      else
        newCells2[numNewCells2++] = newCells[c];    
  }

  if (numNewCells1 > 0) incrementalSubpartition(p->m_sub1, newCells1, numNewCells1);
  if (numNewCells2 > 0) incrementalSubpartition(p->m_sub2, newCells2, numNewCells2);

  free(newCells1);
  free(newCells2);
}


// --------------------------------------------------------------------
// incrementalPartition()
//
/// \brief Adds new cells to an existing partition.  Partition sizes/locations are unchanged.
///
/// The function recurses, adding new cells to appropriate subpartitions.
//
// --------------------------------------------------------------------
void incrementalPartition() {
  int c = 0, c2 = 0;
  int numNewCells = 0;
  ConcreteCell **allCells = (ConcreteCell **)malloc(sizeof(ConcreteCell*)*g_place_numCells),
    **newCells = (ConcreteCell **)malloc(sizeof(ConcreteCell*)*g_place_numCells);

  assert(g_place_rootPartition);

  // update cell list of root partition
  memcpy(allCells, g_place_concreteCells, sizeof(ConcreteCell*)*g_place_numCells);
  qsort(allCells, g_place_numCells, sizeof(ConcreteCell*), cellSortByID);
  qsort(g_place_rootPartition->m_members, g_place_rootPartition->m_numMembers,
        sizeof(ConcreteCell*), cellSortByID);

  // scan sorted lists and collect cells not in partitions
  while(!allCells[c++]);
  while(!g_place_rootPartition->m_members[c2++]);

  for(; c<g_place_numCells; c++, c2++) {
    while(c2 < g_place_rootPartition->m_numMembers &&
          allCells[c]->m_id > g_place_rootPartition->m_members[c2]->m_id) c2++;
    while(c < g_place_numCells && 
          (c2 >= g_place_rootPartition->m_numMembers ||
           allCells[c]->m_id < g_place_rootPartition->m_members[c2]->m_id)) {
      // a new cell!
      newCells[numNewCells++] = allCells[c];
      c++;
    }
  }
  
  printf("QPRT-50 : \tincremental partitioning with %d new cells\n", numNewCells);
  if (numNewCells>0) incrementalSubpartition(g_place_rootPartition, newCells, numNewCells);

  free(allCells);
  free(newCells);
}
