#include "partition.h"
#include "partition_region.h"
#include <algorithm>
#include <utility>

const std::string& Partition::get_name() const {
    return name;
}

void Partition::set_name(std::string _part_name) {
    name = std::move(_part_name);
}

const PartitionRegion& Partition::get_part_region() const {
    return part_region;
}

PartitionRegion& Partition::get_mutable_part_region() {
    return part_region;
}

void Partition::set_part_region(PartitionRegion pr) {
    part_region = std::move(pr);
}

void print_partition(FILE* fp, const Partition& part) {
    const std::string& name = part.get_name();
    fprintf(fp, "partition_name: %s\n", name.c_str());

    const PartitionRegion& pr = part.get_part_region();

    print_partition_region(fp, pr);
}
