#ifndef ECHO_FILES_H
#define ECHO_FILES_H

enum e_echo_files {
	E_ECHO_INITIAL_CLB_PLACEMENT = 0,
    E_ECHO_PLACE_MACROS,
	E_ECHO_INITIAL_PLACEMENT_SLACK,
	E_ECHO_INITIAL_PLACEMENT_CRITICALITY,
	E_ECHO_END_CLB_PLACEMENT,
	E_ECHO_PLACEMENT_SINK_DELAYS,
	E_ECHO_FINAL_PLACEMENT_SLACK,
	E_ECHO_FINAL_PLACEMENT_CRITICALITY,
    E_ECHO_PLACEMENT_CRIT_PATH,
	E_ECHO_PLACEMENT_DELTA_DELAY_MODEL,
	E_ECHO_PB_GRAPH,
	E_ECHO_LB_TYPE_RR_GRAPH,
	E_ECHO_ARCH,
	E_ECHO_PLACEMENT_CRITICAL_PATH,
	E_ECHO_PLACEMENT_LOWER_BOUND_SINK_DELAYS,
	E_ECHO_PLACEMENT_LOGIC_SINK_DELAYS,
	E_ECHO_ROUTING_SINK_DELAYS,
	E_ECHO_POST_PACK_NETLIST,
	E_ECHO_BLIF_INPUT,
	E_ECHO_COMPRESSED_NETLIST,
	E_ECHO_NET_DELAY,
	E_ECHO_CLUSTERING_TIMING_INFO,
	E_ECHO_PRE_PACKING_SLACK,
	E_ECHO_PRE_PACKING_CRITICALITY,
	E_ECHO_CLUSTERING_BLOCK_CRITICALITIES,
	E_ECHO_PRE_PACKING_MOLECULES_AND_PATTERNS,
	E_ECHO_MEM,
	E_ECHO_RR_GRAPH,
	E_ECHO_TIMING_CONSTRAINTS,
	E_ECHO_CRITICAL_PATH,
	E_ECHO_SLACK,
	E_ECHO_COMPLETE_NET_TRACE,
	E_ECHO_SEG_DETAILS,
	E_ECHO_CHAN_DETAILS,
	E_ECHO_SBLOCK_PATTERN,
    E_ECHO_ENDPOINT_TIMING,

    //Timing Graphs
	E_ECHO_PRE_PACKING_TIMING_GRAPH,
	E_ECHO_INITIAL_PLACEMENT_TIMING_GRAPH,
	E_ECHO_FINAL_PLACEMENT_TIMING_GRAPH,
    E_ECHO_FINAL_ROUTING_TIMING_GRAPH,
	E_ECHO_ANALYSIS_TIMING_GRAPH,


	E_ECHO_END_TOKEN
};


enum e_output_files {
	E_CRIT_PATH_FILE,
	E_SLACK_FILE,
	E_CRITICALITY_FILE,
	E_FILE_END_TOKEN
};

bool getEchoEnabled(void);
void setEchoEnabled(bool echo_enabled);

void setAllEchoFileEnabled(bool value);
void setEchoFileEnabled(enum e_echo_files echo_option, bool value);
void setEchoFileName(enum e_echo_files echo_option, const char *name);

bool isEchoFileEnabled(enum e_echo_files echo_option);
char *getEchoFileName(enum e_echo_files echo_option);

void alloc_and_load_echo_file_info();
void free_echo_file_info();

void setOutputFileName(enum e_output_files ename, const char *name, const char* default_name);
char *getOutputFileName(enum e_output_files ename);
void alloc_and_load_output_file_names(const std::string default_name);
void free_output_file_names();

#endif
