#include "constraints_load.h"

static void print_region(FILE* fp, Region region);
static void print_partition(FILE* fp, Partition part);
static void print_partition_region(FILE* fp, PartitionRegion pr);
static void print_constraints(FILE* fp, VprConstraints constraints, int num_parts);

void print_region(FILE* fp, Region region) {
    vtr::Rect<int> rect = region.get_region_rect();
    int xmin = rect.xmin();
    int xmax = rect.xmax();
    int ymin = rect.ymin();
    int ymax = rect.ymax();
    int subtile = region.get_sub_tile();

    fprintf(fp, "\tRegion: \n");
    fprintf(fp, "\txmin: %d\n", xmin);
    fprintf(fp, "\txmax: %d\n", xmax);
    fprintf(fp, "\tymin: %d\n", ymin);
    fprintf(fp, "\tymax: %d\n", ymax);
    fprintf(fp, "\tsubtile: %d\n\n", subtile);
}

void print_partition(FILE* fp, Partition part) {
    std::string name = part.get_name();
    fprintf(fp, "partition_name: %s\n", name.c_str());

    PartitionRegion pr = part.get_part_region();

    print_partition_region(fp, pr);
}

void print_partition_region(FILE* fp, PartitionRegion pr) {
    std::vector<Region> part_region = pr.get_partition_region();

    int pr_size = part_region.size();

    fprintf(fp, "\tNumber of regions in partition is: %d\n", pr_size);

    for (unsigned int i = 0; i < part_region.size(); i++) {
        print_region(fp, part_region[i]);
    }
}

void print_constraints(FILE* fp, VprConstraints constraints, int num_parts) {
    Partition temp_part;
    std::vector<AtomBlockId> atoms;

    for (int i = 0; i < num_parts; i++) {
        PartitionId part_id(i);

        temp_part = constraints.get_partition(part_id);

        fprintf(fp, "\npartition_id: %zu\n", size_t(part_id));
        print_partition(fp, temp_part);

        atoms = constraints.get_part_atoms(part_id);

        int atoms_size = atoms.size();

        fprintf(fp, "\tAtom vector size is %d\n", atoms_size);
        fprintf(fp, "\tIds of atoms in partition: \n");
        for (unsigned int j = 0; j < atoms.size(); j++) {
            AtomBlockId atom_id = atoms[j];
            fprintf(fp, "\t#%zu\n", size_t(atom_id));
        }
    }
}

void echo_constraints(char* filename, VprConstraints constraints) {
    FILE* fp;
    fp = vtr::fopen(filename, "w");

    int num_of_parts = constraints.get_num_partitions();

    fprintf(fp, "--------------------------------------------------------------\n");
    fprintf(fp, "Constraints\n");
    fprintf(fp, "--------------------------------------------------------------\n");
    fprintf(fp, "\n");
    fprintf(fp, "\n Number of partitions is %d \n", num_of_parts);
    print_constraints(fp, constraints, num_of_parts);

    fclose(fp);
}
