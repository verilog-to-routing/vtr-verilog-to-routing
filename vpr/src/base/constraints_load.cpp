#include "constraints_load.h"

void echo_constraints(char* filename, VprConstraints constraints) {
    FILE* fp;
    fp = vtr::fopen(filename, "w");

    fprintf(fp, "--------------------------------------------------------------\n");
    fprintf(fp, "Constraints\n");
    fprintf(fp, "--------------------------------------------------------------\n");
    fprintf(fp, "\n");
    print_constraints(fp, constraints);

    fclose(fp);
}
