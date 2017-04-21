#include <cstdio>
#include <cstring>
using namespace std;

#include "vtr_util.h"

#include "vpr_types.h"
#include "vpr_error.h"

#include "hash.h"
#include "OptionTokens.h"
#include "ReadOptions.h"
#include "globals.h"


static bool EchoEnabled;

static bool *echoFileEnabled = NULL;
static char **echoFileNames = NULL;

static char **outputFileNames = NULL;

/******** Function prototypes ********/

static char **ReadBaseToken(char **Args, enum e_OptionBaseToken *Token);
static void Error(const char *Token);
static char **ProcessOption(char **Args, t_options * Options);
static char **ReadFloat(char **Args, float *Val);
static char **ReadInt(char **Args, int *Val);
static char **ReadOnOff(char **Args, bool * Val);
static char **ReadClusterSeed(char **Args, enum e_cluster_seed *Type);
static char **ReadFixPins(char **Args, char **PinFile);
static char **ReadPlaceAlgorithm(char **Args,
		enum e_place_algorithm *Algo);
static char **ReadRouterAlgorithm(char **Args,
		enum e_router_algorithm *Algo);
static char **ReadPackerAlgorithm(char **Args,
		enum e_packer_algorithm *Algo);
static char **ReadRoutingPredictor(char **Args,
		enum e_routing_failure_predictor *RoutingPred);
static char **ReadBaseCostType(char **Args,
		enum e_base_cost_type *BaseCostType);
static char **ReadRouteType(char **Args, enum e_route_type *Type);
static char **ReadString(char **Args, char **Val);
static char **ReadChar(char **Args, char *Val);

/******** Globally Accessible Function ********/
/* Determines whether timing analysis should be on or off. 
 Unless otherwise specified, always default to timing.
 */
bool IsTimingEnabled(const t_options *Options) {
	/* First priority to the '--timing_analysis' flag */
	if (Options->Count[OT_TIMING_ANALYSIS]) {
		return Options->TimingAnalysis;
	}
	return true;
}

/* Determines whether file echo should be on or off. 
 Unless otherwise specified, always default to on.
 */
bool IsEchoEnabled(const t_options *Options) {
	/* First priority to the '--echo_file' flag */
	if (Options->Count[OT_CREATE_ECHO_FILE]) {
		return Options->CreateEchoFile;
	}
	return false;
}


bool getEchoEnabled(void) {
	return EchoEnabled;
}

void setEchoEnabled(bool echo_enabled) {
	/* enable echo outputs */
	EchoEnabled = echo_enabled;
	if(echoFileEnabled == NULL) {
		/* initialize default echo options */
		alloc_and_load_echo_file_info();
	}
}

void setAllEchoFileEnabled(bool value) {
	int i;
	for(i = 0; i < (int) E_ECHO_END_TOKEN; i++) {
		echoFileEnabled[i] = value;
	}
}

void setEchoFileEnabled(enum e_echo_files echo_option, bool value) {
	echoFileEnabled[(int)echo_option] = value;
}

void setEchoFileName(enum e_echo_files echo_option, const char *name) {
	if(echoFileNames[(int)echo_option] != NULL) {
		free(echoFileNames[(int)echo_option]);
	}
	echoFileNames[(int)echo_option] = vtr::strdup(name);
}

bool isEchoFileEnabled(enum e_echo_files echo_option) {
	if(echoFileEnabled == NULL) {
		return false;
	} else {
		return echoFileEnabled[(int)echo_option];
	}
}
char *getEchoFileName(enum e_echo_files echo_option) {
	return echoFileNames[(int)echo_option];
}

void alloc_and_load_echo_file_info() {
	echoFileEnabled = (bool*)vtr::calloc((int) E_ECHO_END_TOKEN, sizeof(bool));
	echoFileNames = (char**)vtr::calloc((int) E_ECHO_END_TOKEN, sizeof(char*));

	setAllEchoFileEnabled(getEchoEnabled());

    //Timing Graphs
	setEchoFileName(E_ECHO_PRE_PACKING_TIMING_GRAPH, "timing_graph.pre_pack.echo");
	setEchoFileName(E_ECHO_INITIAL_PLACEMENT_TIMING_GRAPH, "timing_graph.place_initial.echo");
	setEchoFileName(E_ECHO_FINAL_PLACEMENT_TIMING_GRAPH, "timing_graph.place_final.echo");
	setEchoFileName(E_ECHO_FINAL_ROUTING_TIMING_GRAPH, "timing_graph.route_final.echo");
	setEchoFileName(E_ECHO_ANALYSIS_TIMING_GRAPH, "timing_graph.analysis.echo");

	setEchoFileName(E_ECHO_INITIAL_CLB_PLACEMENT, "initial_clb_placement.echo");
	setEchoFileName(E_ECHO_INITIAL_PLACEMENT_SLACK, "initial_placement_slack.echo");
	setEchoFileName(E_ECHO_INITIAL_PLACEMENT_CRITICALITY, "initial_placement_criticality.echo");
	setEchoFileName(E_ECHO_END_CLB_PLACEMENT, "end_clb_placement.echo");
	setEchoFileName(E_ECHO_PLACEMENT_SINK_DELAYS, "placement_sink_delays.echo");
	setEchoFileName(E_ECHO_FINAL_PLACEMENT_SLACK, "final_placement_slack.echo");
	setEchoFileName(E_ECHO_FINAL_PLACEMENT_CRITICALITY, "final_placement_criticality.echo");
	setEchoFileName(E_ECHO_PLACEMENT_CRIT_PATH, "placement_crit_path.echo");
	setEchoFileName(E_ECHO_PLACEMENT_DELTA_DELAY_MODEL, "placement_delta_delay_model.%s.echo");
	setEchoFileName(E_ECHO_PB_GRAPH, "pb_graph.echo");
	setEchoFileName(E_ECHO_LB_TYPE_RR_GRAPH, "lb_type_rr_graph.echo");
	setEchoFileName(E_ECHO_ARCH, "arch.echo");
	setEchoFileName(E_ECHO_PLACEMENT_CRITICAL_PATH, "placement_critical_path.echo");
	setEchoFileName(E_ECHO_PLACEMENT_LOWER_BOUND_SINK_DELAYS, "placement_lower_bound_sink_delays.echo");
	setEchoFileName(E_ECHO_PLACEMENT_LOGIC_SINK_DELAYS, "placement_logic_sink_delays.echo");
	setEchoFileName(E_ECHO_ROUTING_SINK_DELAYS, "routing_sink_delays.echo");
	setEchoFileName(E_ECHO_POST_PACK_NETLIST, "post_pack_netlist.blif");
	setEchoFileName(E_ECHO_BLIF_INPUT, "blif_input.echo");
	setEchoFileName(E_ECHO_COMPRESSED_NETLIST, "compressed_netlist.echo");
	setEchoFileName(E_ECHO_NET_DELAY, "net_delay.echo");
	setEchoFileName(E_ECHO_CLUSTERING_TIMING_INFO, "clustering_timing_info.echo");
	setEchoFileName(E_ECHO_PRE_PACKING_SLACK, "pre_packing_slack.echo");
	setEchoFileName(E_ECHO_PRE_PACKING_CRITICALITY, "pre_packing_criticality.echo");
	setEchoFileName(E_ECHO_CLUSTERING_BLOCK_CRITICALITIES, "clustering_block_criticalities.echo");
	setEchoFileName(E_ECHO_PRE_PACKING_MOLECULES_AND_PATTERNS, "pre_packing_molecules_and_patterns.echo");
	setEchoFileName(E_ECHO_MEM, "mem.echo");
	setEchoFileName(E_ECHO_RR_GRAPH, "rr_graph.echo");
	setEchoFileName(E_ECHO_TIMING_CONSTRAINTS, "timing_constraints.echo");	
	setEchoFileName(E_ECHO_CRITICAL_PATH, "critical_path.echo");	
	setEchoFileName(E_ECHO_SLACK, "slack.echo");	
	setEchoFileName(E_ECHO_COMPLETE_NET_TRACE, "complete_net_trace.echo");
	setEchoFileName(E_ECHO_SEG_DETAILS, "seg_details.txt");
	setEchoFileName(E_ECHO_CHAN_DETAILS, "chan_details.txt");
	setEchoFileName(E_ECHO_SBLOCK_PATTERN, "sblock_pattern.txt");
	setEchoFileName(E_ECHO_ENDPOINT_TIMING, "endpoint_timing.echo.json");
}

void free_echo_file_info() {
	int i;
	if(echoFileEnabled != NULL) {
		for(i = 0; i < (int) E_ECHO_END_TOKEN; i++) {
			if(echoFileNames[i] != NULL) {
				free(echoFileNames[i]);
			}
		}
		free(echoFileNames);
		free(echoFileEnabled);
		echoFileNames = NULL;
		echoFileEnabled = NULL;
	}
}

void setOutputFileName(enum e_output_files ename, const char *name, const char *default_name) {
	if(outputFileNames == NULL) {
		alloc_and_load_output_file_names(default_name);
	}
	if(outputFileNames[(int)ename] != NULL) {
		free(outputFileNames[(int)ename]);
	}
	outputFileNames[(int)ename] = vtr::strdup(name);
}

char *getOutputFileName(enum e_output_files ename) {
	return outputFileNames[(int)ename];
}

void alloc_and_load_output_file_names(const char *default_name) {
	char *name;

	if(outputFileNames == NULL) {

		outputFileNames = (char**)vtr::calloc((int)E_FILE_END_TOKEN, sizeof(char*));

		name = (char*)vtr::malloc((strlen(default_name) + 40) * sizeof(char));
		sprintf(name, "%s.critical_path.out", default_name);
		setOutputFileName(E_CRIT_PATH_FILE, name, default_name);
	
		sprintf(name, "%s.slack.out", default_name);
		setOutputFileName(E_SLACK_FILE, name, default_name);

		sprintf(name, "%s.criticality.out", default_name);
		setOutputFileName(E_CRITICALITY_FILE, name, default_name);

		free(name);
	}
}

void free_output_file_names() {
	int i;
	if(outputFileNames != NULL) {
		for(i = 0; i < (int)E_FILE_END_TOKEN; i++) {
			if(outputFileNames[i] != NULL) {
				free(outputFileNames[i]);
				outputFileNames[i] = NULL;
			}
		}
		free(outputFileNames);
		outputFileNames = NULL;
	}
}



/******** Subroutine implementations ********/

void ReadOptions(int argc, const char **argv, t_options * Options) {
	char **Args, **head;
	int offset;
	
	/* Clear values and pointers to zero */
	memset(Options, 0, sizeof(t_options));

	/* Alloc a new pointer list for args with a NULL at end.
	 * This makes parsing the same as for archfile for consistency.
	 * Skips the first arg as it is the program image path */
	--argc;
	++argv;
	head = Args = (char **) vtr::malloc(sizeof(char *) * (argc + 1));
	memcpy(Args, argv, (sizeof(char *) * argc));
	Args[argc] = NULL;

	/* Go through the command line args. If they have hyphens they are 
	 * options. Otherwise assume they are part of the four mandatory
	 * arguments */
	while (*Args) {
		if (strncmp("--", *Args, 2) == 0) {
			*Args += 2; /* Skip the prefix */
			Args = ProcessOption(Args, Options);
		} else if (strncmp("-", *Args, 1) == 0) {
			*Args += 1; /* Skip the prefix */
			Args = ProcessOption(Args, Options);
		} else if (NULL == Options->ArchFile) {
			Options->ArchFile = vtr::strdup(*Args);
			++Args;
		} else if (NULL == Options->CircuitName) {
			Options->CircuitName = vtr::strdup(*Args);
			/*if the user entered the circuit name with the .blif extension, remove it now*/
			offset = strlen(Options->CircuitName) - 5;
			if (offset > 0 && !strcmp(Options->CircuitName + offset, ".blif")) {
				Options->CircuitName[offset] = '\0';
			}
			++Args;
		} else {
			/* Not an option and arch and net already specified so fail */
			Error(*Args);
		}
	}
	free(head);
}

static char **
ProcessOption(char **Args, t_options * Options) {
	enum e_OptionBaseToken Token;
	char **PrevArgs;

	PrevArgs = Args;
	Args = ReadBaseToken(Args, &Token);
	
	if (Token < OT_BASE_UNKNOWN) {
		/* If this was previously set by a lower priority source
		 * (ie. a settings file), reset the provenance and the
		 * count */
		if (Options->Provenance[Token])
		{
			Options->Provenance[Token] = 0;
			Options->Count[Token] = 1;
		}
		else
			++Options->Count[Token];
	}

	switch (Token) {
		/* File naming options */
	case OT_BLIF_FILE:
		return ReadString(Args, &Options->BlifFile);
	case OT_NET_FILE:
		return ReadString(Args, &Options->NetFile);
	case OT_PLACE_FILE:
		return ReadString(Args, &Options->PlaceFile);
	case OT_ROUTE_FILE:
		return ReadString(Args, &Options->RouteFile);
	case OT_SDC_FILE:
		return ReadString(Args, &Options->SDCFile);
	case OT_DUMP_RR_STRUCTS_FILE:
		return ReadString(Args, &Options->dump_rr_structs_file);
		/* General Options */
	case OT_VERSION:
        Options->show_version = true;
	case OT_DISP:
		return ReadOnOff(Args, &Options->show_graphics);
	case OT_CONGESTION_ANALYSIS:
	case OT_FANOUT_ANALYSIS:
    case OT_SWITCH_USAGE_ANALYSIS:
		return Args;
	case OT_AUTO:
		return ReadInt(Args, &Options->GraphPause);
	case OT_PACK:
	case OT_ROUTE:
	case OT_PLACE:
	case OT_ANALYSIS:
		return Args;
    case OT_SLACK_DEFINITION:
        return ReadChar(Args, &Options->SlackDefinition);
	case OT_FAST:
	case OT_FULL_STATS:
		return Args;
	case OT_TIMING_ANALYSIS:
		return ReadOnOff(Args, &Options->TimingAnalysis);
	case OT_OUTFILE_PREFIX:
		return ReadString(Args, &Options->out_file_prefix);
	case OT_CREATE_ECHO_FILE:
		return ReadOnOff(Args, &Options->CreateEchoFile);
	case OT_GEN_NELIST_AS_BLIF:
		return Args;
	case OT_GENERATE_POST_SYNTHESIS_NETLIST:          
	  return ReadOnOff(Args, &Options->Generate_Post_Synthesis_Netlist);

    //Atom netlist options
	case OT_ABSORB_BUFFER_LUTS:
		return ReadOnOff(Args, &Options->absorb_buffer_luts);
	case OT_SWEEP_DANGLING_PRIMARY_IOS:
		return ReadOnOff(Args, &Options->sweep_dangling_primary_ios);
	case OT_SWEEP_DANGLING_NETS:
		return ReadOnOff(Args, &Options->sweep_dangling_nets);
	case OT_SWEEP_DANGLING_BLOCKS:
		return ReadOnOff(Args, &Options->sweep_dangling_blocks);
	case OT_SWEEP_CONSTANT_PRIMARY_OUTPUTS:
		return ReadOnOff(Args, &Options->sweep_constant_primary_outputs);

    /* Clustering Options */
	case OT_GLOBAL_CLOCKS:
		return ReadOnOff(Args, &Options->global_clocks);
	case OT_HILL_CLIMBING_FLAG:
		return ReadOnOff(Args, &Options->hill_climbing_flag);
	case OT_TIMING_DRIVEN_CLUSTERING:
		return ReadOnOff(Args, &Options->timing_driven);
	case OT_CLUSTER_SEED:
		return ReadClusterSeed(Args, &Options->cluster_seed_type);
	case OT_ALPHA_CLUSTERING:
		return ReadFloat(Args, &Options->alpha);
	case OT_BETA_CLUSTERING:
		return ReadFloat(Args, &Options->beta);
	case OT_ALLOW_UNRELATED_CLUSTERING:
		return ReadOnOff(Args, &Options->allow_unrelated_clustering);
	case OT_INTER_CLUSTER_NET_DELAY:
		return ReadFloat(Args, &Options->inter_cluster_net_delay);
	case OT_CONNECTION_DRIVEN_CLUSTERING:
		return ReadOnOff(Args, &Options->connection_driven);
	case OT_SKIP_CLUSTERING:
		return Args;
	case OT_PACKER_ALGORITHM:
		return ReadPackerAlgorithm(Args, &Options->packer_algorithm);

		/* Placer Options */
	case OT_PLACE_ALGORITHM:
		return ReadPlaceAlgorithm(Args, &Options->PlaceAlgorithm);
	case OT_INIT_T:
		return ReadFloat(Args, &Options->PlaceInitT);
	case OT_EXIT_T:
		return ReadFloat(Args, &Options->PlaceExitT);
	case OT_ALPHA_T:
		return ReadFloat(Args, &Options->PlaceAlphaT);
	case OT_INNER_NUM:
		return ReadFloat(Args, &Options->PlaceInnerNum);
	case OT_SEED:
		return ReadInt(Args, &Options->Seed);
	case OT_PLACE_COST_EXP:
		return ReadFloat(Args, &Options->place_cost_exp);
	case OT_PLACE_CHAN_WIDTH:
		return ReadInt(Args, &Options->PlaceChanWidth);
	case OT_FIX_PINS:
		return ReadFixPins(Args, &Options->PinFile);
	case OT_ENABLE_TIMING_COMPUTATIONS:
		return ReadOnOff(Args, &Options->ShowPlaceTiming);

		/* Placement Options Valid Only for Timing-Driven Placement */
	case OT_TIMING_TRADEOFF:
		return ReadFloat(Args, &Options->PlaceTimingTradeoff);
	case OT_RECOMPUTE_CRIT_ITER:
		return ReadInt(Args, &Options->RecomputeCritIter);
	case OT_INNER_LOOP_RECOMPUTE_DIVIDER:
		return ReadInt(Args, &Options->inner_loop_recompute_divider);
	case OT_TD_PLACE_EXP_FIRST:
		return ReadFloat(Args, &Options->place_exp_first);
	case OT_TD_PLACE_EXP_LAST:
		return ReadFloat(Args, &Options->place_exp_last);

		/* Router Options */
	case OT_MAX_ROUTER_ITERATIONS:
		return ReadInt(Args, &Options->max_router_iterations);
	case OT_MIN_INCREMENTAL_REROUTE_FANOUT:
		return ReadInt(Args, &Options->min_incremental_reroute_fanout);
	case OT_BB_FACTOR:
		return ReadInt(Args, &Options->bb_factor);
	case OT_INITIAL_PRES_FAC:
		return ReadFloat(Args, &Options->initial_pres_fac);
	case OT_PRES_FAC_MULT:
		return ReadFloat(Args, &Options->pres_fac_mult);
	case OT_ACC_FAC:
		return ReadFloat(Args, &Options->acc_fac);
	case OT_FIRST_ITER_PRES_FAC:
		return ReadFloat(Args, &Options->first_iter_pres_fac);
	case OT_BEND_COST:
		return ReadFloat(Args, &Options->bend_cost);
	case OT_ROUTE_TYPE:
		return ReadRouteType(Args, &Options->RouteType);
	case OT_VERIFY_BINARY_SEARCH:
		return Args;
	case OT_ROUTE_CHAN_WIDTH:
		return ReadInt(Args, &Options->RouteChanWidth);
	case OT_MIN_ROUTE_CHAN_WIDTH_HINT:
		return ReadInt(Args, &Options->min_route_chan_width_hint);
	case OT_TRIM_EMPTY_CHAN:
		return ReadOnOff(Args, &Options->TrimEmptyChan);
	case OT_TRIM_OBS_CHAN:
		return ReadOnOff(Args, &Options->TrimObsChan);
	case OT_ROUTER_ALGORITHM:
		return ReadRouterAlgorithm(Args, &Options->RouterAlgorithm);
	case OT_BASE_COST_TYPE:
		return ReadBaseCostType(Args, &Options->base_cost_type);

		/* Routing options valid only for timing-driven routing */
	case OT_ASTAR_FAC:
		return ReadFloat(Args, &Options->astar_fac);
	case OT_MAX_CRITICALITY:
		return ReadFloat(Args, &Options->max_criticality);
	case OT_CRITICALITY_EXP:
		return ReadFloat(Args, &Options->criticality_exp);
	case OT_ROUTING_FAILURE_PREDICTOR:
		return ReadRoutingPredictor(Args, &Options->routing_failure_predictor);

		/* Power options */
	case OT_POWER:
		return Args;
	case OT_ACTIVITY_FILE:
		return ReadString(Args, &Options->ActFile);
	case OT_POWER_OUT_FILE:
		return ReadString(Args, &Options->PowerFile);
	case OT_CMOS_TECH_BEHAVIOR_FILE:
		return ReadString(Args, &Options->CmosTechFile);

	default:
		vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__, 
			"Unexpected option '%s' on command line.\n", *PrevArgs);
		return NULL;
	}
}

static char **
ReadBaseToken(char **Args, enum e_OptionBaseToken *Token) {
	struct s_TokenPair *Cur;

	/* Empty string is end of tokens marker */
	if (NULL == *Args)
		Error(*Args);

	/* Linear search for the pair */
	Cur = OptionBaseTokenList;
	while (Cur->Str) {
		if (strcmp(*Args, Cur->Str) == 0) {
			*Token = (enum e_OptionBaseToken) Cur->Enum;
			return ++Args;
		}
		++Cur;
	}

	*Token = OT_BASE_UNKNOWN;
	return ++Args;
}

static char **
ReadToken(char **Args, enum e_OptionArgToken *Token) {
	struct s_TokenPair *Cur;

	/* Empty string is end of tokens marker */
	if (NULL == *Args)
		Error(*Args);

	/* Linear search for the pair */
	Cur = OptionArgTokenList;
	while (Cur->Str) {
		if (strcmp(*Args, Cur->Str) == 0) {
			*Token = (enum e_OptionArgToken)Cur->Enum;
			return ++Args;
		}
		++Cur;
	}

	*Token = OT_ARG_UNKNOWN;
	return ++Args;
}

/* Called for parse errors. Spits out a message and then exits program. */
static void Error(const char *Token) {
	if (Token) {
		vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__, 
		"Unexpected token '%s' on command line.\n", Token);
	} else {
		vpr_throw(VPR_ERROR_OTHER,__FILE__, __LINE__, 
		"Missing token at end of command line.\n");
	}
}

static char **
ReadClusterSeed(char **Args, enum e_cluster_seed *Type) {
	enum e_OptionArgToken Token;
	char **PrevArgs;

	PrevArgs = Args;
	Args = ReadToken(Args, &Token);
	switch (Token) {
	case OT_TIMING:
		*Type = VPACK_TIMING;
		break;
	case OT_MAX_INPUTS:
		*Type = VPACK_MAX_INPUTS;
		break;
	case OT_BLEND:
		*Type = VPACK_BLEND;
		break;
	default:
		Error(*PrevArgs);
	}

	return Args;
}

static char **
ReadPackerAlgorithm(char **Args, enum e_packer_algorithm *Algo) {
	enum e_OptionArgToken Token;
	char **PrevArgs;

	PrevArgs = Args;
	Args = ReadToken(Args, &Token);
	switch (Token) {
	case OT_GREEDY:
		*Algo = PACK_GREEDY;
		break;
	case OT_BRUTE_FORCE:
		*Algo = PACK_BRUTE_FORCE;
		break;
	default:
		Error(*PrevArgs);
	}

	return Args;
}


static char **
ReadRouterAlgorithm(char **Args, enum e_router_algorithm *Algo) {
	enum e_OptionArgToken Token;
	char **PrevArgs;

	PrevArgs = Args;
	Args = ReadToken(Args, &Token);
	switch (Token) {
	case OT_BREADTH_FIRST:
		*Algo = BREADTH_FIRST;
		break;
	case OT_NO_TIMING:
		*Algo = NO_TIMING;
		break;
	case OT_TIMING_DRIVEN:
		*Algo = TIMING_DRIVEN;
		break;
	default:
		Error(*PrevArgs);
	}

	return Args;
}

static char **
ReadRoutingPredictor(char **Args, enum e_routing_failure_predictor *RoutingPred) {
	enum e_OptionArgToken Token;
	char **PrevArgs;

	PrevArgs = Args;
	Args = ReadToken(Args, &Token);
	switch (Token) {
	case OT_OFF:
		*RoutingPred = OFF;
		break;
	case OT_ROUTING_FAILURE_SAFE:
		*RoutingPred = SAFE;
		break;
	case OT_ROUTING_FAILURE_AGGRESSIVE:
		*RoutingPred = AGGRESSIVE;
		break;
	default:
		Error(*PrevArgs);
	}

	return Args;
}

static char **
ReadBaseCostType(char **Args, enum e_base_cost_type *BaseCostType) {
	enum e_OptionArgToken Token;
	char **PrevArgs;

	PrevArgs = Args;
	Args = ReadToken(Args, &Token);
	switch (Token) {
	case OT_DELAY_NORMALIZED:
		*BaseCostType = DELAY_NORMALIZED;
		break;
	case OT_DEMAND_ONLY:
		*BaseCostType = DEMAND_ONLY;
		break;
	default:
		Error(*PrevArgs);
	}

	return Args;
}

static char **
ReadRouteType(char **Args, enum e_route_type *Type) {
	enum e_OptionArgToken Token;
	char **PrevArgs;

	PrevArgs = Args;
	Args = ReadToken(Args, &Token);
	switch (Token) {
	case OT_GLOBAL:
		*Type = GLOBAL;
		break;
	case OT_DETAILED:
		*Type = DETAILED;
		break;
	default:
		Error(*PrevArgs);
	}

	return Args;
}

static char **
ReadPlaceAlgorithm(char **Args, enum e_place_algorithm *Algo) {
	enum e_OptionArgToken Token;
	char **PrevArgs;

	PrevArgs = Args;
	Args = ReadToken(Args, &Token);
	switch (Token) {
	case OT_BOUNDING_BOX:
		*Algo = BOUNDING_BOX_PLACE;
		break;
	case OT_PATH_TIMING_DRIVEN:
		*Algo = PATH_TIMING_DRIVEN_PLACE;
		break;
	default:
		Error(*PrevArgs);
	}

	return Args;
}

static char **
ReadFixPins(char **Args, char **PinFile) {
	enum e_OptionArgToken Token;
	int Len;
	char **PrevArgs = Args;

	Args = ReadToken(Args, &Token);
	if (OT_RANDOM != Token) {
		Len = 1 + strlen(*PrevArgs);
		*PinFile = (char *) vtr::malloc(Len * sizeof(char));
		memcpy(*PinFile, *PrevArgs, Len);
	}
	return Args;
}

static char **
ReadOnOff(char **Args, bool * Val) {
	enum e_OptionArgToken Token;
	char **PrevArgs;

	PrevArgs = Args;
	Args = ReadToken(Args, &Token);
	switch (Token) {
	case OT_ON:
		*Val = true;
		break;
	case OT_OFF:
		*Val = false;
		break;
	default:
		Error(*PrevArgs);
	}
	return Args;
}

static char **
ReadInt(char **Args, int *Val) {
	if (NULL == *Args)
		Error(*Args);
	if ((**Args > '9') || (**Args < '0'))
		Error(*Args);

	*Val = vtr::atoi(*Args);

	return ++Args;
}

static char **
ReadFloat(char ** Args, float *Val) {
	if (NULL == *Args) {
		Error(*Args);
	}

	if ((**Args != '-') && (**Args != '.')
			&& ((**Args > '9') || (**Args < '0'))) {
		Error(*Args);
	}

	*Val = vtr::atof(*Args);

	return ++Args;
}

static char **
ReadString(char **Args, char **Val) {
	if (NULL == *Args) {
		Error(*Args);
	}

	*Val = vtr::strdup(*Args);

	return ++Args;
}

static char **
ReadChar(char **Args, char *Val) {
    if (NULL == *Args) {
        Error(*Args);
    }

    *Val = (*Args)[0];

    return ++Args;
}

