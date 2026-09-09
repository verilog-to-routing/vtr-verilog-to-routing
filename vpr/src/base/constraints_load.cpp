#include "constraints_load.h"

void echo_constraints(char* filename, const UserPlaceConstraints& constraints, const UserRelativeMacros& relative_macros) {
    FILE* fp;
    fp = vtr::fopen(filename, "w");

    fprintf(fp, "--------------------------------------------------------------\n");
    fprintf(fp, "Constraints\n");
    fprintf(fp, "--------------------------------------------------------------\n");
    fprintf(fp, "\n");
    print_placement_constraints(fp, constraints);
    print_relative_macros(fp, relative_macros);

    fclose(fp);
}
