#include "partition.h"
#include <algorithm>
#include <vector>

const std::string Partition::get_name() {
    return name;
}

void Partition::set_name(std::string _part_name) {
    name = _part_name;
}

const PartitionRegion Partition::get_part_region() {
    return part_region;
}

void Partition::set_part_region(PartitionRegion pr) {
    part_region = pr;
}
