#ifndef PARTITION_H
#define PARTITION_H

#include "vtr_strong_id.h"
#include "region.h"

/**
 * This class stores the data for each constraint partition -
 * partition name, the atoms that are in the partition,
 * and the regions that the partition can be placed in.
 */

//Type tag for PartitionId
struct partition_id_tag;

//A unique identifier for a partition
typedef vtr::StrongId<partition_id_tag> PartitionId;

class Partition {
  public: //accessors
  private:
    PartitionId id;
    std::string name;
    std::vector<AtomBlockId> atomBlocks; //atoms that belong to this partition
    std::vector<Region> regions;         //regions that this partition can be placed in
};
#endif /* PARTITION_H */
