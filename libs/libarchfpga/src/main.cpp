/*
 * Test libarchfpga, try reading an architecture and print the results to a file
 *
 * Date: February 19, 2009
 * Author: Jason Luu
 */

#include <stdio.h>
#include <stdlib.h>

#include "vtr_error.h"
#include "vtr_memory.h"

#include "read_xml_arch_file.h"
#include "echo_arch.h"

void print_help();

int main(int argc, char** argv) {
    try {
        t_arch* arch = (t_arch*)vtr::calloc(1, sizeof(t_arch));
        t_type_descriptor* types;
        int numTypes;

        if (argc - 1 != 3) {
            printf("Error: Unexpected # of arguments.  Expected 3 found %d arguments\n",
                   argc);
            print_help();
            return 1;
        }

        printf("------------------------------------------------------------------------------\n");
        printf("- Read architecture file and print library data structures into an output file\n");
        printf("------------------------------------------------------------------------------\n\n");

        printf(
            "Inputs: \n"
            "architecture %s \n"
            "timing_driven %d \n"
            "output file %s\n",
            argv[1], atoi(argv[2]), argv[3]);
        printf("Reading in architecture\n");

        /* function declarations */
        XmlReadArch(argv[1], atoi(argv[2]), arch, &types, &numTypes);

        printf("Printing Results\n");

        EchoArch(argv[3], types, numTypes, arch);
        free(arch);
    } catch (vtr::VtrError& vtr_error) {
        printf("Failed to process architecture %s: %s\n", argv[1], vtr_error.what());
        return 1;
    }

    printf("Done\n");

    return 0;
}

void print_help() {
    printf("\n---------------------------------------------------------------------------------------\n");
    printf("read_arch - Read a VPR architecture file and output internal data structures\n");
    printf("\n");
    printf("Usage: read_arch <arch_file.xml> <timing_driven (0|1)> <output_file>\n");
    printf("\n");
    printf("  ex: read_arch k4_n10.xml 1 arch_data.out\n");
    printf("      Read timing-driven architecture k4_n10.xml and output the results to arch_data.out\n");
    printf("\n---------------------------------------------------------------------------------------\n");
}
