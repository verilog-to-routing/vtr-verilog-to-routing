#include <cstdio>
#include <cstring>

#include "vtr_util.h"
#include "vtr_memory.h"

#include "vpr_types.h"
#include "vpr_error.h"

#include "hash.h"
#include "echo_files.h"
#include "globals.h"

static bool EchoEnabled;

static bool* echoFileEnabled = nullptr;
static char** echoFileNames = nullptr;

static char** outputFileNames = nullptr;

bool getEchoEnabled() {
    return EchoEnabled;
}

void setEchoEnabled(bool echo_enabled) {
    /* enable echo outputs */
    EchoEnabled = echo_enabled;
    if (echoFileEnabled == nullptr) {
        /* initialize default echo options */
        alloc_and_load_echo_file_info();
    }
}

void setAllEchoFileEnabled(bool value) {
    int i;
    for (i = 0; i < (int)E_ECHO_END_TOKEN; i++) {
        echoFileEnabled[i] = value;
    }
}

void setEchoFileEnabled(enum e_echo_files echo_option, bool value) {
    echoFileEnabled[(int)echo_option] = value;
}

void setEchoFileName(enum e_echo_files echo_option, const char* name) {
    if (echoFileNames[(int)echo_option] != nullptr) {
        delete[](echoFileNames[(int)echo_option]);
    }
    echoFileNames[(int)echo_option] = new char[strlen(name) + 1];
    strcpy(echoFileNames[(int)echo_option], name);
}

bool isEchoFileEnabled(enum e_echo_files echo_option) {
    if (echoFileEnabled == nullptr) {
        return false;
    } else {
        return echoFileEnabled[(int)echo_option];
    }
}

char* getEchoFileName(enum e_echo_files echo_option) {
    return echoFileNames[(int)echo_option];
}

void alloc_and_load_echo_file_info() {
    echoFileEnabled = new bool[(int)E_ECHO_END_TOKEN];
    echoFileNames = new char*[(int)E_ECHO_END_TOKEN];
    for (auto i = 0; i < (int)E_ECHO_END_TOKEN; i++) {
        echoFileEnabled[i] = false;
        echoFileNames[i] = NULL;
    }

    setAllEchoFileEnabled(getEchoEnabled());
    //User input nelist
    setEchoFileName(E_ECHO_ATOM_NETLIST_ORIG, "atom_netlist.orig.echo.blif");
    setEchoFileName(E_ECHO_ATOM_NETLIST_CLEANED, "atom_netlist.cleaned.echo.blif");

    //Vpr constraints
    setEchoFileName(E_ECHO_VPR_CONSTRAINTS, "vpr_constraints.echo");

    //Packing
    setEchoFileName(E_ECHO_CLUSTERS, "clusters.echo");

    //Intra-block routing
    setEchoFileName(E_ECHO_INTRA_LB_FAILED_ROUTE, "intra_lb_failed_route.echo");

    //Timing Graphs
    setEchoFileName(E_ECHO_TRACK_TO_PIN_MAP, "track_to_pin_map.echo");
    setEchoFileName(E_ECHO_PRE_PACKING_TIMING_GRAPH, "timing_graph.pre_pack.echo");
    setEchoFileName(E_ECHO_INITIAL_PLACEMENT_TIMING_GRAPH, "timing_graph.place_initial.echo");
    setEchoFileName(E_ECHO_FINAL_PLACEMENT_TIMING_GRAPH, "timing_graph.place_final.echo");
    setEchoFileName(E_ECHO_FINAL_ROUTING_TIMING_GRAPH, "timing_graph.route_final.echo");
    setEchoFileName(E_ECHO_ANALYSIS_TIMING_GRAPH, "timing_graph.analysis.echo");

    setEchoFileName(E_ECHO_PLACE_MACROS, "place_macros.echo");
    setEchoFileName(E_ECHO_INITIAL_CLB_PLACEMENT, "initial_clb_placement.echo");
    setEchoFileName(E_ECHO_INITIAL_PLACEMENT_SLACK, "initial_placement_slack.echo");
    setEchoFileName(E_ECHO_INITIAL_PLACEMENT_CRITICALITY, "initial_placement_criticality.echo");
    setEchoFileName(E_ECHO_END_CLB_PLACEMENT, "end_clb_placement.echo");
    setEchoFileName(E_ECHO_PLACEMENT_SINK_DELAYS, "placement_sink_delays.echo");
    setEchoFileName(E_ECHO_FINAL_PLACEMENT_SLACK, "final_placement_slack.echo");
    setEchoFileName(E_ECHO_FINAL_PLACEMENT_CRITICALITY, "final_placement_criticality.echo");
    setEchoFileName(E_ECHO_PLACEMENT_CRIT_PATH, "placement_crit_path.echo");
    setEchoFileName(E_ECHO_PLACEMENT_DELTA_DELAY_MODEL, "placement_delta_delay_model.echo");
    setEchoFileName(E_ECHO_PB_GRAPH, "pb_graph.echo");
    setEchoFileName(E_ECHO_LB_TYPE_RR_GRAPH, "lb_type_rr_graph.echo");
    setEchoFileName(E_ECHO_ARCH, "arch.echo");
    setEchoFileName(E_ECHO_PLACEMENT_CRITICAL_PATH, "placement_critical_path.echo");
    setEchoFileName(E_ECHO_PLACEMENT_LOWER_BOUND_SINK_DELAYS, "placement_lower_bound_sink_delays.echo");
    setEchoFileName(E_ECHO_PLACEMENT_LOGIC_SINK_DELAYS, "placement_logic_sink_delays.echo");
    setEchoFileName(E_ECHO_ROUTING_SINK_DELAYS, "routing_sink_delays.echo");
    setEchoFileName(E_ECHO_POST_PACK_NETLIST, "post_pack_netlist.blif");
    setEchoFileName(E_ECHO_NET_DELAY, "net_delay.echo");
    setEchoFileName(E_ECHO_CLUSTERING_TIMING_INFO, "clustering_timing_info.echo");
    setEchoFileName(E_ECHO_PRE_PACKING_SLACK, "pre_packing_slack.echo");
    setEchoFileName(E_ECHO_PRE_PACKING_CRITICALITY, "pre_packing_criticality.echo");
    setEchoFileName(E_ECHO_CLUSTERING_BLOCK_CRITICALITIES, "clustering_block_criticalities.echo");
    setEchoFileName(E_ECHO_PRE_PACKING_MOLECULES_AND_PATTERNS, "pre_packing_molecules_and_patterns.echo");
    setEchoFileName(E_ECHO_MEM, "mem.echo");
    setEchoFileName(E_ECHO_TIMING_CONSTRAINTS, "timing_constraints.echo");
    setEchoFileName(E_ECHO_CRITICAL_PATH, "critical_path.echo");
    setEchoFileName(E_ECHO_SLACK, "slack.echo");
    setEchoFileName(E_ECHO_COMPLETE_NET_TRACE, "complete_net_trace.echo");
    setEchoFileName(E_ECHO_SEG_DETAILS, "seg_details.txt");
    setEchoFileName(E_ECHO_CHAN_DETAILS, "chan_details.txt");
    setEchoFileName(E_ECHO_SBLOCK_PATTERN, "sblock_pattern.txt");
    setEchoFileName(E_ECHO_TRACK_TO_PIN_MAP, "track_to_pin_map.echo");
    setEchoFileName(E_ECHO_ENDPOINT_TIMING, "endpoint_timing.echo.json");
    setEchoFileName(E_ECHO_LOOKAHEAD_MAP, "lookahead_map.echo");
    setEchoFileName(E_ECHO_RR_GRAPH_INDEXED_DATA, "rr_indexed_data.echo");
    setEchoFileName(E_ECHO_COMPRESSED_GRIDS, "compressed_grids.echo");
}

void free_echo_file_info() {
    int i;
    if (echoFileEnabled != nullptr) {
        for (i = 0; i < (int)E_ECHO_END_TOKEN; i++) {
            if (echoFileNames[i] != nullptr) {
                delete[](echoFileNames[i]);
            }
        }
        delete[] echoFileNames;
        delete[] echoFileEnabled;
        echoFileNames = nullptr;
        echoFileEnabled = nullptr;
    }
}

void setOutputFileName(enum e_output_files ename, const char* name, const char* default_name) {
    if (outputFileNames == nullptr) {
        alloc_and_load_output_file_names(default_name);
    }
    if (outputFileNames[(int)ename] != nullptr) {
        delete[](outputFileNames[(int)ename]);
    }
    outputFileNames[(int)ename] = new char[strlen(name) + 1];
    strcpy(outputFileNames[(int)ename], name);
}

char* getOutputFileName(enum e_output_files ename) {
    return outputFileNames[(int)ename];
}

void alloc_and_load_output_file_names(const std::string default_name) {
    std::string name;

    if (outputFileNames == nullptr) {
        outputFileNames = new char*[(int)E_FILE_END_TOKEN];
        for (auto i = 0; i < (int)E_FILE_END_TOKEN; i++)
            outputFileNames[i] = nullptr;

        name = default_name + ".critical_path.out";
        setOutputFileName(E_CRIT_PATH_FILE, name.c_str(), default_name.c_str());

        name = default_name + ".slack.out";
        setOutputFileName(E_SLACK_FILE, name.c_str(), default_name.c_str());

        name = default_name + ".criticality.out";
        setOutputFileName(E_CRITICALITY_FILE, name.c_str(), default_name.c_str());
    }
}

void free_output_file_names() {
    int i;
    if (outputFileNames != nullptr) {
        for (i = 0; i < (int)E_FILE_END_TOKEN; i++) {
            if (outputFileNames[i] != nullptr) {
                delete[](outputFileNames[i]);
                outputFileNames[i] = nullptr;
            }
        }
        delete[] outputFileNames;
        outputFileNames = nullptr;
    }
}
