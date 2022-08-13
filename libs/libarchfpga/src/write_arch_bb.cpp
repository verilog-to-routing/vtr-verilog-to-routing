/**
 * Reading an architecture and output the Verilog black box
 * declaration of complex blocks in a file
 *
 * Date: July, 2022
 * Author: Seyed Alireza Damghani
 */

#include <stdio.h>
#include <stdlib.h>
#include <vector>

#include "vtr_error.h"
#include "vtr_memory.h"

#include "arch_util.h"
#include "read_xml_arch_file.h"
#include "write_models_bb.h"

void print_help();

int main(int argc, char** argv) {
    try {
        t_arch arch;
        std::vector<t_physical_tile_type> physical_tile_types;
        std::vector<t_logical_block_type> logical_block_types;

        if (argc - 1 != 2) {
            printf("Error: Unexpected # of arguments.  Expected 2 found %d arguments\n",
                   argc);
            print_help();
            return 1;
        }

        printf("-------------------------------------------------------------------------------------------------------\n");
        printf("- Read architecture file and generate a Verilog file including the declaration of models as black boxes\n");
        printf("-------------------------------------------------------------------------------------------------------\n\n");

        printf(
            "Inputs: \n"
            "architecture %s\n"
            "output file %s\n",
            argv[1], argv[2]);
        printf("Reading in architecture ...\n");

        /* function declarations */
        XmlReadArch(argv[1], false, &arch, physical_tile_types, logical_block_types);

        printf("Printing Results ...\n");

        WriteModels_bb(argv[1], argv[2], &arch);

        // CLEAN UP
        free_arch(&arch);
        free_type_descriptors(physical_tile_types);
        free_type_descriptors(logical_block_types);

    } catch (vtr::VtrError& vtr_error) {
        printf("Failed to process architecture %s: %s\n", argv[1], vtr_error.what());
        return 1;
    } catch (std::exception& error) {
        printf("Failed to process architecture %s: %s\n", argv[1], error.what());
        return 1;
    }

    printf("Done\n");

    return 0;
}

void print_help() {
    printf("\n-----------------------------------------------------------------------------------------------------------------------\n");
    printf("write_arch_bb - Read a VPR architecture file and output a Verilog file including the declaration of models as black boxes\n");
    printf("\n");
    printf("Usage: write_arch_bb <arch_file.xml> <output_file>\n");
    printf("\n");
    printf("  ex: write_arch_bb k4_n10.xml dsp_bb.v\n");
    printf("      Read timing-driven architecture k4_n10.xml and output the results to arch_data.out\n");
    printf("\n-----------------------------------------------------------------------------------------------------------------------\n");
}
