#include <cstring>
using namespace std;

#include <assert.h>
#include <vector>

#include "util.h"
#include "vpr_types.h"
#include "OptionTokens.h"
#include "ReadOptions.h"
#include "globals.h"
#include "read_xml_arch_file.h"
#include "SetupVPR.h"
#include "pb_type_graph.h"
#include "pack_types.h"
#include "lb_type_rr_graph.h"
#include "ReadOptions.h"
#include "rr_graph_area.h"

static void SetupOperation(INP t_options Options,
		OUTP enum e_operation *Operation);
static void SetupPackerOpts(INP t_options Options, INP bool TimingEnabled,
		INP t_arch Arch, INP char *net_file,
		OUTP struct s_packer_opts *PackerOpts);
static void SetupPlacerOpts(INP t_options Options, INP bool TimingEnabled,
		OUTP struct s_placer_opts *PlacerOpts);
static void SetupAnnealSched(INP t_options Options,
		OUTP struct s_annealing_sched *AnnealSched);
static void SetupRouterOpts(INP t_options Options, INP bool TimingEnabled,
		OUTP struct s_router_opts *RouterOpts);
static void SetupRoutingArch(INP t_arch Arch,
		OUTP struct s_det_routing_arch *RoutingArch);
static void SetupTiming(INP t_options Options, INP t_arch Arch,
		INP bool TimingEnabled, INP enum e_operation Operation,
		INP struct s_placer_opts PlacerOpts,
		INP struct s_router_opts RouterOpts, OUTP t_timing_inf * Timing);
static void SetupSwitches(INP t_arch Arch,
		INOUTP struct s_det_routing_arch *RoutingArch,
		INP struct s_arch_switch_inf *ArchSwitches, INP int NumArchSwitches);
static void SetupPowerOpts(t_options Options, t_power_opts *power_opts,
		t_arch * Arch);

/* Sets VPR parameters and defaults. Does not do any error checking
 * as this should have been done by the various input checkers */
void SetupVPR(INP t_options *Options, INP bool TimingEnabled,
		INP bool readArchFile, OUTP struct s_file_name_opts *FileNameOpts,
		INOUTP t_arch * Arch, OUTP enum e_operation *Operation,
		OUTP t_model ** user_models, OUTP t_model ** library_models,
		OUTP struct s_packer_opts *PackerOpts,
		OUTP struct s_placer_opts *PlacerOpts,
		OUTP struct s_annealing_sched *AnnealSched,
		OUTP struct s_router_opts *RouterOpts,
		OUTP struct s_det_routing_arch *RoutingArch,
		OUTP vector <t_lb_type_rr_node> **PackerRRGraphs,
		OUTP t_segment_inf ** Segments, OUTP t_timing_inf * Timing,
		OUTP bool * ShowGraphics, OUTP int *GraphPause,
		t_power_opts * PowerOpts) {
	int i, j, len;

	if (!Options->CircuitName) vpr_throw(VPR_ERROR_BLIF_F,__FILE__, __LINE__, "No blif file found in arguments (did you specify an architecture file?)\n");
	len = strlen(Options->CircuitName) + 6; /* circuit_name.blif/0*/
	if (Options->out_file_prefix != NULL ) {
		len += strlen(Options->out_file_prefix);
	}
	default_output_name = (char*) my_calloc(len, sizeof(char));
	if (Options->out_file_prefix == NULL ) {
		sprintf(default_output_name, "%s", Options->CircuitName);
	} else {
		sprintf(default_output_name, "%s%s", Options->out_file_prefix,
				Options->CircuitName);
	}

	/* init default filenames */
	if (Options->BlifFile == NULL ) {
		len = strlen(Options->CircuitName) + 6; /* circuit_name.blif/0*/
		if (Options->out_file_prefix != NULL ) {
			len += strlen(Options->out_file_prefix);
		}
		Options->BlifFile = (char*) my_calloc(len, sizeof(char));
		if (Options->out_file_prefix == NULL ) {
			sprintf(Options->BlifFile, "%s.blif", Options->CircuitName);
		} else {
			sprintf(Options->BlifFile, "%s%s.blif", Options->out_file_prefix,
					Options->CircuitName);
		}
	}

	if (Options->NetFile == NULL ) {
		len = strlen(Options->CircuitName) + 5; /* circuit_name.net/0*/
		if (Options->out_file_prefix != NULL ) {
			len += strlen(Options->out_file_prefix);
		}
		Options->NetFile = (char*) my_calloc(len, sizeof(char));
		if (Options->out_file_prefix == NULL ) {
			sprintf(Options->NetFile, "%s.net", Options->CircuitName);
		} else {
			sprintf(Options->NetFile, "%s%s.net", Options->out_file_prefix,
					Options->CircuitName);
		}
	}

	if (Options->PlaceFile == NULL ) {
		len = strlen(Options->CircuitName) + 7; /* circuit_name.place/0*/
		if (Options->out_file_prefix != NULL ) {
			len += strlen(Options->out_file_prefix);
		}
		Options->PlaceFile = (char*) my_calloc(len, sizeof(char));
		if (Options->out_file_prefix == NULL ) {
			sprintf(Options->PlaceFile, "%s.place", Options->CircuitName);
		} else {
			sprintf(Options->PlaceFile, "%s%s.place", Options->out_file_prefix,
					Options->CircuitName);
		}
	}

	if (Options->RouteFile == NULL ) {
		len = strlen(Options->CircuitName) + 7; /* circuit_name.route/0*/
		if (Options->out_file_prefix != NULL ) {
			len += strlen(Options->out_file_prefix);
		}
		Options->RouteFile = (char*) my_calloc(len, sizeof(char));
		if (Options->out_file_prefix == NULL ) {
			sprintf(Options->RouteFile, "%s.route", Options->CircuitName);
		} else {
			sprintf(Options->RouteFile, "%s%s.route", Options->out_file_prefix,
					Options->CircuitName);
		}
	}
	if (Options->ActFile == NULL ) {
		len = strlen(Options->CircuitName) + 7; /* circuit_name.route/0*/
		if (Options->out_file_prefix != NULL ) {
			len += strlen(Options->out_file_prefix);
		}
		Options->ActFile = (char*) my_calloc(len, sizeof(char));
		if (Options->out_file_prefix == NULL ) {
			sprintf(Options->ActFile, "%s.act", Options->CircuitName);
		} else {
			sprintf(Options->ActFile, "%s%s.act", Options->out_file_prefix,
					Options->CircuitName);
		}
	}

	if (Options->PowerFile == NULL ) {
		len = strlen(Options->CircuitName) + 7; /* circuit_name.route/0*/
		if (Options->out_file_prefix != NULL ) {
			len += strlen(Options->out_file_prefix);
		}
		Options->PowerFile = (char*) my_calloc(len, sizeof(char));
		if (Options->out_file_prefix == NULL ) {
			sprintf(Options->PowerFile, "%s.power", Options->CircuitName);
		} else {
			sprintf(Options->ActFile, "%s%s.power", Options->out_file_prefix,
					Options->CircuitName);
		}
	}

	alloc_and_load_output_file_names(default_output_name);

	FileNameOpts->CircuitName = Options->CircuitName;
	FileNameOpts->ArchFile = Options->ArchFile;
	FileNameOpts->BlifFile = Options->BlifFile;
	FileNameOpts->NetFile = Options->NetFile;
	FileNameOpts->PlaceFile = Options->PlaceFile;
	FileNameOpts->RouteFile = Options->RouteFile;
	FileNameOpts->ActFile = Options->ActFile;
	FileNameOpts->PowerFile = Options->PowerFile;
	FileNameOpts->CmosTechFile = Options->CmosTechFile;
	FileNameOpts->out_file_prefix = Options->out_file_prefix;

	SetupOperation(*Options, Operation);
	SetupPlacerOpts(*Options, TimingEnabled, PlacerOpts);
	SetupAnnealSched(*Options, AnnealSched);
	SetupRouterOpts(*Options, TimingEnabled, RouterOpts);
	SetupPowerOpts(*Options, PowerOpts, Arch);

	if (readArchFile == true) {
		XmlReadArch(Options->ArchFile, TimingEnabled, Arch, &type_descriptors,
				&num_types);
	}

	*user_models = Arch->models;
	*library_models = Arch->model_library;

	/* TODO: this is inelegant, I should be populating this information in XmlReadArch */
	EMPTY_TYPE = NULL;
	FILL_TYPE = NULL;
	IO_TYPE = NULL;
	for (i = 0; i < num_types; i++) {
		if (strcmp(type_descriptors[i].name, "<EMPTY>") == 0) {
			EMPTY_TYPE = &type_descriptors[i];
		} else if (strcmp(type_descriptors[i].name, "io") == 0) {
			IO_TYPE = &type_descriptors[i];
		} else {
			for (j = 0; j < type_descriptors[i].num_grid_loc_def; j++) {
				if (type_descriptors[i].grid_loc_def[j].grid_loc_type == FILL) {
					assert(FILL_TYPE == NULL);
					FILL_TYPE = &type_descriptors[i];
				}
			}
		}
	}
	assert(EMPTY_TYPE != NULL && FILL_TYPE != NULL && IO_TYPE != NULL);

	*Segments = Arch->Segments;
	RoutingArch->num_segment = Arch->num_segments;

#ifdef INTERPOSER_BASED_ARCHITECTURE
	/* getting the data from the options struct  for interposer-based architectures*/
	percent_wires_cut = Options->percent_wires_cut;
	num_cuts = Options->num_cuts;
	delay_increase = Options->delay_increase;
	placer_cost_constant = Options->placer_cost_constant;
	constant_type = Options->constant_type;

	/* used for experiments of interposer-based architectures */
	allow_chanx_interposer_connections = Options->allow_chanx_interposer_connections? true: false;
	transfer_interposer_fanins = Options->transfer_interposer_fanins? true: false;
	allow_additional_interposer_fanins = Options->allow_additional_interposer_fanins? true: false;
	pct_of_interposer_nodes_each_chany_can_drive = Options->pct_of_interposer_nodes_each_chany_can_drive;
	transfer_interposer_fanouts = Options->transfer_interposer_fanouts? true: false;
	allow_additional_interposer_fanouts = Options->allow_additional_interposer_fanouts? true: false;
	pct_of_chany_wires_an_interposer_node_can_drive = Options->pct_of_chany_wires_an_interposer_node_can_drive;
#endif

	SetupSwitches(*Arch, RoutingArch, Arch->Switches, Arch->num_switches);
	SetupRoutingArch(*Arch, RoutingArch);
	SetupTiming(*Options, *Arch, TimingEnabled, *Operation, *PlacerOpts,
			*RouterOpts, Timing);
	SetupPackerOpts(*Options, TimingEnabled, *Arch, Options->NetFile,
			PackerOpts);
	RoutingArch->dump_rr_structs_file = Options->dump_rr_structs_file;

	/* init global variables */
	out_file_prefix = Options->out_file_prefix;
	grid_logic_tile_area = Arch->grid_logic_tile_area;

	/* Set seed for pseudo-random placement, default seed to 1 */
	PlacerOpts->seed = 1;
	if (Options->Count[OT_SEED]) {
		PlacerOpts->seed = Options->Seed;
	}
	my_srandom(PlacerOpts->seed);

	vpr_printf_info("Building complex block graph.\n");
	alloc_and_load_all_pb_graphs(PowerOpts->do_power);
	*PackerRRGraphs = alloc_and_load_all_lb_type_rr_graph();
	if (getEchoEnabled() && isEchoFileEnabled(E_ECHO_LB_TYPE_RR_GRAPH)) {
		echo_lb_type_rr_graphs(getEchoFileName(E_ECHO_LB_TYPE_RR_GRAPH),*PackerRRGraphs);
	}

	if (getEchoEnabled() && isEchoFileEnabled(E_ECHO_PB_GRAPH)) {
		echo_pb_graph(getEchoFileName(E_ECHO_PB_GRAPH));
	}

	*GraphPause = 1; /* DEFAULT */
	if (Options->Count[OT_AUTO]) {
		*GraphPause = Options->GraphPause;
	}
#ifdef NO_GRAPHICS
	*ShowGraphics = false; /* DEFAULT */
#else /* NO_GRAPHICS */
	*ShowGraphics = true; /* DEFAULT */
	if (Options->Count[OT_NODISP]) {
		*ShowGraphics = false;
	}
#endif /* NO_GRAPHICS */

	if (getEchoEnabled() && isEchoFileEnabled(E_ECHO_ARCH)) {
		EchoArch(getEchoFileName(E_ECHO_ARCH), type_descriptors, num_types,
				Arch);
	}

}

static void SetupTiming(INP t_options Options, INP t_arch Arch,
		INP bool TimingEnabled, INP enum e_operation Operation,
		INP struct s_placer_opts PlacerOpts,
		INP struct s_router_opts RouterOpts, OUTP t_timing_inf * Timing) {

	/* Don't do anything if they don't want timing */
	if (false == TimingEnabled) {
		memset(Timing, 0, sizeof(t_timing_inf));
		Timing->timing_analysis_enabled = false;
		return;
	}

	Timing->C_ipin_cblock = Arch.C_ipin_cblock;
	Timing->T_ipin_cblock = Arch.T_ipin_cblock;
	Timing->timing_analysis_enabled = TimingEnabled;

	/* If the user specified an SDC filename on the command line, look for specified_name.sdc, otherwise look for circuit_name.sdc*/
	if (Options.SDCFile == NULL ) {
		Timing->SDCFile = (char*) my_calloc(strlen(Options.CircuitName) + 5,
				sizeof(char)); /* circuit_name.sdc/0*/
		sprintf(Timing->SDCFile, "%s.sdc", Options.CircuitName);
	} else {
		Timing->SDCFile = (char*) my_strdup(Options.SDCFile);
	}

    if (Options.SlackDefinition != '\0') {
        Timing->slack_definition = Options.SlackDefinition;
        assert(Timing->slack_definition == 'R' || Timing->slack_definition == 'I' ||
               Timing->slack_definition == 'S' || Timing->slack_definition == 'G' ||
               Timing->slack_definition == 'C' || Timing->slack_definition == 'N');
    } else {
        Timing->slack_definition = 'R'; // default
    }
}

/* This loads up VPR's arch_switch_inf data by combining the switches from 
 * the arch file with the special switches that VPR needs. */
static void SetupSwitches(INP t_arch Arch,
		INOUTP struct s_det_routing_arch *RoutingArch,
		INP struct s_arch_switch_inf *ArchSwitches, INP int NumArchSwitches) {

	int switches_to_copy = NumArchSwitches;
	g_num_arch_switches = NumArchSwitches;

	/* If ipin cblock info has not been read in from a switch, then we will
	   create a new switch for it. Otherwise, the switch already exists */
	if (NULL == Arch.ipin_cblock_switch_name){
		/* Depends on g_num_arch_switches */
		RoutingArch->wire_to_arch_ipin_switch = g_num_arch_switches;
		++g_num_arch_switches;
		++NumArchSwitches;
	} else {
		/* need to find the index of the input cblock switch */
		int ipin_cblock_switch_index = -1;
		char *ipin_cblock_switch_name = Arch.ipin_cblock_switch_name;
		for (int iswitch = 0; iswitch < g_num_arch_switches; iswitch++){
			char *iswitch_name = ArchSwitches[iswitch].name;
			if (0 == strcmp(ipin_cblock_switch_name, iswitch_name)){
				ipin_cblock_switch_index = iswitch;
				break;
			}
		}
		if (ipin_cblock_switch_index == -1){
			vpr_throw(VPR_ERROR_OTHER,__FILE__, __LINE__, "Could not find arch switch matching name %s\n", ipin_cblock_switch_name);
		}

		RoutingArch->wire_to_arch_ipin_switch = ipin_cblock_switch_index;
	}


#ifdef INTERPOSER_BASED_ARCHITECTURE
	/* Order of the switches will be as follows:
	 * 0..NumArchSwitches-1: Original switches from arch file (including wire to ipin switch)
	 * NumArchSwitches...2*NumArchSwitches-1: switches with increased delay
	 * 2*NumArchSwitches: delayless switch */
	int i;
	double d_delay_increase;

	/* Convert the delay to seconds, as it is given as int in ps */
	d_delay_increase = (double)delay_increase * 1e-12;
	vpr_printf_info("delay_increase:%d d_delay_increase: %g\n", delay_increase, d_delay_increase);
	

	/* ANDRE: Adds the extra switch types with increased delay */
	g_num_arch_switches *= 2;

	/* Depends on RoutingArch->num_arch_switches */
	RoutingArch->delayless_switch = g_num_arch_switches;
	RoutingArch->global_route_switch = RoutingArch->delayless_switch;
	++g_num_arch_switches;

	/* Alloc the list now that we know the final num_arch_switches value */
	g_arch_switch_inf = new s_arch_switch_inf[g_num_arch_switches];

	/* Mapping of the switches to their respective increased delay versions */
	increased_delay_edge_map = (int *) my_malloc(sizeof(int) * g_num_arch_switches);
	
	/* create delay edge map which maps from a normal switch to its increased-delay counterpart */
	for(i = 0; i < switches_to_copy; i++){
		increased_delay_edge_map[i] = i + NumArchSwitches;
		increased_delay_edge_map[i+NumArchSwitches] = i + NumArchSwitches;
	}
	increased_delay_edge_map[RoutingArch->delayless_switch] = RoutingArch->delayless_switch;

	/* copy switches that were specified in the architecture file */
	for (int iswitch = 0; iswitch < NumArchSwitches; iswitch++){
		g_arch_switch_inf[iswitch] = ArchSwitches[iswitch];
	}

	/* If ipin cblock info has *not* been read in from a switch, then we have
	   created a new switch for it, and now need to set its values */
	if (NULL == Arch.ipin_cblock_switch_name){
		/* The wire to ipin switch for all types. Curently all types
		 * must share ipin switch. Some of the timing code would
		 * need to be changed otherwise. */
		g_arch_switch_inf[RoutingArch->wire_to_arch_ipin_switch].buffered = true;
		g_arch_switch_inf[RoutingArch->wire_to_arch_ipin_switch].R = 0.;
		g_arch_switch_inf[RoutingArch->wire_to_arch_ipin_switch].Cin = Arch.C_ipin_cblock;
		g_arch_switch_inf[RoutingArch->wire_to_arch_ipin_switch].Cout = 0.;
		g_arch_switch_inf[RoutingArch->wire_to_arch_ipin_switch].Tdel_map[UNDEFINED] = Arch.T_ipin_cblock;
		g_arch_switch_inf[RoutingArch->wire_to_arch_ipin_switch].power_buffer_type = POWER_BUFFER_TYPE_NONE;
		g_arch_switch_inf[RoutingArch->wire_to_arch_ipin_switch].mux_trans_size = Arch.ipin_mux_trans_size;

		/* Assume the ipin cblock output to lblock input buffer below is 4x     *
		 * minimum drive strength (enough to drive a fanout of up to 16 pretty  * 
		 * nicely) -- should cover a reasonable wiring C plus the fanout.       */
		g_arch_switch_inf[RoutingArch->wire_to_arch_ipin_switch].buf_size = trans_per_buf(Arch.R_minW_nmos / 4., Arch.R_minW_nmos, Arch.R_minW_pmos);
	}

	/* Actually create the new switch types for the switch boxes */
	for(i = NumArchSwitches; i < 2*NumArchSwitches; i++){
		g_arch_switch_inf[i] = g_arch_switch_inf[i-NumArchSwitches];
		std::map<int, double> *Tdel_map = &g_arch_switch_inf[i].Tdel_map;
		std::map<int, double>::iterator it;

		for (it = Tdel_map->begin(); it != Tdel_map->end(); it++){
			it->second += d_delay_increase;
		}
	}

	/* Delayless switch for connecting sinks and sources with their pins. */
	g_arch_switch_inf[RoutingArch->delayless_switch].buffered = true;
	g_arch_switch_inf[RoutingArch->delayless_switch].R = 0.;
	g_arch_switch_inf[RoutingArch->delayless_switch].Cin = 0.;
	g_arch_switch_inf[RoutingArch->delayless_switch].Cout = 0.;
	g_arch_switch_inf[RoutingArch->delayless_switch].Tdel_map[UNDEFINED] = 0.;
	g_arch_switch_inf[RoutingArch->delayless_switch].power_buffer_type = POWER_BUFFER_TYPE_NONE;
	g_arch_switch_inf[RoutingArch->delayless_switch].mux_trans_size = 0.;

#else


	/* Depends on g_num_arch_switches */
	RoutingArch->delayless_switch = g_num_arch_switches;
	RoutingArch->global_route_switch = RoutingArch->delayless_switch;
	++g_num_arch_switches;

	/* Alloc the list now that we know the final num_arch_switches value */
	g_arch_switch_inf = new s_arch_switch_inf[g_num_arch_switches];
	for (int iswitch = 0; iswitch < switches_to_copy; iswitch++){
		g_arch_switch_inf[iswitch] = ArchSwitches[iswitch];
	}

	/* Delayless switch for connecting sinks and sources with their pins. */
	g_arch_switch_inf[RoutingArch->delayless_switch].buffered = true;
	g_arch_switch_inf[RoutingArch->delayless_switch].R = 0.;
	g_arch_switch_inf[RoutingArch->delayless_switch].Cin = 0.;
	g_arch_switch_inf[RoutingArch->delayless_switch].Cout = 0.;
	g_arch_switch_inf[RoutingArch->delayless_switch].Tdel_map[UNDEFINED] = 0.;
	g_arch_switch_inf[RoutingArch->delayless_switch].power_buffer_type = POWER_BUFFER_TYPE_NONE;
	g_arch_switch_inf[RoutingArch->delayless_switch].mux_trans_size = 0.;

	/* If ipin cblock info has *not* been read in from a switch, then we have
	   created a new switch for it, and now need to set its values */
	if (NULL == Arch.ipin_cblock_switch_name){
		/* The wire to ipin switch for all types. Curently all types
		 * must share ipin switch. Some of the timing code would
		 * need to be changed otherwise. */
		g_arch_switch_inf[RoutingArch->wire_to_arch_ipin_switch].buffered = true;
		g_arch_switch_inf[RoutingArch->wire_to_arch_ipin_switch].R = 0.;
		g_arch_switch_inf[RoutingArch->wire_to_arch_ipin_switch].Cin = Arch.C_ipin_cblock;
		g_arch_switch_inf[RoutingArch->wire_to_arch_ipin_switch].Cout = 0.;
		g_arch_switch_inf[RoutingArch->wire_to_arch_ipin_switch].Tdel_map[UNDEFINED] = Arch.T_ipin_cblock;
		g_arch_switch_inf[RoutingArch->wire_to_arch_ipin_switch].power_buffer_type = POWER_BUFFER_TYPE_NONE;
		g_arch_switch_inf[RoutingArch->wire_to_arch_ipin_switch].mux_trans_size = Arch.ipin_mux_trans_size;

		/* Assume the ipin cblock output to lblock input buffer below is 4x     *
		 * minimum drive strength (enough to drive a fanout of up to 16 pretty  * 
		 * nicely) -- should cover a reasonable wiring C plus the fanout.       */
		g_arch_switch_inf[RoutingArch->wire_to_arch_ipin_switch].buf_size = trans_per_buf(Arch.R_minW_nmos / 4., Arch.R_minW_nmos, Arch.R_minW_pmos);
	}
#endif


}

/* Sets up routing structures. Since checks are already done, this
 * just copies values across */
static void SetupRoutingArch(INP t_arch Arch,
		OUTP struct s_det_routing_arch *RoutingArch) {

	RoutingArch->switch_block_type = Arch.SBType;
	RoutingArch->R_minW_nmos = Arch.R_minW_nmos;
	RoutingArch->R_minW_pmos = Arch.R_minW_pmos;
	RoutingArch->Fs = Arch.Fs;
	RoutingArch->directionality = BI_DIRECTIONAL;
	if (Arch.Segments){
		RoutingArch->directionality = Arch.Segments[0].directionality;
	}

	/* copy over the switch block information */
	RoutingArch->switchblocks = Arch.switchblocks;
}

static void SetupRouterOpts(INP t_options Options, INP bool TimingEnabled,
		OUTP struct s_router_opts *RouterOpts) {
	RouterOpts->astar_fac = 1.2; /* DEFAULT */
	if (Options.Count[OT_ASTAR_FAC]) {
		RouterOpts->astar_fac = Options.astar_fac;
	}

	RouterOpts->bb_factor = 3; /* DEFAULT */
	if (Options.Count[OT_FAST]) {
		RouterOpts->bb_factor = 0; /* DEFAULT */
	}
	if (Options.Count[OT_BB_FACTOR]) {
		RouterOpts->bb_factor = Options.bb_factor;
	}

	RouterOpts->criticality_exp = 1.0; /* DEFAULT */
	if (Options.Count[OT_CRITICALITY_EXP]) {
		RouterOpts->criticality_exp = Options.criticality_exp;
	}

	RouterOpts->max_criticality = 0.99; /* DEFAULT */
	if (Options.Count[OT_MAX_CRITICALITY]) {
		RouterOpts->max_criticality = Options.max_criticality;
	}

	RouterOpts->max_router_iterations = 50; /* DEFAULT */
	if (Options.Count[OT_FAST]) {
		RouterOpts->max_router_iterations = 10;
	}
	if (Options.Count[OT_MAX_ROUTER_ITERATIONS]) {
		RouterOpts->max_router_iterations = Options.max_router_iterations;
	}

	/* Based on testing, choosing a low threshold can lead to instability
	   where sometimes route time and critical path are degraded. 64 seems
	   to be a reasonable choice for most circuits. For nets with a greater
	   distribution of high fanout nets, choose a larger threshold */
	RouterOpts->min_incremental_reroute_fanout = 64;	
	if (Options.Count[OT_MIN_INCREMENTAL_REROUTE_FANOUT]) {
		RouterOpts->min_incremental_reroute_fanout = Options.min_incremental_reroute_fanout;
	}

	RouterOpts->pres_fac_mult = 1.3; /* DEFAULT */
	if (Options.Count[OT_PRES_FAC_MULT]) {
		RouterOpts->pres_fac_mult = Options.pres_fac_mult;
	}

	RouterOpts->route_type = DETAILED; /* DEFAULT */
	if (Options.Count[OT_ROUTE_TYPE]) {
		RouterOpts->route_type = Options.RouteType;
	}

	RouterOpts->full_stats = false; /* DEFAULT */
	if (Options.Count[OT_FULL_STATS]) {
		RouterOpts->full_stats = true;
	}

	RouterOpts->congestion_analysis = false;
	if (Options.Count[OT_CONGESTION_ANALYSIS]) {
		RouterOpts->congestion_analysis = true;
	}

	RouterOpts->fanout_analysis = false;
	if (Options.Count[OT_FANOUT_ANALYSIS]) {
		RouterOpts->fanout_analysis = true;
	}

    RouterOpts->switch_usage_analysis = false;
    if (Options.Count[OT_SWITCH_USAGE_ANALYSIS]) {
        RouterOpts->switch_usage_analysis = true;
    }

	RouterOpts->verify_binary_search = false; /* DEFAULT */
	if (Options.Count[OT_VERIFY_BINARY_SEARCH]) {
		RouterOpts->verify_binary_search = true;
	}

	/* Depends on RouteOpts->route_type */
	RouterOpts->router_algorithm = NO_TIMING; /* DEFAULT */
	if (TimingEnabled) {
		RouterOpts->router_algorithm = TIMING_DRIVEN; /* DEFAULT */
	}
	if (GLOBAL == RouterOpts->route_type) {
		RouterOpts->router_algorithm = NO_TIMING; /* DEFAULT */
	}
	if (Options.Count[OT_ROUTER_ALGORITHM]) {
		RouterOpts->router_algorithm = Options.RouterAlgorithm;
	}

	RouterOpts->fixed_channel_width = NO_FIXED_CHANNEL_WIDTH; /* DEFAULT */
	if (Options.Count[OT_ROUTE_CHAN_WIDTH]) {
		RouterOpts->fixed_channel_width = Options.RouteChanWidth;
	}

	RouterOpts->trim_empty_channels = false; /* DEFAULT */
	if (Options.Count[OT_TRIM_EMPTY_CHAN]) {
		RouterOpts->trim_empty_channels = Options.TrimEmptyChan;
	}
	RouterOpts->trim_obs_channels = false; /* DEFAULT */
	if (Options.Count[OT_TRIM_OBS_CHAN]) {
		RouterOpts->trim_obs_channels = Options.TrimObsChan;
	}

	/* Depends on RouterOpts->router_algorithm */
	RouterOpts->initial_pres_fac = 0.5; /* DEFAULT */
	if (NO_TIMING == RouterOpts->router_algorithm || Options.Count[OT_FAST]) {
		RouterOpts->initial_pres_fac = 10000.0; /* DEFAULT */
	}
	if (Options.Count[OT_INITIAL_PRES_FAC]) {
		RouterOpts->initial_pres_fac = Options.initial_pres_fac;
	}

	/* Depends on RouterOpts->router_algorithm */
	RouterOpts->base_cost_type = DELAY_NORMALIZED; /* DEFAULT */
	if (BREADTH_FIRST == RouterOpts->router_algorithm) {
		RouterOpts->base_cost_type = DEMAND_ONLY; /* DEFAULT */
	}
	if (NO_TIMING == RouterOpts->router_algorithm) {
		RouterOpts->base_cost_type = DEMAND_ONLY; /* DEFAULT */
	}
	if (Options.Count[OT_BASE_COST_TYPE]) {
		RouterOpts->base_cost_type = Options.base_cost_type;
	}

	/* Depends on RouterOpts->router_algorithm */
	RouterOpts->first_iter_pres_fac = 0.5; /* DEFAULT */
	if (BREADTH_FIRST == RouterOpts->router_algorithm) {
		RouterOpts->first_iter_pres_fac = 0.0; /* DEFAULT */
	}
	if (NO_TIMING == RouterOpts->router_algorithm || Options.Count[OT_FAST]) {
		RouterOpts->first_iter_pres_fac = 10000.0; /* DEFAULT */
	}
	if (Options.Count[OT_FIRST_ITER_PRES_FAC]) {
		RouterOpts->first_iter_pres_fac = Options.first_iter_pres_fac;
	}

	/* Depends on RouterOpts->router_algorithm */
	RouterOpts->acc_fac = 1.0;
	if (BREADTH_FIRST == RouterOpts->router_algorithm) {
		RouterOpts->acc_fac = 0.2;
	}
	if (Options.Count[OT_ACC_FAC]) {
		RouterOpts->acc_fac = Options.acc_fac;
	}

	/* Depends on RouterOpts->route_type */
	RouterOpts->bend_cost = 0.0; /* DEFAULT */
	if (GLOBAL == RouterOpts->route_type) {
		RouterOpts->bend_cost = 1.0; /* DEFAULT */
	}
	if (Options.Count[OT_BEND_COST]) {
		RouterOpts->bend_cost = Options.bend_cost;
	}

	RouterOpts->doRouting = false;
	if (Options.Count[OT_ROUTE]) {
		RouterOpts->doRouting = true;
	} else if (!Options.Count[OT_PACK] && !Options.Count[OT_PLACE]
			&& !Options.Count[OT_ROUTE]) {
		if (!Options.Count[OT_TIMING_ANALYZE_ONLY_WITH_NET_DELAY])
			RouterOpts->doRouting = true;
	}

	/* Andre: Default value for the predictor is SAFE */
	RouterOpts->routing_failure_predictor = SAFE;
	if (Options.Count[OT_ROUTING_FAILURE_PREDICTOR]) {
		RouterOpts->routing_failure_predictor = Options.routing_failure_predictor;
	}

}

static void SetupAnnealSched(INP t_options Options,
		OUTP struct s_annealing_sched *AnnealSched) {
	AnnealSched->alpha_t = 0.8; /* DEFAULT */
	if (Options.Count[OT_ALPHA_T]) {
		AnnealSched->alpha_t = Options.PlaceAlphaT;
	}
	if (AnnealSched->alpha_t >= 1 || AnnealSched->alpha_t <= 0) {
		vpr_throw(VPR_ERROR_OTHER,__FILE__, __LINE__, "alpha_t must be between 0 and 1 exclusive.\n");
	}
	AnnealSched->exit_t = 0.01; /* DEFAULT */
	if (Options.Count[OT_EXIT_T]) {
		AnnealSched->exit_t = Options.PlaceExitT;
	}
	if (AnnealSched->exit_t <= 0) {
		vpr_throw(VPR_ERROR_OTHER,__FILE__, __LINE__, "exit_t must be greater than 0.\n");
	}
	AnnealSched->init_t = 100.0; /* DEFAULT */
	if (Options.Count[OT_INIT_T]) {
		AnnealSched->init_t = Options.PlaceInitT;
	}
	if (AnnealSched->init_t <= 0) {
		vpr_throw(VPR_ERROR_OTHER,__FILE__, __LINE__, "init_t must be greater than 0.\n");
	}
	if (AnnealSched->init_t < AnnealSched->exit_t) {
		vpr_throw(VPR_ERROR_OTHER,__FILE__, __LINE__, "init_t must be greater or equal to than exit_t.\n");
	}
	AnnealSched->inner_num = 1.0; /* DEFAULT */
	if (Options.Count[OT_INNER_NUM]) {
		AnnealSched->inner_num = Options.PlaceInnerNum;
	}
	if (AnnealSched->inner_num <= 0) {
		vpr_throw(VPR_ERROR_OTHER,__FILE__, __LINE__, "init_t must be greater than 0.\n");
	}
	AnnealSched->type = AUTO_SCHED; /* DEFAULT */
	if ((Options.Count[OT_ALPHA_T]) || (Options.Count[OT_EXIT_T])
			|| (Options.Count[OT_INIT_T])) {
		AnnealSched->type = USER_SCHED;
	}
}

/* Sets up the s_packer_opts structure baesd on users inputs and on the architecture specified.  
 * Error checking, such as checking for conflicting params is assumed to be done beforehand 
 */
void SetupPackerOpts(INP t_options Options, INP bool TimingEnabled,
		INP t_arch Arch, INP char *net_file,
		OUTP struct s_packer_opts *PackerOpts) {

	if (Arch.clb_grid.IsAuto) {
		PackerOpts->aspect = Arch.clb_grid.Aspect;
	} else {
		PackerOpts->aspect = (float) Arch.clb_grid.H / (float) Arch.clb_grid.W;
	}
	PackerOpts->output_file = net_file;

	PackerOpts->blif_file_name = Options.BlifFile;

	PackerOpts->doPacking = false; /* DEFAULT */
	if (Options.Count[OT_PACK]) {
		PackerOpts->doPacking = true;
	} else if (!Options.Count[OT_PACK] && !Options.Count[OT_PLACE]
			&& !Options.Count[OT_ROUTE]) {
		if (!Options.Count[OT_TIMING_ANALYZE_ONLY_WITH_NET_DELAY])
			PackerOpts->doPacking = true;
	}

	PackerOpts->global_clocks = true; /* DEFAULT */
	if (Options.Count[OT_GLOBAL_CLOCKS]) {
		PackerOpts->global_clocks = Options.global_clocks;
	}

	PackerOpts->hill_climbing_flag = false; /* DEFAULT */
	if (Options.Count[OT_HILL_CLIMBING_FLAG]) {
		PackerOpts->hill_climbing_flag = Options.hill_climbing_flag;
	}

	PackerOpts->sweep_hanging_nets_and_inputs = true;
	if (Options.Count[OT_SWEEP_HANGING_NETS_AND_INPUTS]) {
		PackerOpts->sweep_hanging_nets_and_inputs =
				Options.sweep_hanging_nets_and_inputs;
	}

	PackerOpts->skip_clustering = false; /* DEFAULT */
	if (Options.Count[OT_SKIP_CLUSTERING]) {
		PackerOpts->skip_clustering = true;
	}
	PackerOpts->allow_unrelated_clustering = true; /* DEFAULT */
	if (Options.Count[OT_ALLOW_UNRELATED_CLUSTERING]) {
		PackerOpts->allow_unrelated_clustering =
				Options.allow_unrelated_clustering;
	}
	PackerOpts->allow_early_exit = false; /* DEFAULT */
	if (Options.Count[OT_ALLOW_EARLY_EXIT]) {
		PackerOpts->allow_early_exit = Options.allow_early_exit;
	}
	PackerOpts->connection_driven = true; /* DEFAULT */
	if (Options.Count[OT_CONNECTION_DRIVEN_CLUSTERING]) {
		PackerOpts->connection_driven = Options.connection_driven;
	}

	PackerOpts->timing_driven = TimingEnabled; /* DEFAULT */
	if (Options.Count[OT_TIMING_DRIVEN_CLUSTERING]) {
		PackerOpts->timing_driven = Options.timing_driven;
	}
	PackerOpts->cluster_seed_type = (
			TimingEnabled ? VPACK_BLEND : VPACK_MAX_INPUTS); /* DEFAULT */
	if (Options.Count[OT_CLUSTER_SEED]) {
		PackerOpts->cluster_seed_type = Options.cluster_seed_type;
	}
	PackerOpts->alpha = 0.75; /* DEFAULT */
	if (Options.Count[OT_ALPHA_CLUSTERING]) {
		PackerOpts->alpha = Options.alpha;
	}
	PackerOpts->beta = 0.9; /* DEFAULT */
	if (Options.Count[OT_BETA_CLUSTERING]) {
		PackerOpts->beta = Options.beta;
	}

	/* never recomputer timing */
	PackerOpts->recompute_timing_after = MAX_SHORT; /* DEFAULT */
	if (Options.Count[OT_RECOMPUTE_TIMING_AFTER]) {
		PackerOpts->recompute_timing_after = Options.recompute_timing_after;
	}
	PackerOpts->block_delay = 0; /* DEFAULT */
	if (Options.Count[OT_CLUSTER_BLOCK_DELAY]) {
		PackerOpts->block_delay = Options.block_delay;
	}
	PackerOpts->intra_cluster_net_delay = 0; /* DEFAULT */
	if (Options.Count[OT_INTRA_CLUSTER_NET_DELAY]) {
		PackerOpts->intra_cluster_net_delay = Options.intra_cluster_net_delay;
	}
	PackerOpts->inter_cluster_net_delay = 1.0; /* DEFAULT */
	PackerOpts->auto_compute_inter_cluster_net_delay = true;
	if (Options.Count[OT_INTER_CLUSTER_NET_DELAY]) {
		PackerOpts->inter_cluster_net_delay = Options.inter_cluster_net_delay;
		PackerOpts->auto_compute_inter_cluster_net_delay = false;
	}

	PackerOpts->packer_algorithm = PACK_GREEDY; /* DEFAULT */
	if (Options.Count[OT_PACKER_ALGORITHM]) {
		PackerOpts->packer_algorithm = Options.packer_algorithm;
	}
}

/* Sets up the s_placer_opts structure based on users input. Error checking,
 * such as checking for conflicting params is assumed to be done beforehand */
static void SetupPlacerOpts(INP t_options Options, INP bool TimingEnabled,
		OUTP struct s_placer_opts *PlacerOpts) {
	PlacerOpts->block_dist = 1; /* DEFAULT */
	if (Options.Count[OT_BLOCK_DIST]) {
		PlacerOpts->block_dist = Options.block_dist;
	}

	PlacerOpts->inner_loop_recompute_divider = 0; /* DEFAULT */
	if (Options.Count[OT_INNER_LOOP_RECOMPUTE_DIVIDER]) {
		PlacerOpts->inner_loop_recompute_divider =
				Options.inner_loop_recompute_divider;
	}

	PlacerOpts->place_cost_exp = 1.; /* DEFAULT */
	if (Options.Count[OT_PLACE_COST_EXP]) {
		PlacerOpts->place_cost_exp = Options.place_cost_exp;
	}

	PlacerOpts->td_place_exp_first = 1.; /* DEFAULT */
	if (Options.Count[OT_TD_PLACE_EXP_FIRST]) {
		PlacerOpts->td_place_exp_first = Options.place_exp_first;
	}

	PlacerOpts->td_place_exp_last = 8.; /* DEFAULT */
	if (Options.Count[OT_TD_PLACE_EXP_LAST]) {
		PlacerOpts->td_place_exp_last = Options.place_exp_last;
	}

	PlacerOpts->place_algorithm = BOUNDING_BOX_PLACE; /* DEFAULT */
	if (TimingEnabled) {
		PlacerOpts->place_algorithm = PATH_TIMING_DRIVEN_PLACE; /* DEFAULT */
	}
	if (Options.Count[OT_PLACE_ALGORITHM]) {
		PlacerOpts->place_algorithm = Options.PlaceAlgorithm;
	}

	PlacerOpts->pad_loc_file = NULL; /* DEFAULT */
	if (Options.Count[OT_FIX_PINS]) {
		if (Options.PinFile) {
			PlacerOpts->pad_loc_file = my_strdup(Options.PinFile);
		}
	}

	PlacerOpts->pad_loc_type = FREE; /* DEFAULT */
	if (Options.Count[OT_FIX_PINS]) {
		PlacerOpts->pad_loc_type = (Options.PinFile ? USER : RANDOM);
	}

	PlacerOpts->place_chan_width = 100; /* DEFAULT */
	if (Options.Count[OT_PLACE_CHAN_WIDTH]) {
		PlacerOpts->place_chan_width = Options.PlaceChanWidth;
	}

	PlacerOpts->recompute_crit_iter = 1; /* DEFAULT */
	if (Options.Count[OT_RECOMPUTE_CRIT_ITER]) {
		PlacerOpts->recompute_crit_iter = Options.RecomputeCritIter;
	}

	PlacerOpts->timing_tradeoff = 0.5; /* DEFAULT */
	if (Options.Count[OT_TIMING_TRADEOFF]) {
		PlacerOpts->timing_tradeoff = Options.PlaceTimingTradeoff;
	}

	/* Depends on PlacerOpts->place_algorithm */
	PlacerOpts->enable_timing_computations = false; /* DEFAULT */
	if ((PlacerOpts->place_algorithm == PATH_TIMING_DRIVEN_PLACE)
			|| (PlacerOpts->place_algorithm == NET_TIMING_DRIVEN_PLACE)) {
		PlacerOpts->enable_timing_computations = true; /* DEFAULT */
	}
	if (Options.Count[OT_ENABLE_TIMING_COMPUTATIONS]) {
		PlacerOpts->enable_timing_computations = Options.ShowPlaceTiming;
	}

	PlacerOpts->place_freq = PLACE_ONCE; /* DEFAULT */
	if ((Options.Count[OT_ROUTE_CHAN_WIDTH])
			|| (Options.Count[OT_PLACE_CHAN_WIDTH])) {
		PlacerOpts->place_freq = PLACE_ONCE;
	}

	PlacerOpts->doPlacement = false; /* DEFAULT */
	if (Options.Count[OT_PLACE]) {
		PlacerOpts->doPlacement = true;
	} else if (!Options.Count[OT_PACK] && !Options.Count[OT_PLACE]
			&& !Options.Count[OT_ROUTE]) {
		if (!Options.Count[OT_TIMING_ANALYZE_ONLY_WITH_NET_DELAY])
			PlacerOpts->doPlacement = true;
	}
	if (PlacerOpts->doPlacement == false) {
		PlacerOpts->place_freq = PLACE_NEVER;
	}

}

static void SetupOperation(INP t_options Options,
		OUTP enum e_operation *Operation) {
	*Operation = RUN_FLOW; /* DEFAULT */
	if (Options.Count[OT_TIMING_ANALYZE_ONLY_WITH_NET_DELAY]) {
		*Operation = TIMING_ANALYSIS_ONLY;
	}
}

static void SetupPowerOpts(t_options Options, t_power_opts *power_opts,
		t_arch * Arch) {

	if (Options.Count[OT_POWER]) {
		power_opts->do_power = true;
	} else {
		power_opts->do_power = false;
	}

	if (power_opts->do_power) {
		if (!Arch->power)
			Arch->power = (t_power_arch*) my_malloc(sizeof(t_power_arch));
		if (!Arch->clocks)
			Arch->clocks = (t_clock_arch*) my_malloc(sizeof(t_clock_arch));
		g_clock_arch = Arch->clocks;
	} else {
		Arch->power = NULL;
		Arch->clocks = NULL;
		g_clock_arch = NULL;
	}

}
