#include "vtr_assert.h"

#include "vtr_util.h"

#include "endpoint_timing.h"


#include "path_delay.h"
#include "globals.h"

void print_tnode_info(FILE* fp, int inode, const char* identifier);

void print_endpoint_timing(char* filename) {
    FILE* fp = vtr::fopen(filename, "w");

    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& timing_ctx = g_vpr_ctx.timing();

	vtr::vector_map<ClusterBlockId, std::vector<int>> tnode_lookup_from_pin_id = alloc_and_load_tnode_lookup_from_pin_id();

    fprintf(fp, "{\n");
    fprintf(fp, "  \"endpoint_timing\": [\n");

    std::vector<int> outpad_sink_tnodes;

    for(int inode = 0; inode < timing_ctx.num_tnodes; inode++) {
        if(timing_ctx.tnodes[inode].type == TN_OUTPAD_SINK) {
            outpad_sink_tnodes.push_back(inode);
        }
    }

    for(size_t i = 0; i < outpad_sink_tnodes.size(); ++i) {
        int inode = outpad_sink_tnodes[i];
        const char* identifier = cluster_ctx.clb_nlist.block_name(timing_ctx.tnodes[inode].block).c_str() + 4; //Trim out:
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

void print_tnode_info(FILE* fp, int inode, const char* identifier) {
    auto& timing_ctx = g_vpr_ctx.timing();
    fprintf(fp, "    {\n");
    fprintf(fp, "      \"node_identifier\": \"%s\",\n", identifier);
    fprintf(fp, "      \"tnode_id\": \"%d\",\n", inode);

    if(timing_ctx.tnodes[inode].type == TN_OUTPAD_SINK) {
        fprintf(fp, "      \"tnode_type\": \"TN_OUTPAD_SINK\",\n");
    } else {
        VTR_ASSERT(timing_ctx.tnodes[inode].type == TN_FF_SINK);
        fprintf(fp, "      \"tnode_type\": \"TN_FF_SINK\",\n");
    }

    double T_arr = timing_ctx.tnodes[inode].T_arr;
    double T_req = timing_ctx.tnodes[inode].T_req;

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
