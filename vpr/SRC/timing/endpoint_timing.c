#include <cassert>

#include "endpoint_timing.h"

#include "path_delay.h"
#include "globals.h"

void print_tnode_info(FILE* fp, int inode, char* identifier);

void print_endpoint_timing(char* filename) {
    FILE* fp = my_fopen(filename, "w", 0);

    int** tnode_lookup_from_pin_id = alloc_and_load_tnode_lookup_from_pin_id();
    
    fprintf(fp, "{\n");
    fprintf(fp, "  \"endpoint_timing\": [\n");

    std::vector<int> outpad_sink_tnodes;

    for(int inode = 0; inode < num_tnodes; inode++) {
        if(tnode[inode].type == TN_OUTPAD_SINK) {
            outpad_sink_tnodes.push_back(inode);
        }
    }
    for(size_t i = 0; i < outpad_sink_tnodes.size(); ++i) {
        int inode = outpad_sink_tnodes[i];
        char* identifier = block[tnode[inode].block].name + 4; //Trim out:
        print_tnode_info(fp, inode, identifier);

        if(i != outpad_sink_tnodes.size() - 1) {
            fprintf(fp, ",");
        }
        fprintf(fp, "\n");
    }

    fprintf(fp, "  ]\n");
    fprintf(fp, "}\n");


    free_tnode_lookup_from_pin_id(tnode_lookup_from_pin_id);

    fclose(fp);
}

void print_tnode_info(FILE* fp, int inode, char* identifier) {
    fprintf(fp, "    {\n");
    fprintf(fp, "      \"node_identifier\": \"%s\",\n", identifier);
    fprintf(fp, "      \"tnode_id\": \"%d\",\n", inode);

    if(tnode[inode].type == TN_OUTPAD_SINK) {
        fprintf(fp, "      \"tnode_type\": \"TN_OUTPAD_SINK\",\n");
    } else {
        assert(tnode[inode].type == TN_FF_SINK);
        fprintf(fp, "      \"tnode_type\": \"TN_FF_SINK\",\n");
    }

    double T_arr = tnode[inode].T_arr;
    double T_req = tnode[inode].T_req;

    if(T_arr <= HUGE_NEGATIVE_FLOAT) {
        //Un-specified (e.g. driven by constant generator
        T_arr = 0.;
    }
    if(T_req >= HUGE_POSITIVE_FLOAT) {
        //Un-specified (e.g. driven by constant generator
        T_req = 0.;
    }

    fprintf(fp, "      \"T_arr\": \"%g\",\n", T_arr);
    fprintf(fp, "      \"T_req\": \"%g\"\n", T_req);
     
    fprintf(fp, "    }");
}
