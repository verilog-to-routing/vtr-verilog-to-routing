/*===================================================================*/
//  
//     place_gordian.h
//
//        Aaron P. Hurst, 2003-2007
//              ahurst@eecs.berkeley.edu
//
/*===================================================================*/

#if !defined(PLACE_GORDIAN_H_)
#define ABC__phys__place__place_gordian_h


#include "place_base.h"
#include "place_qpsolver.h"

ABC_NAMESPACE_HEADER_START


// Parameters for analytic placement
#define CLIQUE_PENALTY 1.0
#define IGNORE_NETSIZE 20

// Parameters for partitioning
#define LARGEST_FINAL_SIZE 20
#define PARTITION_AREA_ONLY true
#define REALLOCATE_PARTITIONS false
#define FINAL_REALLOCATE_PARTITIONS false
#define IGNORE_COG false
#define MAX_PARTITION_NONSYMMETRY 0.30

// Parameters for re-partitioning
#define REPARTITION_LEVEL_DEPTH 4
#define REPARTITION_TARGET_FRACTION 0.15
#define REPARTITION_FM false
#define REPARTITION_HMETIS true

// Parameters for F-M re-partitioning
#define FM_MAX_BIN 10
#define FM_MAX_PASSES 10

extern int g_place_numPartitions;

extern qps_problem_t *g_place_qpProb;

typedef struct Partition {

  int               m_numMembers;
  ConcreteCell    **m_members;
  Rect              m_bounds;
  bool              m_done, 
                    m_leaf, 
                    m_vertical;
  float             m_area;
  int               m_level;
  struct Partition *m_sub1, *m_sub2;
} Partition;

extern Partition *g_place_rootPartition;

void initPartitioning();

void incrementalPartition();

bool refinePartitions();
void reallocPartitions();
bool refinePartition(Partition *p);
void resizePartition(Partition *p);
void reallocPartition(Partition *p);

void repartitionHMetis(Partition *parent);
void repartitionFM(Partition *parent);

void partitionScanlineMincut(Partition *parent);
void partitionEqualArea(Partition *parent);

void sanitizePlacement();

void constructQuadraticProblem();
void solveQuadraticProblem(bool useCOG);



ABC_NAMESPACE_HEADER_END

#endif
