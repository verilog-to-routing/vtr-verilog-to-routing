#include "partition.h"
#include "partition_region.h"
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

void print_partition(FILE* fp, Partition part) {
    std::string name = part.get_name();
    fprintf(fp, "partition_name: %s\n", name.c_str());

    PartitionRegion pr = part.get_part_region();

    print_partition_region(fp, pr);
}
