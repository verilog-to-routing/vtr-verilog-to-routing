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
        free(echoFileNames[(int)echo_option]);
    }
    echoFileNames[(int)echo_option] = vtr::strdup(name);
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
    echoFileEnabled = (bool*)vtr::calloc((int)E_ECHO_END_TOKEN, sizeof(bool));
    echoFileNames = (char**)vtr::calloc((int)E_ECHO_END_TOKEN, sizeof(char*));

    setAllEchoFileEnabled(getEchoEnabled());

    //User input nelist
    setEchoFileName(E_ECHO_ATOM_NETLIST_ORIG, "atom_netlist.orig.echo.blif");
    setEchoFileName(E_ECHO_ATOM_NETLIST_CLEANED, "atom_netlist.cleaned.echo.blif");

    //Intra-block routing
    setEchoFileName(E_ECHO_INTRA_LB_FAILED_ROUTE, "intra_lb_failed_route.echo");

    //Timing Graphs
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
    setEchoFileName(E_ECHO_ENDPOINT_TIMING, "endpoint_timing.echo.json");

    setEchoFileName(E_ECHO_LOOKAHEAD_MAP, "lookahead_map.echo");
}

void free_echo_file_info() {
    int i;
    if (echoFileEnabled != nullptr) {
        for (i = 0; i < (int)E_ECHO_END_TOKEN; i++) {
            if (echoFileNames[i] != nullptr) {
                free(echoFileNames[i]);
            }
        }
        free(echoFileNames);
        free(echoFileEnabled);
        echoFileNames = nullptr;
        echoFileEnabled = nullptr;
    }
}

void setOutputFileName(enum e_output_files ename, const char* name, const char* default_name) {
    if (outputFileNames == nullptr) {
        alloc_and_load_output_file_names(default_name);
    }
    if (outputFileNames[(int)ename] != nullptr) {
        free(outputFileNames[(int)ename]);
    }
    outputFileNames[(int)ename] = vtr::strdup(name);
}

char* getOutputFileName(enum e_output_files ename) {
    return outputFileNames[(int)ename];
}

void alloc_and_load_output_file_names(const std::string default_name) {
    char* name;

    if (outputFileNames == nullptr) {
        outputFileNames = (char**)vtr::calloc((int)E_FILE_END_TOKEN, sizeof(char*));

        name = (char*)vtr::malloc((strlen(default_name.c_str()) + 40) * sizeof(char));
        sprintf(name, "%s.critical_path.out", default_name.c_str());
        setOutputFileName(E_CRIT_PATH_FILE, name, default_name.c_str());

        sprintf(name, "%s.slack.out", default_name.c_str());
        setOutputFileName(E_SLACK_FILE, name, default_name.c_str());

        sprintf(name, "%s.criticality.out", default_name.c_str());
        setOutputFileName(E_CRITICALITY_FILE, name, default_name.c_str());

        free(name);
    }
}

void free_output_file_names() {
    int i;
    if (outputFileNames != nullptr) {
        for (i = 0; i < (int)E_FILE_END_TOKEN; i++) {
            if (outputFileNames[i] != nullptr) {
                free(outputFileNames[i]);
                outputFileNames[i] = nullptr;
            }
        }
        free(outputFileNames);
        outputFileNames = nullptr;
    }
}
