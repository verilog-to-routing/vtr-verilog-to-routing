#include <cstring>
#include <vector>
#include <sstream>
using namespace std;

#include "vtr_assert.h"
#include "vtr_util.h"
#include "vtr_random.h"
#include "vtr_log.h"
#include "vtr_memory.h"
#include "vtr_time.h"

#include "vpr_types.h"
#include "vpr_error.h"
#include "vpr_utils.h"

#include "globals.h"
#include "read_xml_arch_file.h"
#include "SetupVPR.h"
#include "pb_type_graph.h"
#include "pack_types.h"
#include "lb_type_rr_graph.h"
#include "rr_graph_area.h"
#include "echo_arch.h"
#include "read_options.h"
#include "echo_files.h"
#include "clock_modeling.h"

static void SetupNetlistOpts(const t_options& Options, t_netlist_opts& NetlistOpts);
static void SetupPackerOpts(const t_options& Options,
		t_packer_opts *PackerOpts);
static void SetupPlacerOpts(const t_options& Options,
		t_placer_opts *PlacerOpts);
static void SetupAnnealSched(const t_options& Options,
		t_annealing_sched *AnnealSched);
static void SetupRouterOpts(const t_options& Options, t_router_opts *RouterOpts);
static void SetupRoutingArch(const t_arch& Arch, t_det_routing_arch *RoutingArch);
static void SetupTiming(const t_options& Options, const t_arch& Arch,
		const bool TimingEnabled,
		t_timing_inf * Timing);
static void SetupSwitches(const t_arch& Arch,
		t_det_routing_arch *RoutingArch,
		const t_arch_switch_inf *ArchSwitches, int NumArchSwitches);
static void SetupAnalysisOpts(const t_options& Options, t_analysis_opts& analysis_opts);
static void SetupPowerOpts(const t_options& Options, t_power_opts *power_opts,
		t_arch * Arch);
static int find_ipin_cblock_switch_index(const t_arch& Arch);

/* Sets VPR parameters and defaults. Does not do any error checking
 * as this should have been done by the various input checkers */
void SetupVPR(t_options *Options,
              const bool TimingEnabled,
              const bool readArchFile,
              t_file_name_opts *FileNameOpts,
              t_arch * Arch,
              t_model ** user_models,
              t_model ** library_models,
              t_netlist_opts* NetlistOpts,
              t_packer_opts *PackerOpts,
              t_placer_opts *PlacerOpts,
              t_annealing_sched *AnnealSched,
              t_router_opts *RouterOpts,
              t_analysis_opts* AnalysisOpts,
              t_det_routing_arch *RoutingArch,
              vector <t_lb_type_rr_node> **PackerRRGraphs,
              t_segment_inf ** Segments, t_timing_inf * Timing,
              bool * ShowGraphics, int *GraphPause,
              t_power_opts * PowerOpts) {
	int i;
    using argparse::Provenance;

    auto& device_ctx = g_vpr_ctx.mutable_device();

	if (Options->CircuitName.value() == "") {
        vpr_throw(VPR_ERROR_BLIF_F,__FILE__, __LINE__,
                  "No blif file found in arguments (did you specify an architecture file?)\n");
    }


	alloc_and_load_output_file_names(Options->CircuitName);

    //TODO: Move FileNameOpts setup into separate function
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

    FileNameOpts->verify_file_digests = Options->verify_file_digests;

    SetupNetlistOpts(*Options, *NetlistOpts);
	SetupPlacerOpts(*Options, PlacerOpts);
	SetupAnnealSched(*Options, AnnealSched);
	SetupRouterOpts(*Options, RouterOpts);
	SetupAnalysisOpts(*Options, *AnalysisOpts);
	SetupPowerOpts(*Options, PowerOpts, Arch);

	if (readArchFile == true) {
        vtr::ScopedStartFinishTimer t("Loading Architecture Description");
		XmlReadArch(Options->ArchFile.value().c_str(), TimingEnabled, Arch, &device_ctx.block_types,
				&device_ctx.num_block_types);
	}

	*user_models = Arch->models;
	*library_models = Arch->model_library;

	/* TODO: this is inelegant, I should be populating this information in XmlReadArch */
	device_ctx.EMPTY_TYPE = nullptr;
	for (i = 0; i < device_ctx.num_block_types; i++) {
        t_type_ptr type = &device_ctx.block_types[i];
		if (strcmp(device_ctx.block_types[i].name, EMPTY_BLOCK_NAME) == 0) {
            VTR_ASSERT(device_ctx.EMPTY_TYPE == nullptr);
			device_ctx.EMPTY_TYPE = type;
		} else {
            if (block_type_contains_blif_model(type, MODEL_INPUT)) {
                device_ctx.input_types.insert(type);
            }
            if (block_type_contains_blif_model(type, MODEL_OUTPUT)) {
                device_ctx.output_types.insert(type);
            }
        }
    }

	VTR_ASSERT(device_ctx.EMPTY_TYPE != nullptr);

    if (device_ctx.input_types.empty()) {
        VPR_THROW(VPR_ERROR_ARCH,
                "Architecture contains no top-level block type containing '.input' models");
    }

    if (device_ctx.output_types.empty()) {
        VPR_THROW(VPR_ERROR_ARCH,
                "Architecture contains no top-level block type containing '.output' models");
    }

	*Segments = Arch->Segments;
	RoutingArch->num_segment = Arch->num_segments;

	SetupSwitches(*Arch, RoutingArch, Arch->Switches, Arch->num_switches);
	SetupRoutingArch(*Arch, RoutingArch);
	SetupTiming(*Options, *Arch, TimingEnabled, Timing);
	SetupPackerOpts(*Options, PackerOpts);
	RoutingArch->write_rr_graph_filename = Options->write_rr_graph_file;
    RoutingArch->read_rr_graph_filename = Options->read_rr_graph_file;

    //Setup the default flow, if no specific stages specified
    //do all
    if (   !Options->do_packing
        && !Options->do_placement
        && !Options->do_routing
        && !Options->do_analysis) {

        //run all stages if none specified
        PackerOpts->doPacking = STAGE_DO;
        PlacerOpts->doPlacement = STAGE_DO;
        RouterOpts->doRouting = STAGE_DO;
        AnalysisOpts->doAnalysis = STAGE_AUTO; //Deferred until implementation status known
    } else {
        //We run all stages up to the specified stage
        //Note that by checking in reverse order (i.e. analysis to packing)
        //we ensure that earlier stages override the default 'LOAD' action
        //set by later stages

        if(Options->do_analysis) {
            PackerOpts->doPacking = STAGE_LOAD;
            PlacerOpts->doPlacement = STAGE_LOAD;
            RouterOpts->doRouting = STAGE_LOAD;
            AnalysisOpts->doAnalysis = STAGE_DO;
        }

        if(Options->do_routing) {
            PackerOpts->doPacking = STAGE_LOAD;
            PlacerOpts->doPlacement = STAGE_LOAD;
            RouterOpts->doRouting = STAGE_DO;
            AnalysisOpts->doAnalysis = ((Options->do_analysis) ? STAGE_DO : STAGE_AUTO); //Always run analysis after routing
        }

        if(Options->do_placement) {
            PackerOpts->doPacking = STAGE_LOAD;
            PlacerOpts->doPlacement = STAGE_DO;
        }

        if(Options->do_packing) {
            PackerOpts->doPacking = STAGE_DO;
        }
    }

	/* init global variables */
    vtr::out_file_prefix = Options->out_file_prefix;

	/* Set seed for pseudo-random placement, default seed to 1 */
	PlacerOpts->seed =  Options->Seed;
	vtr::srandom(PlacerOpts->seed);

    {
        vtr::ScopedStartFinishTimer t("Building complex block graph");
        alloc_and_load_all_pb_graphs(PowerOpts->do_power);
        *PackerRRGraphs = alloc_and_load_all_lb_type_rr_graph();
    }

    if (Options->clock_modeling == ROUTED_CLOCK) {
        ClockModeling::treat_clock_pins_as_non_globals();
    }

    if (getEchoEnabled() && isEchoFileEnabled(E_ECHO_LB_TYPE_RR_GRAPH)) {
		echo_lb_type_rr_graphs(getEchoFileName(E_ECHO_LB_TYPE_RR_GRAPH),*PackerRRGraphs);
	}

	if (getEchoEnabled() && isEchoFileEnabled(E_ECHO_PB_GRAPH)) {
		echo_pb_graph(getEchoFileName(E_ECHO_PB_GRAPH));
	}

	*GraphPause = Options->GraphPause;

	*ShowGraphics = Options->show_graphics;

	if (getEchoEnabled() && isEchoFileEnabled(E_ECHO_ARCH)) {
		EchoArch(getEchoFileName(E_ECHO_ARCH), device_ctx.block_types, device_ctx.num_block_types,
				Arch);
	}

}

static void SetupTiming(const t_options& Options, const t_arch& Arch,
		const bool TimingEnabled,
		t_timing_inf * Timing) {

	/* Don't do anything if they don't want timing */
	if (false == TimingEnabled) {
		Timing->timing_analysis_enabled = false;
		return;
	}

    int ipin_cblock_switch_index = find_ipin_cblock_switch_index(Arch);

	Timing->C_ipin_cblock = Arch.Switches[ipin_cblock_switch_index].Cin;
	Timing->T_ipin_cblock = Arch.Switches[ipin_cblock_switch_index].Tdel();
	Timing->timing_analysis_enabled = TimingEnabled;
    Timing->SDCFile = Options.SDCFile;
    Timing->slack_definition = Options.SlackDefinition;

#ifdef ENABLE_CLASSIC_VPR_STA
    VTR_ASSERT(Timing->slack_definition == std::string("R") || Timing->slack_definition == std::string("I") ||
           Timing->slack_definition == std::string("S") || Timing->slack_definition == std::string("G") ||
           Timing->slack_definition == std::string("C") || Timing->slack_definition == std::string("N"));
#endif
}

/* This loads up VPR's arch_switch_inf data by combining the switches from
 * the arch file with the special switches that VPR needs. */
static void SetupSwitches(const t_arch& Arch,
		t_det_routing_arch *RoutingArch,
		const t_arch_switch_inf *ArchSwitches, int NumArchSwitches) {

    auto& device_ctx = g_vpr_ctx.mutable_device();

	int switches_to_copy = NumArchSwitches;
	device_ctx.num_arch_switches = NumArchSwitches;

    RoutingArch->wire_to_arch_ipin_switch = find_ipin_cblock_switch_index(Arch);

	/* Depends on device_ctx.num_arch_switches */
	RoutingArch->delayless_switch = device_ctx.num_arch_switches++;

	/* Alloc the list now that we know the final num_arch_switches value */
	device_ctx.arch_switch_inf = new t_arch_switch_inf[device_ctx.num_arch_switches];
	for (int iswitch = 0; iswitch < switches_to_copy; iswitch++){
		device_ctx.arch_switch_inf[iswitch] = ArchSwitches[iswitch];
	}

	/* Delayless switch for connecting sinks and sources with their pins. */
	device_ctx.arch_switch_inf[RoutingArch->delayless_switch].set_type(SwitchType::MUX);
	device_ctx.arch_switch_inf[RoutingArch->delayless_switch].name = vtr::strdup("__vpr_delayless_switch__");
	device_ctx.arch_switch_inf[RoutingArch->delayless_switch].R = 0.;
	device_ctx.arch_switch_inf[RoutingArch->delayless_switch].Cin = 0.;
	device_ctx.arch_switch_inf[RoutingArch->delayless_switch].Cout = 0.;
	device_ctx.arch_switch_inf[RoutingArch->delayless_switch].set_Tdel(t_arch_switch_inf::UNDEFINED_FANIN, 0.);
	device_ctx.arch_switch_inf[RoutingArch->delayless_switch].power_buffer_type = POWER_BUFFER_TYPE_NONE;
	device_ctx.arch_switch_inf[RoutingArch->delayless_switch].mux_trans_size = 0.;
	device_ctx.arch_switch_inf[RoutingArch->delayless_switch].buf_size_type = BufferSize::ABSOLUTE;
	device_ctx.arch_switch_inf[RoutingArch->delayless_switch].buf_size = 0.;
    VTR_ASSERT_MSG(device_ctx.arch_switch_inf[RoutingArch->delayless_switch].buffered(), "Delayless switch expected to be buffered (isolating)");
    VTR_ASSERT_MSG(device_ctx.arch_switch_inf[RoutingArch->delayless_switch].configurable(), "Delayless switch expected to be configurable");

	RoutingArch->global_route_switch = RoutingArch->delayless_switch;

    //Warn about non-zero Cout values for the ipin switch, since these values have no effect.
    //VPR do not model the R/C's of block internal routing connectsion.
    //
    //Note that we don't warn about the R value as it may be used to size the buffer (if buf_size_type is AUTO)
    if (device_ctx.arch_switch_inf[RoutingArch->wire_to_arch_ipin_switch].Cout != 0.) {
        VTR_LOG_WARN( "Non-zero switch output capacitance (%g) has no effect when switch '%s' is used for connection block inputs\n",
                device_ctx.arch_switch_inf[RoutingArch->wire_to_arch_ipin_switch].Cout, Arch.ipin_cblock_switch_name.c_str());
    }

}

/* Sets up routing structures. Since checks are already done, this
 * just copies values across */
static void SetupRoutingArch(const t_arch& Arch,
		t_det_routing_arch *RoutingArch) {

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

static void SetupRouterOpts(const t_options& Options, t_router_opts *RouterOpts) {
	RouterOpts->astar_fac = Options.astar_fac;
	RouterOpts->bb_factor = Options.bb_factor;
	RouterOpts->criticality_exp = Options.criticality_exp;
	RouterOpts->max_criticality = Options.max_criticality;
	RouterOpts->max_router_iterations = Options.max_router_iterations;
	RouterOpts->min_incremental_reroute_fanout = Options.min_incremental_reroute_fanout;
	RouterOpts->incr_reroute_delay_ripup = Options.incr_reroute_delay_ripup;
	RouterOpts->pres_fac_mult = Options.pres_fac_mult;
	RouterOpts->route_type = Options.RouteType;

	RouterOpts->full_stats = Options.full_stats;

    //TODO document these?
	RouterOpts->congestion_analysis = Options.full_stats;
	RouterOpts->fanout_analysis = Options.full_stats;
    RouterOpts->switch_usage_analysis = Options.full_stats;

	RouterOpts->verify_binary_search = Options.verify_binary_search;
	RouterOpts->router_algorithm = Options.RouterAlgorithm;
	RouterOpts->fixed_channel_width = Options.RouteChanWidth;
	RouterOpts->min_channel_width_hint = Options.min_route_chan_width_hint;

    //TODO document these?
	RouterOpts->trim_empty_channels = false; /* DEFAULT */
	RouterOpts->trim_obs_channels = false; /* DEFAULT */

	RouterOpts->initial_pres_fac = Options.initial_pres_fac;
	RouterOpts->base_cost_type = Options.base_cost_type;
	RouterOpts->first_iter_pres_fac = Options.first_iter_pres_fac;
	RouterOpts->acc_fac = Options.acc_fac;
	RouterOpts->bend_cost = Options.bend_cost;
    if (Options.do_routing) {
        RouterOpts->doRouting = STAGE_DO;
    }
	RouterOpts->routing_failure_predictor = Options.routing_failure_predictor;
    RouterOpts->routing_budgets_algorithm = Options.routing_budgets_algorithm;
    RouterOpts->save_routing_per_iteration = Options.save_routing_per_iteration;
    RouterOpts->congested_routing_iteration_threshold_frac = Options.congested_routing_iteration_threshold_frac;
    RouterOpts->route_bb_update = Options.route_bb_update;
    RouterOpts->router_debug_net = Options.router_debug_net;
}

static void SetupAnnealSched(const t_options& Options,
		t_annealing_sched *AnnealSched) {
	AnnealSched->alpha_t = Options.PlaceAlphaT;
	if (AnnealSched->alpha_t >= 1 || AnnealSched->alpha_t <= 0) {
		vpr_throw(VPR_ERROR_OTHER,__FILE__, __LINE__, "alpha_t must be between 0 and 1 exclusive.\n");
	}

	AnnealSched->exit_t = Options.PlaceExitT;
	if (AnnealSched->exit_t <= 0) {
		vpr_throw(VPR_ERROR_OTHER,__FILE__, __LINE__, "exit_t must be greater than 0.\n");
	}

	AnnealSched->init_t = Options.PlaceInitT;
	if (AnnealSched->init_t <= 0) {
		vpr_throw(VPR_ERROR_OTHER,__FILE__, __LINE__, "init_t must be greater than 0.\n");
	}

	if (AnnealSched->init_t < AnnealSched->exit_t) {
		vpr_throw(VPR_ERROR_OTHER,__FILE__, __LINE__, "init_t must be greater or equal to than exit_t.\n");
	}

	AnnealSched->inner_num = Options.PlaceInnerNum;
	if (AnnealSched->inner_num <= 0) {
		vpr_throw(VPR_ERROR_OTHER,__FILE__, __LINE__, "inner_num must be greater than 0.\n");
	}

	AnnealSched->type = Options.anneal_sched_type;
}

/* Sets up the s_packer_opts structure baesd on users inputs and on the architecture specified.
 * Error checking, such as checking for conflicting params is assumed to be done beforehand
 */
void SetupPackerOpts(const t_options& Options,
		t_packer_opts *PackerOpts) {

	PackerOpts->output_file = Options.NetFile;

	PackerOpts->blif_file_name = Options.BlifFile;

    if (Options.do_packing) {
        PackerOpts->doPacking = STAGE_DO;
    }

    //TODO: document?
	PackerOpts->global_clocks = true; /* DEFAULT */
	PackerOpts->hill_climbing_flag = false; /* DEFAULT */

	PackerOpts->allow_unrelated_clustering = Options.allow_unrelated_clustering;
	PackerOpts->connection_driven = Options.connection_driven_clustering;
	PackerOpts->timing_driven = Options.timing_driven_clustering;
	PackerOpts->cluster_seed_type = Options.cluster_seed_type;
	PackerOpts->alpha = Options.alpha_clustering;
	PackerOpts->beta = Options.beta_clustering;
	PackerOpts->pack_verbosity = Options.pack_verbosity;
	PackerOpts->enable_pin_feasibility_filter = Options.enable_clustering_pin_feasibility_filter;
	PackerOpts->target_external_pin_util = Options.target_external_pin_util;
    PackerOpts->target_device_utilization = Options.target_device_utilization;

    //TODO: document?
	PackerOpts->inter_cluster_net_delay = 1.0; /* DEFAULT */
	PackerOpts->auto_compute_inter_cluster_net_delay = true;
	PackerOpts->packer_algorithm = PACK_GREEDY; /* DEFAULT */

    PackerOpts->device_layout = Options.device_layout;

	PackerOpts->hmetis_input_file = Options.hmetis_input_file;
}

static void SetupNetlistOpts(const t_options& Options, t_netlist_opts& NetlistOpts) {

    NetlistOpts.const_gen_inference = Options.const_gen_inference;
    NetlistOpts.absorb_buffer_luts = Options.absorb_buffer_luts;
    NetlistOpts.sweep_dangling_primary_ios = Options.sweep_dangling_primary_ios;
    NetlistOpts.sweep_dangling_nets = Options.sweep_dangling_nets;
    NetlistOpts.sweep_dangling_blocks = Options.sweep_dangling_blocks;
    NetlistOpts.sweep_constant_primary_outputs = Options.sweep_constant_primary_outputs;
    NetlistOpts.netlist_verbosity = Options.netlist_verbosity;
}

/* Sets up the s_placer_opts structure based on users input. Error checking,
 * such as checking for conflicting params is assumed to be done beforehand */
static void SetupPlacerOpts(const t_options& Options, t_placer_opts *PlacerOpts) {

    if (Options.do_placement) {
        PlacerOpts->doPlacement = STAGE_DO;
    }

	PlacerOpts->inner_loop_recompute_divider = Options.inner_loop_recompute_divider;

    //TODO: document?
	PlacerOpts->place_cost_exp = 1;

	PlacerOpts->td_place_exp_first = Options.place_exp_first;

	PlacerOpts->td_place_exp_last = Options.place_exp_last;

	PlacerOpts->place_algorithm = Options.PlaceAlgorithm;

	PlacerOpts->pad_loc_file = Options.pad_loc_file;
	PlacerOpts->pad_loc_type = Options.pad_loc_type;

	PlacerOpts->place_chan_width = Options.PlaceChanWidth;

	PlacerOpts->recompute_crit_iter = Options.RecomputeCritIter;

	PlacerOpts->timing_tradeoff = Options.PlaceTimingTradeoff;

	/* Depends on PlacerOpts->place_algorithm */
	PlacerOpts->enable_timing_computations = Options.ShowPlaceTiming;

    //TODO: document?
	PlacerOpts->place_freq = PLACE_ONCE; /* DEFAULT */
}

static void SetupAnalysisOpts(const t_options& Options, t_analysis_opts& analysis_opts) {
    if (Options.do_analysis) {
        analysis_opts.doAnalysis = STAGE_DO;
    }

    analysis_opts.gen_post_synthesis_netlist = Options.Generate_Post_Synthesis_Netlist;

    analysis_opts.timing_report_npaths = Options.timing_report_npaths;
    analysis_opts.timing_report_detail = Options.timing_report_detail;
    analysis_opts.timing_report_skew = Options.timing_report_skew;
}

static void SetupPowerOpts(const t_options& Options, t_power_opts *power_opts,
		t_arch * Arch) {

    auto& device_ctx = g_vpr_ctx.mutable_device();

    power_opts->do_power = Options.do_power;

	if (power_opts->do_power) {
		if (!Arch->power)
			Arch->power = (t_power_arch*) vtr::malloc(sizeof(t_power_arch));
		if (!Arch->clocks)
			Arch->clocks = (t_clock_arch*) vtr::malloc(sizeof(t_clock_arch));
		device_ctx.clock_arch = Arch->clocks;
	} else {
		Arch->power = nullptr;
		Arch->clocks = nullptr;
		device_ctx.clock_arch = nullptr;
	}
}

static int find_ipin_cblock_switch_index(const t_arch& Arch) {
    int ipin_cblock_switch_index = UNDEFINED;
    for (int i = 0; i < Arch.num_switches; ++i) {
        if (Arch.Switches[i].name == Arch.ipin_cblock_switch_name) {
            if (ipin_cblock_switch_index != UNDEFINED) {
                VPR_THROW(VPR_ERROR_ARCH, "Found duplicate switches named '%s'\n", Arch.ipin_cblock_switch_name.c_str());
            } else {
                ipin_cblock_switch_index = i;
            }
        }
    }

    if (ipin_cblock_switch_index == UNDEFINED) {
        VPR_THROW(VPR_ERROR_ARCH, "Failed to find connection block input pin switch named '%s'\n", Arch.ipin_cblock_switch_name.c_str());
    }
    return ipin_cblock_switch_index;
}

