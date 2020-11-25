#include "constraints_load.h"

void print_region(FILE* fp, Region region);
void print_partition(FILE* fp, Partition part);
void print_partition_region(FILE* fp, PartitionRegion pr);
void print_constraints(FILE* fp, VprConstraints constraints, int num_parts);

void print_region(FILE* fp, Region region) {
    vtr::Rect<int> rect = region.get_region_rect();
    int xmin = rect.xmin();
    int xmax = rect.xmax();
    int ymin = rect.ymin();
    int ymax = rect.ymax();
    int subtile = region.get_sub_tile();

    fprintf(fp, "Region: \n");
    fprintf(fp, "\txmin: %d\n", xmin);
    fprintf(fp, "\txmax: %d\n", xmax);
    fprintf(fp, "\tymin: %d\n", ymin);
    fprintf(fp, "\tymax: %d\n", ymax);
    fprintf(fp, "\tsubtile: %d\n\n", subtile);
}

void print_partition(FILE* fp, Partition part) {
    std::string name = part.get_name();
    fprintf(fp, "\tpartition_name: %s\n", name.c_str());

    PartitionRegion pr = part.get_part_region();

    print_partition_region(fp, pr);
}

void print_partition_region(FILE* fp, PartitionRegion pr) {
    std::vector<Region> part_region = pr.get_partition_region();

    int pr_size = part_region.size();

    fprintf(fp, "\tpart_region size is: %d\n", pr_size);

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

        print_partition(fp, temp_part);

        atoms = constraints.get_part_atoms(part_id);

        int atoms_size = atoms.size();

        fprintf(fp, "\tAtom vector size is %d\n", atoms_size);

        for (unsigned int j = 0; j < atoms.size(); j++) {
            AtomBlockId atom_id = atoms[j];
            fprintf(fp, "\tAtom %d is in the partition\n", atom_id);
        }
    }
}

void echo_region(char* filename, Region region) {
    FILE* fp;
    fp = vtr::fopen(filename, "w");

    fprintf(fp, "--------------------------------------------------------------\n");
    fprintf(fp, "Region\n");
    fprintf(fp, "--------------------------------------------------------------\n");
    fprintf(fp, "\n");
    print_region(fp, region);

    fclose(fp);
}

void echo_partition(char* filename, Partition part) {
    FILE* fp;
    fp = vtr::fopen(filename, "w");

    fprintf(fp, "--------------------------------------------------------------\n");
    fprintf(fp, "Partition\n");
    fprintf(fp, "--------------------------------------------------------------\n");
    fprintf(fp, "\n");
    print_partition(fp, part);

    fclose(fp);
}

void echo_constraints(char* filename, VprConstraints constraints, int num_parts) {
    FILE* fp;
    fp = vtr::fopen(filename, "w");

    fprintf(fp, "--------------------------------------------------------------\n");
    fprintf(fp, "Constraints\n");
    fprintf(fp, "--------------------------------------------------------------\n");
    fprintf(fp, "\n");
    print_constraints(fp, constraints, num_parts);

    fclose(fp);
}
