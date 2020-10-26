#include "partition.h"
#include <algorithm>
#include <vector>

const std::string Partition::get_name() {
    return name;
}

void Partition::set_name(std::string _part_name) {
    name = _part_name;
}

const PartitionRegion Partition::get_part_regions() {
    return part_regions;
}

void Partition::set_part_regions(PartitionRegion pr) {
    part_regions = pr;
}
