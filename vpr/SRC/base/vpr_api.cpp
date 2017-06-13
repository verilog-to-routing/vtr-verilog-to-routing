/**
 General API for VPR
 Other software tools should generally call just the functions defined here
 For advanced/power users, you can call functions defined elsewhere in VPR or modify the data structures directly at your discretion but be aware that doing so can break the correctness of VPR

 Author: Jason Luu
 June 21, 2012
 */

#include <cstdio>
#include <cstring>
#include <ctime>
#include <chrono>
#include <cmath>
using namespace std;


#include "vtr_assert.h"
#include "vtr_list.h"
#include "vtr_matrix.h"
#include "vtr_math.h"
#include "vtr_log.h"
#include "vtr_version.h"
#include "vtr_time.h"

#include "vpr_types.h"
#include "vpr_utils.h"
#include "globals.h"
#include "atom_netlist.h"
#include "graphics.h"
#include "read_netlist.h"
#include "check_netlist.h"
#include "print_netlist.h"
#include "read_blif.h"
#include "draw.h"
#include "place_and_route.h"
#include "pack.h"
#include "SetupGrid.h"
#include "stats.h"
#include "path_delay.h"
#include "OptionTokens.h"
#include "read_options.h"
#include "ReadOptions.h"
#include "read_xml_arch_file.h"
#include "SetupVPR.h"
#include "ShowSetup.h"
#include "CheckArch.h"
#include "CheckSetup.h"
#include "CheckOptions.h"
#include "rr_graph.h"
#include "pb_type_graph.h"
#include "route_common.h"
#include "timing_place_lookup.h"
#include "route_export.h"
#include "vpr_api.h"
#include "read_sdc.h"
#include "read_sdc2.h"
#include "power.h"
#include "pack_types.h"
#include "lb_type_rr_graph.h"
#include "output_blif.h"
#include "read_activity.h"
#include "net_delay.h"
#include "AnalysisDelayCalculator.h"
#include "timing_info.h"
#include "netlist_writer.h"

#include "timing_graph_builder.h"
#include "timing_reports.h"
#include "tatum/echo_writer.hpp"

#include "log.h"

/* Local subroutines */
static void free_pb_type(t_pb_type *pb_type);
static void free_complex_block_types(void);

static void free_arch(t_arch* Arch);
static void free_options(const t_options *options);
static void free_circuit(void);

static void get_intercluster_switch_fanin_estimates(const t_vpr_setup& vpr_setup, const int wire_segment_length,
			int *opin_switch_fanin, int *wire_switch_fanin, int *ipin_switch_fanin);

/* Local subroutines end */

/* Display general VPR information */
void vpr_print_title(void) {

	vtr::printf_info("\n");
	vtr::printf_info("VPR FPGA Placement and Routing.\n");
	vtr::printf_info("Version: %s\n", vtr::VERSION);
	vtr::printf_info("Revision: %s\n", vtr::VCS_REVISION);
	vtr::printf_info("Compiled: %s\n", vtr::BUILD_TIMESTAMP);
	vtr::printf_info("Compiler: %s\n", vtr::COMPILER);
	vtr::printf_info("University of Toronto\n");
	vtr::printf_info("vtr-users@googlegroups.com\n");
	vtr::printf_info("This is free open source code under MIT license.\n");
	vtr::printf_info("\n");

}

/* Display help screen */
void vpr_print_usage(void) {
    //TODO: generate this directly from options list

	vtr::printf_info("Usage:  vpr fpga_architecture.xml circuit_name [Options ...]\n");
	vtr::printf_info("\n");
	vtr::printf_info("General Options:  [--version] [--disp on | off] [--auto <int>]\n");
	vtr::printf_info("\t[--pack] [--place] [--route] [--analysis]\n");
	vtr::printf_info("\t[--fast] [--full_stats] [--timing_analysis on | off] [--outfile_prefix <string>]\n");
	vtr::printf_info("\t[--blif_file <string>] [--net_file <string>] [--place_file <string>]\n");
	vtr::printf_info("\t[--route_file <string>] [--sdc_file <string>] [--echo_file on | off]\n");
	vtr::printf_info("\t[--write_rr_graph on | off] [--verify_file_digests on | off]\n");
	vtr::printf_info("\n");
	vtr::printf_info("Netlist Options:\n");
	vtr::printf_info("\t[--absorb_buffer_luts on | off]\n");
	vtr::printf_info("\t[--sweep_primary_ios on | off]\n");
	vtr::printf_info("\t[--sweep_nets on | off]\n");
	vtr::printf_info("\t[--sweep_blocks on | off]\n");
	vtr::printf_info("\n");
	vtr::printf_info("Packer Options:\n");
	/* vtr::printf_info("\t[-global_clocks on | off]\n"); */
	/* vtr::printf_info("\t[-hill_climbing on | off]\n"); */
	/* vtr::printf_info("\t[-sweep_hanging_nets_and_inputs on | off]\n"); */
	vtr::printf_info("\t[--timing_driven_clustering on | off]\n");
	vtr::printf_info("\t[--cluster_seed_type blend|timing|max_inputs] [--alpha_clustering <float>] [--beta_clustering <float>]\n");
	/* vtr::printf_info("\t[-recompute_timing_after <int>] [-cluster_block_delay <float>]\n"); */
	vtr::printf_info("\t[--allow_unrelated_clustering on | off]\n");
	/* vtr::printf_info("\t[-allow_early_exit on | off]\n"); */
	/* vtr::printf_info("\t[-intra_cluster_net_delay <float>] \n"); */
	/* vtr::printf_info("\t[-inter_cluster_net_delay <float>] \n"); */
	vtr::printf_info("\t[--connection_driven_clustering on | off] \n");
	vtr::printf_info("\n");
	vtr::printf_info("Placer Options:\n");
	vtr::printf_info("\t[--place_algorithm bounding_box | path_timing_driven]\n");
	vtr::printf_info("\t[--init_t <float>] [--exit_t <float>]\n");
	vtr::printf_info("\t[--alpha_t <float>] [--inner_num <float>] [--seed <int>]\n");
	vtr::printf_info("\t[--place_cost_exp <float>]\n");
	vtr::printf_info("\t[--place_chan_width <int>] \n");
	vtr::printf_info("\t[--fix_pins random | <file.pads>]\n");
	vtr::printf_info("\t[--enable_timing_computations on | off]\n");
	vtr::printf_info("\n");
	vtr::printf_info("Placement Options Valid Only for Timing-Driven Placement:\n");
	vtr::printf_info("\t[--timing_tradeoff <float>]\n");
	vtr::printf_info("\t[--recompute_crit_iter <int>]\n");
	vtr::printf_info("\t[--inner_loop_recompute_divider <int>]\n");
	vtr::printf_info("\t[--td_place_exp_first <float>]\n");
	vtr::printf_info("\t[--td_place_exp_last <float>]\n");
	vtr::printf_info("\n");
	vtr::printf_info("Router Options:  [-max_router_iterations <int>] [-bb_factor <int>]\n");
	vtr::printf_info("\t[--initial_pres_fac <float>] [--pres_fac_mult <float>]\n");
	vtr::printf_info("\t[--acc_fac <float>] [--first_iter_pres_fac <float>]\n");
	vtr::printf_info("\t[--bend_cost <float>] [--route_type global | detailed]\n");
	vtr::printf_info("\t[--min_incremental_reroute_fanout <int>]\n");
	vtr::printf_info("\t[--verify_binary_search] [--route_chan_width <int>] [--route_chan_trim on | off]\n");
	vtr::printf_info("\t[--router_algorithm breadth_first | timing_driven]\n");
	vtr::printf_info("\t[--base_cost_type delay_normalized | demand_only]\n");
	vtr::printf_info("\n");
	vtr::printf_info("Routing options valid only for timing-driven routing:\n");
	vtr::printf_info("\t[--astar_fac <float>] [--max_criticality <float>]\n");
	vtr::printf_info("\t[--criticality_exp <float>]\n");
	vtr::printf_info("\t[--routing_failure_predictor safe | aggressive | off]\n");
	vtr::printf_info("\n");
	vtr::printf_info("VPR Developer Options:\n");
	vtr::printf_info("\t[--gen_netlist_as_blif]\n");
	vtr::printf_info("\n");
	vtr::printf_info("\tSee https://docs.verilogtorouting.org for option descriptions.\n");
	vtr::printf_info("\n");
}

void vpr_print_args(int argc, const char** argv) {
    vtr::printf_info("VPR was run with the following command-line:\n");
    for(int i = 0; i < argc; i++) {
        if(i != 0) {
            vtr::printf_info(" ");
        }
        vtr::printf_info("%s", argv[i]);
    }
    vtr::printf_info("\n\n");
}

/* Initialize VPR
 1. Read Options
 2. Read Arch
 3. Read Circuit
 4. Sanity check all three
 */
void vpr_init(const int argc, const char **argv,
		t_options *options,
		t_vpr_setup *vpr_setup,
		t_arch *arch) {

    vtr::set_log_file("vpr_stdout.log");

    /* Print title message */
    vpr_print_title();

    memset(options, 0, sizeof(t_options));
    memset(vpr_setup, 0, sizeof(t_vpr_setup));
    memset(arch, 0, sizeof(t_arch));

    /* Read in user options */
    *options = read_options(argc, argv);

    if(options->show_version) {
        //Printed title (which includes version info) above, so all done
        return;
    }

    //Print out the arguments passed to VPR.
    //This provides a reference in the log file to exactly
    //how VPR was run, aiding in re-producibility
    vpr_print_args(argc, argv);


    /* Timing option priorities */
    vpr_setup->TimingEnabled = options->timing_analysis;

    /* Verify the rest of the options */
    CheckOptions(*options, vpr_setup->TimingEnabled);

    vtr::printf_info("\n");
    vtr::printf_info("Architecture file: %s\n", options->ArchFile);
    vtr::printf_info("Circuit name: %s.blif\n", options->CircuitName);
    vtr::printf_info("\n");

    /* Determine whether echo is on or off */
    setEchoEnabled(IsEchoEnabled(options));

    /* Read in arch and circuit */
    SetupVPR(options, 
             vpr_setup->TimingEnabled, 
             true, 
             &vpr_setup->FileNameOpts,
             arch, 
             &vpr_setup->user_models,
             &vpr_setup->library_models, 
             &vpr_setup->NetlistOpts,
             &vpr_setup->PackerOpts,
             &vpr_setup->PlacerOpts, 
             &vpr_setup->AnnealSched,
             &vpr_setup->RouterOpts, 
             &vpr_setup->AnalysisOpts, 
             &vpr_setup->RoutingArch,
             &vpr_setup->PackerRRGraph, 
             &vpr_setup->Segments,
             &vpr_setup->Timing, 
             &vpr_setup->ShowGraphics,
             &vpr_setup->GraphPause, 
             &vpr_setup->PowerOpts);

    /* Check inputs are reasonable */
    CheckArch(*arch);

    /* Verify settings don't conflict or otherwise not make sense */
    CheckSetup(
            vpr_setup->PackerOpts,
            vpr_setup->PlacerOpts,
            vpr_setup->RouterOpts,
            vpr_setup->RoutingArch, vpr_setup->Segments, vpr_setup->Timing,
            arch->Chans);

    /* flush any messages to user still in stdout that hasn't gotten displayed */
    fflush(stdout);

    /* Read blif file and sweep unused components */
    auto& atom_ctx = g_vpr_ctx.mutable_atom();
    atom_ctx.nlist = read_and_process_blif(vpr_setup->PackerOpts.blif_file_name,
                                      vpr_setup->user_models, 
                                      vpr_setup->library_models,
                                      vpr_setup->NetlistOpts.absorb_buffer_luts,
                                      vpr_setup->NetlistOpts.sweep_dangling_primary_ios,
                                      vpr_setup->NetlistOpts.sweep_dangling_nets,
                                      vpr_setup->NetlistOpts.sweep_dangling_blocks,
                                      vpr_setup->NetlistOpts.sweep_constant_primary_outputs);


    if(vpr_setup->PowerOpts.do_power) {
        //Load the net activity file for power estimation
        vtr::ScopedPrintTimer t("Load Activity File");
        auto& power_ctx = g_vpr_ctx.mutable_power();
        power_ctx.atom_net_power = read_activity(atom_ctx.nlist, vpr_setup->FileNameOpts.ActFile);
    }

    //Initialize timing graph and constraints
    if(vpr_setup->TimingEnabled) {
        auto& timing_ctx = g_vpr_ctx.mutable_timing();
        {
            vtr::ScopedPrintTimer t("Build Timing Graph");
            timing_ctx.graph = TimingGraphBuilder(atom_ctx.nlist, atom_ctx.lookup).timing_graph();
            vtr::printf("  Timing Graph Nodes: %zu\n", timing_ctx.graph->nodes().size());
            vtr::printf("  Timing Graph Edges: %zu\n", timing_ctx.graph->edges().size());
        }
        {
            vtr::ScopedPrintTimer t("Load Timing Constraints");
            timing_ctx.constraints = read_sdc2(vpr_setup->Timing, atom_ctx.nlist, atom_ctx.lookup, *timing_ctx.graph);
        }
    }

    fflush(stdout);

    ShowSetup(*options, *vpr_setup);
}

/*
 * Sets globals: device_ctx.nx, device_ctx.ny
 * Allocs globals: chan_width_x, chan_width_y, device_ctx.grid
 * Depends on num_clbs, pins_per_clb */
void vpr_init_pre_place_and_route(const t_vpr_setup& vpr_setup, const t_arch& Arch) {

	/* Read in netlist file for placement and routing */
    auto& cluster_ctx = g_vpr_ctx.mutable_clustering();
    auto& device_ctx = g_vpr_ctx.mutable_device();

	if (vpr_setup.FileNameOpts.NetFile) {
		read_netlist(vpr_setup.FileNameOpts.NetFile, &Arch, vpr_setup.FileNameOpts.verify_file_digests, &cluster_ctx.num_blocks, &cluster_ctx.blocks, &cluster_ctx.clbs_nlist);

		/* This is done so that all blocks have subblocks and can be treated the same */
		check_netlist();

		if(vpr_setup.gen_netlist_as_blif) {
			char *name = (char*)vtr::malloc((strlen(vpr_setup.FileNameOpts.CircuitName) + 16) * sizeof(char));
			sprintf(name, "%s.preplace.blif", vpr_setup.FileNameOpts.CircuitName);
			output_blif(&Arch, cluster_ctx.blocks, cluster_ctx.num_blocks, name);
			free(name);
		}
	}

	/* Output the current settings to console. */
	printClusteredNetlistStats();

    int current = std::max(vtr::nint((float)sqrt((float)cluster_ctx.num_blocks)), 1); /* current is the value of the smaller side of the FPGA */
    int low = 1;
    int high = -1;

    int *num_instances_type = (int*) vtr::calloc(device_ctx.num_block_types, sizeof(int));
    int *num_blocks_type = (int*) vtr::calloc(device_ctx.num_block_types, sizeof(int));

    for (int i = 0; i < cluster_ctx.num_blocks; ++i) {
        num_blocks_type[cluster_ctx.blocks[i].type->index]++;
    }

    if (Arch.clb_grid.IsAuto) {

        /* Auto-size FPGA, perform a binary search */
        while (high == -1 || low < high) {

            /* Generate grid */
            if (Arch.clb_grid.Aspect >= 1.0) {
                device_ctx.ny = current;
                device_ctx.nx = vtr::nint(current * Arch.clb_grid.Aspect);
            } else {
                device_ctx.nx = current;
                device_ctx.ny = vtr::nint(current / Arch.clb_grid.Aspect);
            }
            vtr::printf_info("Auto-sizing FPGA at x = %d y = %d\n", device_ctx.nx, device_ctx.ny);
            alloc_and_load_grid(num_instances_type);
            freeGrid();

            /* Test if netlist fits in grid */
            bool fit = true;
            for (int i = 0; i < device_ctx.num_block_types; ++i) {
                if (num_blocks_type[i] > num_instances_type[i]) {
                    fit = false;
                    break;
                }
            }

            /* get next value */
            if (!fit) {
                /* increase size of max */
                if (high == -1) {
                    current = current * 2;
                    if (current > MAX_SHORT) {
                        vtr::printf_error(__FILE__, __LINE__,
                                "FPGA required is too large for current architecture settings.\n");
                        exit(1);
                    }
                } else {
                    if (low == current)
                        current++;
                    low = current;
                    current = low + ((high - low) / 2);
                }
            } else {
                high = current;
                current = low + ((high - low) / 2);
            }
        }

        /* Generate grid */
        if (Arch.clb_grid.Aspect >= 1.0) {
            device_ctx.ny = current;
            device_ctx.nx = vtr::nint(current * Arch.clb_grid.Aspect);
        } else {
            device_ctx.nx = current;
            device_ctx.ny = vtr::nint(current / Arch.clb_grid.Aspect);
        }
        alloc_and_load_grid(num_instances_type);
        vtr::printf_info("FPGA auto-sized to x = %d y = %d\n", device_ctx.nx, device_ctx.ny);
    } else {
        device_ctx.nx = Arch.clb_grid.W;
        device_ctx.ny = Arch.clb_grid.H;
        alloc_and_load_grid(num_instances_type);
    }

    vtr::printf_info("The circuit will be mapped into a %d x %d array of clbs.\n", device_ctx.nx, device_ctx.ny);

    /* Test if netlist fits in grid */
    for (int i = 0; i < device_ctx.num_block_types; ++i) {
        if (num_blocks_type[i] > num_instances_type[i]) {

            vtr::printf_error(__FILE__, __LINE__,
                    "Not enough physical locations for type %s, "
                    "number of blocks is %d but number of locations is %d.\n",
                    device_ctx.block_types[i].name, num_blocks_type[i],
                    num_instances_type[i]);
            exit(1);
        }
    }

    vtr::printf_info("\n");
    vtr::printf_info("Resource usage...\n");
    for (int i = 0; i < device_ctx.num_block_types; ++i) {
        vtr::printf_info("\tNetlist      %d\tblocks of type: %s\n",
                num_blocks_type[i], device_ctx.block_types[i].name);
        vtr::printf_info("\tArchitecture %d\tblocks of type: %s\n",
                num_instances_type[i], device_ctx.block_types[i].name);
    }
    vtr::printf_info("\n");
    device_ctx.chan_width.x_max = device_ctx.chan_width.y_max = 0;
    device_ctx.chan_width.x_min = device_ctx.chan_width.y_min = 0;
    device_ctx.chan_width.x_list = (int *) vtr::malloc((device_ctx.ny + 1) * sizeof(int));
    device_ctx.chan_width.y_list = (int *) vtr::malloc((device_ctx.nx + 1) * sizeof(int));

    vtr::free(num_blocks_type);
    vtr::free(num_instances_type);
}


void vpr_pack(t_vpr_setup& vpr_setup, const t_arch& arch) {
	std::chrono::high_resolution_clock::time_point end, begin;
	begin = std::chrono::high_resolution_clock::now();
	vtr::printf_info("Initialize packing.\n");

	/* If needed, estimate inter-cluster delay. Assume the average routing hop goes out of
	 a block through an opin switch to a length-4 wire, then through a wire switch to another
	 length-4 wire, then through a wire-to-ipin-switch into another block. */
	int wire_segment_length = 4;

	float inter_cluster_delay = UNDEFINED;
	if (vpr_setup.PackerOpts.timing_driven
			&& vpr_setup.PackerOpts.auto_compute_inter_cluster_net_delay) {

		/* We want to determine a reasonable fan-in to the opin, wire, and ipin switches, based
		   on which the intercluster delays can be estimated. The fan-in of a switch influences its
		   delay.

		   The fan-in of the switch depends on the architecture (unidirectional/bidirectional), as
		   well as Fc_in/out and Fs */
		int opin_switch_fanin, wire_switch_fanin, ipin_switch_fanin;
		get_intercluster_switch_fanin_estimates(vpr_setup, wire_segment_length, &opin_switch_fanin,
				&wire_switch_fanin, &ipin_switch_fanin);


		float Tdel_opin_switch, R_opin_switch, Cout_opin_switch;
		float opin_switch_del = get_arch_switch_info(arch.Segments[0].arch_opin_switch, opin_switch_fanin,
				Tdel_opin_switch, R_opin_switch, Cout_opin_switch);

		float Tdel_wire_switch, R_wire_switch, Cout_wire_switch;
		float wire_switch_del = get_arch_switch_info(arch.Segments[0].arch_wire_switch, wire_switch_fanin,
				Tdel_wire_switch, R_wire_switch, Cout_wire_switch);

		float Tdel_wtoi_switch, R_wtoi_switch, Cout_wtoi_switch;
		float wtoi_switch_del = get_arch_switch_info(
				vpr_setup.RoutingArch.wire_to_arch_ipin_switch, ipin_switch_fanin,
				Tdel_wtoi_switch, R_wtoi_switch, Cout_wtoi_switch);

		float Rmetal = arch.Segments[0].Rmetal;
		float Cmetal = arch.Segments[0].Cmetal;

		/* The delay of a wire with its driving switch is the switch delay plus the
		 product of the equivalent resistance and capacitance experienced by the wire. */

		float first_wire_seg_delay = opin_switch_del
				+ (R_opin_switch + Rmetal * (float)wire_segment_length / 2)
						* (Cout_opin_switch + Cmetal * (float)wire_segment_length);
		float second_wire_seg_delay = wire_switch_del
				+ (R_wire_switch + Rmetal * (float)wire_segment_length / 2)
						* (Cout_wire_switch + Cmetal * (float)wire_segment_length);
		inter_cluster_delay = 4
				* (first_wire_seg_delay + second_wire_seg_delay
						+ wtoi_switch_del); /* multiply by 4 to get a more conservative estimate */
	}

	try_pack(&vpr_setup.PackerOpts, &arch, vpr_setup.user_models,
			vpr_setup.library_models, inter_cluster_delay, vpr_setup.PackerRRGraph
#ifdef ENABLE_CLASSIC_VPR_STA
            , vpr_setup.Timing
#endif
            );

	end = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(end - begin);
    vtr::printf_info("Packing took %g seconds\n", time_span.count());
	fflush(stdout);
}

/* Since the parameters of a switch may change as a function of its fanin,
   to get an estimation of inter-cluster delays we need a reasonable estimation
   of the fan-ins of switches that connect clusters together. These switches are
	1) opin to wire switch
	2) wire to wire switch
	3) wire to ipin switch
   We can estimate the fan-in of these switches based on the Fc_in/Fc_out of
   a logic block, and the switch block Fs value */
static void get_intercluster_switch_fanin_estimates(const t_vpr_setup& vpr_setup, const int wire_segment_length,
			int *opin_switch_fanin, int *wire_switch_fanin, int *ipin_switch_fanin){
	e_directionality directionality;
	int Fs;
	float Fc_in, Fc_out;
	int W = 100;	//W is unknown pre-packing, so *if* we need W here, we will assume a value of 100

    auto& device_ctx = g_vpr_ctx.device();

	directionality = vpr_setup.RoutingArch.directionality;
	Fs = vpr_setup.RoutingArch.Fs;
	Fc_in = 0, Fc_out = 0;

	/* get Fc_in/out for device_ctx.FILL_TYPE block (i.e. logic blocks) */
    VTR_ASSERT(device_ctx.FILL_TYPE->fc_specs.size() > 0);

    //Estimate the maximum Fc_in/Fc_out

    for (const t_fc_specification& fc_spec : device_ctx.FILL_TYPE->fc_specs) {
        float Fc = fc_spec.fc_value;

        if (fc_spec.fc_value_type == e_fc_value_type::ABSOLUTE) {
            //Convert to estimated fractional
            Fc /= W;
        }
        VTR_ASSERT_MSG(Fc >= 0 && Fc <= 1., "Fc should be fractional");

        for (int ipin : fc_spec.pins) {
            int iclass = device_ctx.FILL_TYPE->pin_class[ipin];
            e_pin_type pin_type = device_ctx.FILL_TYPE->class_inf[iclass].type;

            if (pin_type == DRIVER) {
                Fc_out = std::max(Fc, Fc_out);
            } else {
                VTR_ASSERT(pin_type == RECEIVER);
                Fc_in = std::max(Fc, Fc_in);
            }
        }
    }

	/* Estimates of switch fan-in are done as follows:
	   1) opin to wire switch:
		2 CLBs connect to a channel, each with #opins/4 pins. Each pin has Fc_out*W
		switches, and then we assume the switches are distributed evenly over the W wires.
		In the unidirectional case, all these switches are then crammed down to W/wire_segment_length wires.

			Unidirectional: 2 * #opins_per_side * Fc_out * wire_segment_length
			Bidirectional:  2 * #opins_per_side * Fc_out

	   2) wire to wire switch
		A wire segment in a switchblock connects to Fs other wires. Assuming these connections are evenly
		distributed, each target wire receives Fs connections as well. In the unidirectional case,
		source wires can only connect to W/wire_segment_length wires.

			Unidirectional: Fs * wire_segment_length
			Bidirectional:  Fs

	   3) wire to ipin switch
		An input pin of a CLB simply receives Fc_in connections.

			Unidirectional: Fc_in
			Bidirectional:  Fc_in
	*/


	/* Fan-in to opin/ipin/wire switches depends on whether the architecture is unidirectional/bidirectional */
	(*opin_switch_fanin) = 2 * device_ctx.FILL_TYPE->num_drivers/4 * Fc_out;
	(*wire_switch_fanin) = Fs;
	(*ipin_switch_fanin) = Fc_in;
	if (directionality == UNI_DIRECTIONAL){
		/* adjustments to opin-to-wire and wire-to-wire switch fan-ins */
		(*opin_switch_fanin) *= wire_segment_length;
		(*wire_switch_fanin) *= wire_segment_length;
	} else if (directionality == BI_DIRECTIONAL){
		/* no adjustments need to be made here */
	} else {
		vpr_throw(VPR_ERROR_PACK, __FILE__, __LINE__, "Unrecognized directionality: %d\n", (int)directionality);
	}
}

bool vpr_place_and_route(t_vpr_setup *vpr_setup, const t_arch& arch) {

	/* Startup X graphics */
	init_graphics_state(vpr_setup->ShowGraphics, vpr_setup->GraphPause,
			vpr_setup->RouterOpts.route_type);
	if (vpr_setup->ShowGraphics) {
		init_graphics("VPR:  Versatile Place and Route for FPGAs", WHITE);
		alloc_draw_structs(&arch);
	}

	/* Do placement and routing */
	bool success = place_and_route(vpr_setup->PlacerOpts,
			vpr_setup->FileNameOpts, 
            &arch,
			vpr_setup->AnnealSched, vpr_setup->RouterOpts, &vpr_setup->RoutingArch,
			vpr_setup->Segments, vpr_setup->Timing);
	fflush(stdout);

	/* Close down X Display */
	if (vpr_setup->ShowGraphics)
		close_graphics();
	free_draw_structs();

	return(success);
}

/* Free architecture data structures */
void free_arch(t_arch* Arch) {
    auto& device_ctx = g_vpr_ctx.mutable_device();

	freeGrid();
	vtr::free(device_ctx.chan_width.x_list);
	vtr::free(device_ctx.chan_width.y_list);

	device_ctx.chan_width.x_list = device_ctx.chan_width.y_list = NULL;
	device_ctx.chan_width.max = device_ctx.chan_width.x_max = device_ctx.chan_width.y_max = device_ctx.chan_width.x_min = device_ctx.chan_width.y_min = 0;

	for (int i = 0; i < Arch->num_switches; ++i) {
		if (Arch->Switches->name != NULL) {
			vtr::free(Arch->Switches[i].name);
		}
	}
	delete[] Arch->Switches;
	delete[] device_ctx.arch_switch_inf;
	Arch->Switches = NULL;
	device_ctx.arch_switch_inf = NULL;
	for (int i = 0; i < Arch->num_segments; ++i) {
		vtr::free(Arch->Segments[i].cb);
		Arch->Segments[i].cb = NULL;
		vtr::free(Arch->Segments[i].sb);
		Arch->Segments[i].sb = NULL;
		vtr::free(Arch->Segments[i].name);
		Arch->Segments[i].name = NULL;
	}
	vtr::free(Arch->Segments);
	t_model *model = Arch->models;
	while (model) {
		t_model_ports *input_port = model->inputs;
		while (input_port) {
			t_model_ports *prev_port = input_port;
			input_port = input_port->next;
			vtr::free(prev_port->name);
			delete prev_port;
		}
		t_model_ports *output_port = model->outputs;
		while (output_port) {
			t_model_ports *prev_port = output_port;
			output_port = output_port->next;
			vtr::free(prev_port->name);
			delete prev_port;
		}
		vtr::t_linked_vptr *vptr = model->pb_types;
		while (vptr) {
			vtr::t_linked_vptr *vptr_prev = vptr;
			vptr = vptr->next;
			vtr::free(vptr_prev);
		}
		t_model *prev_model = model;

		model = model->next;
		if (prev_model->instances)
			vtr::free(prev_model->instances);
		vtr::free(prev_model->name);
		delete prev_model;
	}

	for (int i = 0; i < 4; ++i) {
		vtr::t_linked_vptr *vptr = Arch->model_library[i].pb_types;
		while (vptr) {
			vtr::t_linked_vptr *vptr_prev = vptr;
			vptr = vptr->next;
			vtr::free(vptr_prev);
		}
	}

	for (int i = 0; i < Arch->num_directs; ++i) {
		vtr::free(Arch->Directs[i].name);
		vtr::free(Arch->Directs[i].from_pin);
		vtr::free(Arch->Directs[i].to_pin);
	}
	vtr::free(Arch->Directs);

    vtr::free(Arch->architecture_id);

	vtr::free(Arch->model_library[0].name);
	vtr::free(Arch->model_library[0].outputs->name);
	delete[] Arch->model_library[0].outputs;
	vtr::free(Arch->model_library[1].inputs->name);
	delete[] Arch->model_library[1].inputs;
	vtr::free(Arch->model_library[1].name);
	vtr::free(Arch->model_library[2].name);
	vtr::free(Arch->model_library[2].inputs[0].name);
	vtr::free(Arch->model_library[2].inputs[1].name);
	delete[] Arch->model_library[2].inputs;
	vtr::free(Arch->model_library[2].outputs->name);
	delete[] Arch->model_library[2].outputs;
	vtr::free(Arch->model_library[3].name);
	vtr::free(Arch->model_library[3].inputs->name);
	delete[] Arch->model_library[3].inputs;
	vtr::free(Arch->model_library[3].outputs->name);
	delete[] Arch->model_library[3].outputs;
	delete[] Arch->model_library;

	if (Arch->clocks) {
		vtr::free(Arch->clocks->clock_inf);
	}

	free_complex_block_types();
	free_chunk_memory_trace();

	vtr::free(Arch->ipin_cblock_switch_name);
}

void free_options(const t_options *options) {

	vtr::free(options->ArchFile);
	vtr::free(options->CircuitName);
	if (options->ActFile)
		vtr::free(options->ActFile);
	if (options->BlifFile)
		vtr::free(options->BlifFile);
	if (options->NetFile)
		vtr::free(options->NetFile);
	if (options->PlaceFile)
		vtr::free(options->PlaceFile);
	if (options->PowerFile)
		vtr::free(options->PowerFile);
	if (options->CmosTechFile)
		vtr::free(options->CmosTechFile);
	if (options->RouteFile)
		vtr::free(options->RouteFile);
	if (options->out_file_prefix)
		vtr::free(options->out_file_prefix);
	if (options->pad_loc_file)
		vtr::free(options->pad_loc_file);
}

static void free_complex_block_types(void) {

	free_all_pb_graph_nodes();

    auto& device_ctx = g_vpr_ctx.mutable_device();

	for (int i = 0; i < device_ctx.num_block_types; ++i) {
		vtr::free(device_ctx.block_types[i].name);

		if (&device_ctx.block_types[i] == device_ctx.EMPTY_TYPE) {
			continue;
		}

		for (int width = 0; width < device_ctx.block_types[i].width; ++width) {
			for (int height = 0; height < device_ctx.block_types[i].height; ++height) {
				for (int side = 0; side < 4; ++side) {
					for (int pin = 0; pin < device_ctx.block_types[i].num_pin_loc_assignments[width][height][side]; ++pin) {
						if (device_ctx.block_types[i].pin_loc_assignments[width][height][side][pin])
							vtr::free(device_ctx.block_types[i].pin_loc_assignments[width][height][side][pin]);
					}
					vtr::free(device_ctx.block_types[i].pinloc[width][height][side]);
					vtr::free(device_ctx.block_types[i].pin_loc_assignments[width][height][side]);
				}
				vtr::free(device_ctx.block_types[i].pinloc[width][height]);
				vtr::free(device_ctx.block_types[i].pin_loc_assignments[width][height]);
				vtr::free(device_ctx.block_types[i].num_pin_loc_assignments[width][height]);
			}
			vtr::free(device_ctx.block_types[i].pinloc[width]);
			vtr::free(device_ctx.block_types[i].pin_loc_assignments[width]);
			vtr::free(device_ctx.block_types[i].num_pin_loc_assignments[width]);
		}
		vtr::free(device_ctx.block_types[i].pinloc);
		vtr::free(device_ctx.block_types[i].pin_loc_assignments);
		vtr::free(device_ctx.block_types[i].num_pin_loc_assignments);

		vtr::free(device_ctx.block_types[i].pin_width);
		vtr::free(device_ctx.block_types[i].pin_height);

		for (int j = 0; j < device_ctx.block_types[i].num_class; ++j) {
			vtr::free(device_ctx.block_types[i].class_inf[j].pinlist);
		}
		vtr::free(device_ctx.block_types[i].class_inf);
		vtr::free(device_ctx.block_types[i].is_global_pin);
		vtr::free(device_ctx.block_types[i].pin_class);
		vtr::free(device_ctx.block_types[i].grid_loc_def);

		free_pb_type(device_ctx.block_types[i].pb_type);
		vtr::free(device_ctx.block_types[i].pb_type);
	}
	delete[] device_ctx.block_types;
}

static void free_pb_type(t_pb_type *pb_type) {

	vtr::free(pb_type->name);
	if (pb_type->blif_model)
		vtr::free(pb_type->blif_model);

	for (int i = 0; i < pb_type->num_modes; ++i) {
		for (int j = 0; j < pb_type->modes[i].num_pb_type_children; ++j) {
			free_pb_type(&pb_type->modes[i].pb_type_children[j]);
		}
		vtr::free(pb_type->modes[i].pb_type_children);
		vtr::free(pb_type->modes[i].name);
		for (int j = 0; j < pb_type->modes[i].num_interconnect; ++j) {
			vtr::free(pb_type->modes[i].interconnect[j].input_string);
			vtr::free(pb_type->modes[i].interconnect[j].output_string);
			vtr::free(pb_type->modes[i].interconnect[j].name);

			for (int k = 0; k < pb_type->modes[i].interconnect[j].num_annotations; ++k) {
				if (pb_type->modes[i].interconnect[j].annotations[k].clock)
					vtr::free(pb_type->modes[i].interconnect[j].annotations[k].clock);
				if (pb_type->modes[i].interconnect[j].annotations[k].input_pins) {
					vtr::free(pb_type->modes[i].interconnect[j].annotations[k].input_pins);
				}
				if (pb_type->modes[i].interconnect[j].annotations[k].output_pins) {
					vtr::free(pb_type->modes[i].interconnect[j].annotations[k].output_pins);
				}
				for (int m = 0; m < pb_type->modes[i].interconnect[j].annotations[k].num_value_prop_pairs; ++m) {
					vtr::free(pb_type->modes[i].interconnect[j].annotations[k].value[m]);
				}
				vtr::free(pb_type->modes[i].interconnect[j].annotations[k].prop);
				vtr::free(pb_type->modes[i].interconnect[j].annotations[k].value);
			}
			vtr::free(pb_type->modes[i].interconnect[j].annotations);
			if (pb_type->modes[i].interconnect[j].interconnect_power)
				vtr::free(pb_type->modes[i].interconnect[j].interconnect_power);
		}
		if (pb_type->modes[i].interconnect)
			vtr::free(pb_type->modes[i].interconnect);
		if (pb_type->modes[i].mode_power)
			vtr::free(pb_type->modes[i].mode_power);
	}
	if (pb_type->modes)
		vtr::free(pb_type->modes);

	for (int i = 0; i < pb_type->num_annotations; ++i) {
		for (int j = 0; j < pb_type->annotations[i].num_value_prop_pairs; ++j) {
			vtr::free(pb_type->annotations[i].value[j]);
		}
		vtr::free(pb_type->annotations[i].value);
		vtr::free(pb_type->annotations[i].prop);
		if (pb_type->annotations[i].input_pins) {
			vtr::free(pb_type->annotations[i].input_pins);
		}
		if (pb_type->annotations[i].output_pins) {
			vtr::free(pb_type->annotations[i].output_pins);
		}
		if (pb_type->annotations[i].clock) {
			vtr::free(pb_type->annotations[i].clock);
		}
	}
	if (pb_type->num_annotations > 0) {
		vtr::free(pb_type->annotations);
	}

	if (pb_type->pb_type_power) {
		vtr::free(pb_type->pb_type_power);
	}

	for (int i = 0; i < pb_type->num_ports; ++i) {
		vtr::free(pb_type->ports[i].name);
		if (pb_type->ports[i].port_class) {
			vtr::free(pb_type->ports[i].port_class);
		}
		if (pb_type->ports[i].port_power) {
			vtr::free(pb_type->ports[i].port_power);
		}
	}
	vtr::free(pb_type->ports);
}

void free_circuit() {

	//Free new net structures
    auto& cluster_ctx = g_vpr_ctx.mutable_clustering();

	free_global_nlist_net(&cluster_ctx.clbs_nlist);

	if (cluster_ctx.blocks != NULL) {
		for (int i = 0; i < cluster_ctx.num_blocks; ++i) {
			if (cluster_ctx.blocks[i].pb != NULL) {
				free_pb(cluster_ctx.blocks[i].pb);
				delete cluster_ctx.blocks[i].pb;
			}
			vtr::free(cluster_ctx.blocks[i].nets);
			vtr::free(cluster_ctx.blocks[i].net_pins);
			vtr::free(cluster_ctx.blocks[i].name);
			delete [] cluster_ctx.blocks[i].pb_route;
		}
	}
	vtr::free(cluster_ctx.blocks);
	cluster_ctx.blocks = NULL;
}

void vpr_free_vpr_data_structures(t_arch& Arch,
		const t_options& options,
		t_vpr_setup& vpr_setup) {

	if (vpr_setup.Timing.SDCFile != NULL) {
		vtr::free(vpr_setup.Timing.SDCFile);
		vpr_setup.Timing.SDCFile = NULL;
	}

	free_all_lb_type_rr_graph(vpr_setup.PackerRRGraph);
	free_options(&options);
	free_circuit();
	free_arch(&Arch);
    free_echo_file_info();
	free_timing_stats();
	free_sdc_related_structs();
}

void vpr_free_all(t_arch& Arch,
		const t_options& options,
		t_vpr_setup& vpr_setup) {

	free_rr_graph();
	if (vpr_setup.RouterOpts.doRouting) {
		free_route_structs();
	}
	free_trace_structs();
	vpr_free_vpr_data_structures(Arch, options, vpr_setup);
}


/****************************************************************************************************
 * Advanced functions
 *  Used when you need fine-grained control over VPR that the main VPR operations do not enable
 ****************************************************************************************************/
/* Read in user options */
void vpr_read_options(const int argc, const char **argv, t_options * options) {
    *options = read_options(argc, argv);
}

/* Read in arch and circuit */
void vpr_setup_vpr(t_options *Options, const bool TimingEnabled,
		const bool readArchFile, t_file_name_opts *FileNameOpts,
		t_arch * Arch,
		t_model ** user_models, t_model ** library_models,
		t_netlist_opts* NetlistOpts,
		t_packer_opts *PackerOpts,
		t_placer_opts *PlacerOpts,
		t_annealing_sched *AnnealSched,
		t_router_opts *RouterOpts,
		t_analysis_opts* AnalysisOpts,
		t_det_routing_arch *RoutingArch,
		vector <t_lb_type_rr_node> **PackerRRGraph,
		t_segment_inf ** Segments, t_timing_inf * Timing,
		bool * ShowGraphics, int *GraphPause,
		t_power_opts * PowerOpts) {
	SetupVPR(Options, TimingEnabled, readArchFile, FileNameOpts, Arch,
			user_models, library_models, NetlistOpts, PackerOpts, PlacerOpts,
			AnnealSched, RouterOpts, AnalysisOpts, RoutingArch, PackerRRGraph, Segments, Timing,
			ShowGraphics, GraphPause, PowerOpts);
}
/* Check inputs are reasonable */
void vpr_check_options(const t_options& Options, const bool TimingEnabled) {
	CheckOptions(Options, TimingEnabled);
}
void vpr_check_arch(const t_arch& Arch) {
	CheckArch(Arch);
}
/* Verify settings don't conflict or otherwise not make sense */
void vpr_check_setup(
        const t_packer_opts PackerOpts,
        const t_placer_opts PlacerOpts,
		const t_router_opts RouterOpts,
		const t_det_routing_arch RoutingArch, const t_segment_inf * Segments,
		const t_timing_inf Timing, const t_chan_width_dist Chans) {
	CheckSetup(PackerOpts, PlacerOpts, RouterOpts, RoutingArch,
			Segments, Timing, Chans);
}

/* Show current setup */
void vpr_show_setup(const t_options& options, const t_vpr_setup& vpr_setup) {
	ShowSetup(options, vpr_setup);
}

void vpr_analysis(const t_vpr_setup& vpr_setup, const t_arch& Arch) {
    if(!vpr_setup.AnalysisOpts.doAnalysis) return;

    auto& route_ctx = g_vpr_ctx.routing();
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& device_ctx = g_vpr_ctx.device();
    auto& atom_ctx = g_vpr_ctx.atom();

    if(route_ctx.trace_head == nullptr) {
        VPR_THROW(VPR_ERROR_ANALYSIS, "No routing loaded -- can not perform post-routing analysis");
    }

    float** net_delay = nullptr;
    vtr::t_chunk net_delay_ch = {NULL, 0, NULL};
#ifdef ENABLE_CLASSIC_VPR_STA
    t_slack* slacks = nullptr;
#endif
	if (vpr_setup.TimingEnabled) {
        //Load the net delays
        net_delay = alloc_net_delay(&net_delay_ch, cluster_ctx.clbs_nlist.net, cluster_ctx.clbs_nlist.net.size());
        load_net_delay_from_routing(net_delay, cluster_ctx.clbs_nlist.net, cluster_ctx.clbs_nlist.net.size());

#ifdef ENABLE_CLASSIC_VPR_STA
        slacks = alloc_and_load_timing_graph(vpr_setup.Timing);
#endif
    }


	routing_stats(vpr_setup.RouterOpts.full_stats, vpr_setup.RouterOpts.route_type,
			device_ctx.num_rr_switches, vpr_setup.Segments,
			vpr_setup.RoutingArch.num_segment, 
            vpr_setup.RoutingArch.R_minW_nmos,
			vpr_setup.RoutingArch.R_minW_pmos, 
            Arch.grid_logic_tile_area,
            vpr_setup.RoutingArch.directionality,
			vpr_setup.RoutingArch.wire_to_rr_ipin_switch,
			vpr_setup.TimingEnabled, net_delay
#ifdef ENABLE_CLASSIC_VPR_STA
            , slacks, vpr_setup.Timing
#endif
            );

	if (vpr_setup.TimingEnabled) {

        //Do final timing analysis
        auto analysis_delay_calc = std::make_shared<AnalysisDelayCalculator>(atom_ctx.nlist, atom_ctx.lookup, net_delay);
        auto timing_info = make_setup_timing_info(analysis_delay_calc);
        timing_info->update();

        if(isEchoFileEnabled(E_ECHO_ANALYSIS_TIMING_GRAPH)) {
            auto& timing_ctx = g_vpr_ctx.timing();
            tatum::write_echo(getEchoFileName(E_ECHO_ANALYSIS_TIMING_GRAPH),
                    *timing_ctx.graph, *timing_ctx.constraints, *analysis_delay_calc, timing_info->analyzer());
        }

        //Timing stats
        generate_timing_stats(*timing_info);

        //Write the post-syntesis netlist
        if(vpr_setup.AnalysisOpts.gen_post_synthesis_netlist) {
            netlist_writer(atom_ctx.nlist.netlist_name().c_str(), analysis_delay_calc);
        }
        
        //Do power analysis
        if(vpr_setup.PowerOpts.do_power) {
            vpr_power_estimation(vpr_setup, Arch, *timing_info);
        }

        //Clean-up the net delays
        free_net_delay(net_delay, &net_delay_ch);

#ifdef ENABLE_CLASSIC_VPR_STA
        free_timing_graph(slacks);
#endif
    }
}

/* This function performs power estimation, and must be called
 * after packing, placement AND routing. Currently, this
 * will not work when running a partial flow (ex. only routing).
 */
void vpr_power_estimation(const t_vpr_setup& vpr_setup, const t_arch& Arch, const SetupTimingInfo& timing_info) {

	/* Ensure we are only using 1 clock */
	if(timing_info.critical_paths().size() != 1) {
        VPR_THROW(VPR_ERROR_POWER, "Power analysis only supported on single-clock circuits");
    }

    auto& power_ctx = g_vpr_ctx.mutable_power();

	/* Get the critical path of this clock */
	power_ctx.solution_inf.T_crit = timing_info.least_slack_critical_path().delay();
	VTR_ASSERT(power_ctx.solution_inf.T_crit > 0.);

	vtr::printf_info("\n\nPower Estimation:\n");
	vtr::printf_info("-----------------\n");

	vtr::printf_info("Initializing power module\n");

	/* Initialize the power module */
	bool power_error = power_init(vpr_setup.FileNameOpts.PowerFile,
			vpr_setup.FileNameOpts.CmosTechFile, &Arch, &vpr_setup.RoutingArch);
	if (power_error) {
		vtr::printf_error(__FILE__, __LINE__,
				"Power initialization failed.\n");
	}

	if (!power_error) {
		float power_runtime_s;

		vtr::printf_info("Running power estimation\n");

		/* Run power estimation */
		e_power_ret_code power_ret_code = power_total(&power_runtime_s, vpr_setup,
				&Arch, &vpr_setup.RoutingArch);

		/* Check for errors/warnings */
		if (power_ret_code == POWER_RET_CODE_ERRORS) {
			vtr::printf_error(__FILE__, __LINE__,
					"Power estimation failed. See power output for error details.\n");
		} else if (power_ret_code == POWER_RET_CODE_WARNINGS) {
			vtr::printf_warning(__FILE__, __LINE__,
					"Power estimation completed with warnings. See power output for more details.\n");
		} else if (power_ret_code == POWER_RET_CODE_SUCCESS) {
		}
		vtr::printf_info("Power estimation took %g seconds\n", power_runtime_s);
	}

	/* Uninitialize power module */
	if (!power_error) {
		vtr::printf_info("Uninitializing power module\n");
		power_error = power_uninit();
		if (power_error) {
			vtr::printf_error(__FILE__, __LINE__,
					"Power uninitialization failed.\n");
		} else {

		}
	}
	vtr::printf_info("\n");
}

void vpr_print_error(const VprError& vpr_error){

	/* Determine the type of VPR error, To-do: can use some enum-to-string mechanism */
    char* error_type = NULL;
    try {
        switch(vpr_error.type()){
        case VPR_ERROR_UNKNOWN:
            error_type = vtr::strdup("Unknown");
            break;
        case VPR_ERROR_ARCH:
            error_type = vtr::strdup("Architecture file");
            break;
        case VPR_ERROR_PACK:
            error_type = vtr::strdup("Packing");
            break;
        case VPR_ERROR_PLACE:
            error_type = vtr::strdup("Placement");
            break;
        case VPR_ERROR_ROUTE:
            error_type = vtr::strdup("Routing");
            break;
        case VPR_ERROR_TIMING:
            error_type = vtr::strdup("Timing");
            break;
        case VPR_ERROR_SDC:
            error_type = vtr::strdup("SDC file");
            break;
        case VPR_ERROR_NET_F:
            error_type = vtr::strdup("Netlist file");
            break;
        case VPR_ERROR_BLIF_F:
            error_type = vtr::strdup("Blif file");
            break;
        case VPR_ERROR_PLACE_F:
            error_type = vtr::strdup("Placement file");
            break;
        case VPR_ERROR_IMPL_NETLIST_WRITER:
            error_type = vtr::strdup("Implementation Netlist Writer");
            break;
        case VPR_ERROR_ATOM_NETLIST:
            error_type = vtr::strdup("Atom Netlist");
            break;
        case VPR_ERROR_POWER:
            error_type = vtr::strdup("Power");
            break;
        case VPR_ERROR_ANALYSIS:
            error_type = vtr::strdup("Analysis");
            break;
        case VPR_ERROR_OTHER:
            error_type = vtr::strdup("Other");
            break;
        default:
            error_type = vtr::strdup("Unrecognized Error");
            break;
        }
    } catch(const vtr::VtrError& e) {
        error_type = NULL;
    }

    //We can't pass std::string's through va_args functions,
    //so we need to copy them and pass via c_str()
    std::string msg = vpr_error.what();
    std::string filename = vpr_error.filename();

	vtr::printf_error(__FILE__, __LINE__,
		"\nType: %s\nFile: %s\nLine: %d\nMessage: %s\n",
		error_type, filename.c_str(), vpr_error.line(),
		msg.c_str());
}
