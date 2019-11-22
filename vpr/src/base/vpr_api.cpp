/**
 * General API for VPR
 * Other software tools should generally call just the functions defined here
 * For advanced/power users, you can call functions defined elsewhere in VPR or modify the data structures directly at your discretion but be aware that doing so can break the correctness of VPR
 *
 * Author: Jason Luu
 * June 21, 2012
 */

#include <cstdio>
#include <cstring>
#include <ctime>
#include <chrono>
#include <cmath>
#include <sstream>

#include "vtr_assert.h"
#include "vtr_math.h"
#include "vtr_log.h"
#include "vtr_version.h"
#include "vtr_time.h"
#include "vtr_path.h"

#include "vpr_types.h"
#include "vpr_utils.h"
#include "globals.h"
#include "atom_netlist.h"
#include "read_netlist.h"
#include "check_netlist.h"
#include "read_blif.h"
#include "draw.h"
#include "place_and_route.h"
#include "pack.h"
#include "place.h"
#include "SetupGrid.h"
#include "setup_clocks.h"
#include "stats.h"
#include "read_options.h"
#include "echo_files.h"
#include "read_xml_arch_file.h"
#include "SetupVPR.h"
#include "ShowSetup.h"
#include "CheckArch.h"
#include "CheckSetup.h"
#include "rr_graph.h"
#include "pb_type_graph.h"
#include "route_common.h"
#include "timing_place_lookup.h"
#include "route_export.h"
#include "vpr_api.h"
#include "read_sdc.h"
#include "power.h"
#include "pack_types.h"
#include "lb_type_rr_graph.h"
#include "read_activity.h"
#include "net_delay.h"
#include "AnalysisDelayCalculator.h"
#include "timing_info.h"
#include "netlist_writer.h"
#include "net_delay.h"
#include "RoutingDelayCalculator.h"
#include "check_route.h"
#include "constant_nets.h"
#include "atom_netlist_utils.h"

#include "pack_report.h"

#include "timing_graph_builder.h"
#include "timing_reports.h"
#include "tatum/echo_writer.hpp"

#include "read_route.h"
#include "read_blif.h"
#include "read_place.h"

#include "arch_util.h"

#include "log.h"
#include "iostream"

#ifdef VPR_USE_TBB
#    include <tbb/task_scheduler_init.h>

//We need to store the scheduler object so any concurrency
//setting is persistent
std::unique_ptr<tbb::task_scheduler_init> tbb_scheduler;
#endif

/* Local subroutines */
static void free_complex_block_types();

static void free_device(const t_det_routing_arch& routing_arch);
static void free_circuit();

static void get_intercluster_switch_fanin_estimates(const t_vpr_setup& vpr_setup,
                                                    const t_arch& arch,
                                                    const int wire_segment_length,
                                                    int* opin_switch_fanin,
                                                    int* wire_switch_fanin,
                                                    int* ipin_switch_fanin);
/* Local subroutines end */

/* Display general VPR information */
void vpr_print_title() {
    VTR_LOG("VPR FPGA Placement and Routing.\n");
    VTR_LOG("Version: %s\n", vtr::VERSION);
    VTR_LOG("Revision: %s\n", vtr::VCS_REVISION);
    VTR_LOG("Compiled: %s\n", vtr::BUILD_TIMESTAMP);
    VTR_LOG("Compiler: %s\n", vtr::COMPILER);
    VTR_LOG("Build Info: %s\n", vtr::BUILD_INFO);
    VTR_LOG("\n");
    VTR_LOG("University of Toronto\n");
    VTR_LOG("verilogtorouting.org\n");
    VTR_LOG("vtr-users@googlegroups.com\n");
    VTR_LOG("This is free open source code under MIT license.\n");
    VTR_LOG("\n");
}

void vpr_print_args(int argc, const char** argv) {
    VTR_LOG("VPR was run with the following command-line:\n");
    for (int i = 0; i < argc; i++) {
        if (i != 0) {
            VTR_LOG(" ");
        }
        VTR_LOG("%s", argv[i]);
    }
    VTR_LOG("\n\n");
}

void vpr_initialize_logging() {
    {
        //Allow the default vpr log file to be overwritten
        const char* env_value = std::getenv("VPR_LOG_FILE");
        if (env_value != nullptr) {
            if (std::strlen(env_value) > 0) {
                vtr::set_log_file(env_value); //Use specified log file
            } else {
                //Empty log file name -> no log file
                vtr::set_log_file(nullptr);
            }
        } else {
            //Unset, use default log name
            vtr::set_log_file("vpr_stdout.log");
        }
    }
}

/* Initialize VPR
 * 1. Read Options
 * 2. Read Arch
 * 3. Read Circuit
 * 4. Sanity check all three
 */
void vpr_init(const int argc, const char** argv, t_options* options, t_vpr_setup* vpr_setup, t_arch* arch) {
    vpr_initialize_logging();

    /* Print title message */
    vpr_print_title();

    /* Read in user options */
    *options = read_options(argc, argv);

    //Print out the arguments passed to VPR.
    //This provides a reference in the log file to exactly
    //how VPR was run, aiding in re-producibility
    vpr_print_args(argc, argv);

    vpr_init_with_options(options, vpr_setup, arch);
}

/* Initialize VPR with options
 * 1. Read Arch
 * 2. Read Circuit
 * 3. Sanity check all three
 */
void vpr_init_with_options(const t_options* options, t_vpr_setup* vpr_setup, t_arch* arch) {
    //Set the number of parallel workers
    // We determine the number of workers in the following order:
    //  1. An explicitly specified command-line argument
    //  2. An environment variable
    //  3. The default value
    size_t num_workers;
    if (options->num_workers.provenance() == argparse::Provenance::SPECIFIED) {
        //Explicit command-line
        num_workers = options->num_workers.value();
    } else {
        const char* env_value = std::getenv("VPR_NUM_WORKERS");
        if (env_value != nullptr) {
            //VPR specific environment variable
            num_workers = vtr::atou(env_value);
        } else {
            //Command-line default value
            VTR_ASSERT(options->num_workers.provenance() == argparse::Provenance::DEFAULT);
            num_workers = options->num_workers.value();
        }
    }

#ifdef VPR_USE_TBB
    //Using Thread Building Blocks
    if (num_workers == 0) {
        //Use default concurrency (i.e. maximum conccurency)
        num_workers = tbb::task_scheduler_init::default_num_threads();
    }

    VTR_LOG("Using up to %zu parallel worker(s)\n", num_workers);
    tbb_scheduler = std::make_unique<tbb::task_scheduler_init>(num_workers);
#else
    //No parallel execution support
    if (num_workers != 1) {
        VTR_LOG_WARN("VPR was compiled without parallel execution support, ignoring the specified number of workers (%zu)",
                     options->num_workers.value());
    }
#endif

    vpr_setup->TimingEnabled = options->timing_analysis;
    vpr_setup->device_layout = options->device_layout;
    vpr_setup->constant_net_method = options->constant_net_method;
    vpr_setup->clock_modeling = options->clock_modeling;
    vpr_setup->exit_before_pack = options->exit_before_pack;

    VTR_LOG("\n");
    VTR_LOG("Architecture file: %s\n", options->ArchFile.value().c_str());
    VTR_LOG("Circuit name: %s\n", options->CircuitName.value().c_str());
    VTR_LOG("\n");

    /* Determine whether echo is on or off */
    setEchoEnabled(options->CreateEchoFile);

    /*
     * Initialize the functions names for which VPR_ERRORs
     * are demoted to VTR_LOG_WARNs
     */
    for (std::string func_name : vtr::split(options->disable_errors, std::string(":"))) {
        map_error_activation_status(func_name);
    }

    /*
     * Initialize the functions names for which
     * warnings are being suppressed
     */
    std::vector<std::string> split_warning_option = vtr::split(options->suppress_warnings, std::string(","));
    std::string warn_log_file;
    std::string warn_functions;
    // If no log file name is provided, the specified warning
    // to suppress are not output anywhere.
    if (split_warning_option.size() == 1) {
        warn_functions = split_warning_option[0];
    } else if (split_warning_option.size() == 2) {
        warn_log_file = split_warning_option[0];
        warn_functions = split_warning_option[1];
    }

    set_noisy_warn_log_file(warn_log_file);
    for (std::string func_name : vtr::split(warn_functions, std::string(":"))) {
        add_warnings_to_suppress(func_name);
    }

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
             vpr_setup->Segments,
             &vpr_setup->Timing,
             &vpr_setup->ShowGraphics,
             &vpr_setup->GraphPause,
             &vpr_setup->SaveGraphics,
             &vpr_setup->PowerOpts);

    /* Check inputs are reasonable */
    CheckArch(*arch);

    /* Verify settings don't conflict or otherwise not make sense */
    CheckSetup(vpr_setup->PackerOpts,
               vpr_setup->PlacerOpts,
               vpr_setup->RouterOpts,
               vpr_setup->RoutingArch, vpr_setup->Segments, vpr_setup->Timing,
               arch->Chans);

    /* flush any messages to user still in stdout that hasn't gotten displayed */
    fflush(stdout);

    /* Read blif file and sweep unused components */
    auto& atom_ctx = g_vpr_ctx.mutable_atom();
    atom_ctx.nlist = read_and_process_circuit(options->circuit_format,
                                              vpr_setup->PackerOpts.blif_file_name.c_str(),
                                              vpr_setup->user_models,
                                              vpr_setup->library_models,
                                              vpr_setup->NetlistOpts.const_gen_inference,
                                              vpr_setup->NetlistOpts.absorb_buffer_luts,
                                              vpr_setup->NetlistOpts.sweep_dangling_primary_ios,
                                              vpr_setup->NetlistOpts.sweep_dangling_nets,
                                              vpr_setup->NetlistOpts.sweep_dangling_blocks,
                                              vpr_setup->NetlistOpts.sweep_constant_primary_outputs,
                                              vpr_setup->NetlistOpts.netlist_verbosity);

    if (vpr_setup->PowerOpts.do_power) {
        //Load the net activity file for power estimation
        vtr::ScopedStartFinishTimer t("Load Activity File");
        auto& power_ctx = g_vpr_ctx.mutable_power();
        power_ctx.atom_net_power = read_activity(atom_ctx.nlist, vpr_setup->FileNameOpts.ActFile.c_str());
    }

    //Initialize timing graph and constraints
    if (vpr_setup->TimingEnabled) {
        auto& timing_ctx = g_vpr_ctx.mutable_timing();
        {
            vtr::ScopedStartFinishTimer t("Build Timing Graph");
            timing_ctx.graph = TimingGraphBuilder(atom_ctx.nlist, atom_ctx.lookup).timing_graph(options->allow_dangling_combinational_nodes);
            VTR_LOG("  Timing Graph Nodes: %zu\n", timing_ctx.graph->nodes().size());
            VTR_LOG("  Timing Graph Edges: %zu\n", timing_ctx.graph->edges().size());
            VTR_LOG("  Timing Graph Levels: %zu\n", timing_ctx.graph->levels().size());
        }
        {
            print_netlist_clock_info(atom_ctx.nlist);
        }
        {
            vtr::ScopedStartFinishTimer t("Load Timing Constraints");
            timing_ctx.constraints = read_sdc(vpr_setup->Timing, atom_ctx.nlist, atom_ctx.lookup, *timing_ctx.graph);
        }
    }

    fflush(stdout);

    ShowSetup(*vpr_setup);
}

bool vpr_flow(t_vpr_setup& vpr_setup, t_arch& arch) {
    if (vpr_setup.exit_before_pack) {
        VTR_LOG_WARN("Exiting before packing as requested.\n");
        return true;
    }

    { //Pack
        bool pack_success = vpr_pack_flow(vpr_setup, arch);

        if (!pack_success) {
            return false; //Unimplementable
        }
    }

    vpr_create_device(vpr_setup, arch);

    vpr_init_graphics(vpr_setup, arch);
    { //Place
        bool place_success = vpr_place_flow(vpr_setup, arch);

        if (!place_success) {
            std::cout << "failed placement" << std::endl;
            return false; //Unimplementable
        }
    }
    RouteStatus route_status;
    { //Route
        route_status = vpr_route_flow(vpr_setup, arch);
    }
    { //Analysis
        vpr_analysis_flow(vpr_setup, arch, route_status);
    }
    vpr_close_graphics(vpr_setup);

    return route_status.success();
}

void vpr_create_device(t_vpr_setup& vpr_setup, const t_arch& arch) {
    vtr::ScopedStartFinishTimer timer("Create Device");
    vpr_create_device_grid(vpr_setup, arch);

    vpr_setup_clock_networks(vpr_setup, arch);

    if (vpr_setup.PlacerOpts.place_chan_width != NO_FIXED_CHANNEL_WIDTH) {
        vpr_create_rr_graph(vpr_setup, arch, vpr_setup.PlacerOpts.place_chan_width);
    }
}

/*
 * Allocs globals: chan_width_x, chan_width_y, device_ctx.grid
 * Depends on num_clbs, pins_per_clb */
void vpr_create_device_grid(const t_vpr_setup& vpr_setup, const t_arch& Arch) {
    vtr::ScopedStartFinishTimer timer("Build Device Grid");
    /* Read in netlist file for placement and routing */
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& device_ctx = g_vpr_ctx.mutable_device();

    device_ctx.arch = &Arch;

    /*
     *Load the device grid
     */

    //Record the resource requirement
    std::map<t_logical_block_type_ptr, size_t> num_type_instances;
    for (auto blk_id : cluster_ctx.clb_nlist.blocks()) {
        num_type_instances[cluster_ctx.clb_nlist.block_type(blk_id)]++;
    }

    //Build the device
    float target_device_utilization = vpr_setup.PackerOpts.target_device_utilization;
    device_ctx.grid = create_device_grid(vpr_setup.device_layout, Arch.grid_layouts, num_type_instances, target_device_utilization);

    /*
     *Report on the device
     */
    size_t num_grid_tiles = count_grid_tiles(device_ctx.grid);
    VTR_LOG("FPGA sized to %zu x %zu: %zu grid tiles (%s)\n", device_ctx.grid.width(), device_ctx.grid.height(), num_grid_tiles, device_ctx.grid.name().c_str());

    VTR_LOG("\n");
    VTR_LOG("Resource usage...\n");
    for (const auto& type : device_ctx.physical_tile_types) {
        VTR_LOG("\tNetlist      %d\tblocks of type: %s\n",
                num_type_instances[logical_block_type(&type)], type.name);
        VTR_LOG("\tArchitecture %d\tblocks of type: %s\n",
                device_ctx.grid.num_instances(&type), type.name);
    }
    VTR_LOG("\n");

    float device_utilization = calculate_device_utilization(device_ctx.grid, num_type_instances);
    VTR_LOG("Device Utilization: %.2f (target %.2f)\n", device_utilization, target_device_utilization);
    for (const auto& type : device_ctx.physical_tile_types) {
        float util = 0.;
        if (device_ctx.grid.num_instances(&type) != 0) {
            util = float(num_type_instances[logical_block_type(&type)]) / device_ctx.grid.num_instances(&type);
        }
        VTR_LOG("\tBlock Utilization: %.2f Type: %s\n", util, type.name);
    }
    VTR_LOG("\n");

    if (!device_ctx.grid.limiting_resources().empty()) {
        std::vector<std::string> limiting_block_names;
        for (auto blk_type : device_ctx.grid.limiting_resources()) {
            limiting_block_names.push_back(blk_type->name);
        }
        VTR_LOG("FPGA size limited by block type(s): %s\n", vtr::join(limiting_block_names, " ").c_str());
        VTR_LOG("\n");
    }
}

void vpr_setup_clock_networks(t_vpr_setup& vpr_setup, const t_arch& Arch) {
    if (vpr_setup.clock_modeling == DEDICATED_NETWORK) {
        setup_clock_networks(Arch, vpr_setup.Segments);
    }
}

bool vpr_pack_flow(t_vpr_setup& vpr_setup, const t_arch& arch) {
    auto& packer_opts = vpr_setup.PackerOpts;

    bool status = true;

    if (packer_opts.doPacking == STAGE_SKIP) {
        //pass
    } else {
        if (packer_opts.doPacking == STAGE_DO) {
            //Do the actual packing
            status = vpr_pack(vpr_setup, arch);

            //TODO: to be consistent with placement/routing vpr_pack should really
            //      load the netlist data structures itself, instead of re-loading
            //      the netlist from the .net file

            //Load the result from the .net file
            vpr_load_packing(vpr_setup, arch);
        } else {
            VTR_ASSERT(packer_opts.doPacking == STAGE_LOAD);
            //Load a previous packing from the .net file
            vpr_load_packing(vpr_setup, arch);
        }

        /* Sanity check the resulting netlist */
        check_netlist(packer_opts.pack_verbosity);

        /* Output the netlist stats to console. */
        printClusteredNetlistStats();
    }

    return status;
}

bool vpr_pack(t_vpr_setup& vpr_setup, const t_arch& arch) {
    vtr::ScopedStartFinishTimer timer("Packing");

    /* If needed, estimate inter-cluster delay. Assume the average routing hop goes out of
     * a block through an opin switch to a length-4 wire, then through a wire switch to another
     * length-4 wire, then through a wire-to-ipin-switch into another block. */
    int wire_segment_length = 4;

    float inter_cluster_delay = UNDEFINED;
    if (vpr_setup.PackerOpts.timing_driven
        && vpr_setup.PackerOpts.auto_compute_inter_cluster_net_delay) {
        /* We want to determine a reasonable fan-in to the opin, wire, and ipin switches, based
         * on which the intercluster delays can be estimated. The fan-in of a switch influences its
         * delay.
         *
         * The fan-in of the switch depends on the architecture (unidirectional/bidirectional), as
         * well as Fc_in/out and Fs */
        int opin_switch_fanin, wire_switch_fanin, ipin_switch_fanin;
        get_intercluster_switch_fanin_estimates(vpr_setup, arch, wire_segment_length, &opin_switch_fanin,
                                                &wire_switch_fanin, &ipin_switch_fanin);

        float Tdel_opin_switch, R_opin_switch, Cout_opin_switch;
        float opin_switch_del = get_arch_switch_info(arch.Segments[0].arch_opin_switch, opin_switch_fanin,
                                                     Tdel_opin_switch, R_opin_switch, Cout_opin_switch);

        float Tdel_wire_switch, R_wire_switch, Cout_wire_switch;
        float wire_switch_del = get_arch_switch_info(arch.Segments[0].arch_wire_switch, wire_switch_fanin,
                                                     Tdel_wire_switch, R_wire_switch, Cout_wire_switch);

        float Tdel_wtoi_switch, R_wtoi_switch, Cout_wtoi_switch;
        float wtoi_switch_del = get_arch_switch_info(vpr_setup.RoutingArch.wire_to_arch_ipin_switch, ipin_switch_fanin,
                                                     Tdel_wtoi_switch, R_wtoi_switch, Cout_wtoi_switch);

        float Rmetal = arch.Segments[0].Rmetal;
        float Cmetal = arch.Segments[0].Cmetal;

        /* The delay of a wire with its driving switch is the switch delay plus the
         * product of the equivalent resistance and capacitance experienced by the wire. */

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

    return try_pack(&vpr_setup.PackerOpts, &vpr_setup.AnalysisOpts,
                    &arch, vpr_setup.user_models,
                    vpr_setup.library_models, inter_cluster_delay,
                    vpr_setup.PackerRRGraph);
}

void vpr_load_packing(t_vpr_setup& vpr_setup, const t_arch& arch) {
    vtr::ScopedStartFinishTimer timer("Load Packing");

    VTR_ASSERT_MSG(!vpr_setup.FileNameOpts.NetFile.empty(),
                   "Must have valid .net filename to load packing");

    auto& cluster_ctx = g_vpr_ctx.mutable_clustering();

    cluster_ctx.clb_nlist = read_netlist(vpr_setup.FileNameOpts.NetFile.c_str(),
                                         &arch,
                                         vpr_setup.FileNameOpts.verify_file_digests,
                                         vpr_setup.PackerOpts.pack_verbosity);

    process_constant_nets(cluster_ctx.clb_nlist, vpr_setup.constant_net_method, vpr_setup.PackerOpts.pack_verbosity);

    {
        std::ofstream ofs("packing_pin_util.rpt");
        report_packing_pin_usage(ofs, g_vpr_ctx);
    }
}

bool vpr_place_flow(t_vpr_setup& vpr_setup, const t_arch& arch) {
    VTR_LOG("\n");
    const auto& placer_opts = vpr_setup.PlacerOpts;
    if (placer_opts.doPlacement == STAGE_SKIP) {
        //pass
    } else {
        if (placer_opts.doPlacement == STAGE_DO) {
            //Do the actual placement
            vpr_place(vpr_setup, arch);

        } else {
            VTR_ASSERT(placer_opts.doPlacement == STAGE_LOAD);

            //Load a previous placement
            vpr_load_placement(vpr_setup, arch);
        }

        sync_grid_to_blocks();
        post_place_sync();
    }

    return true;
}

void vpr_place(t_vpr_setup& vpr_setup, const t_arch& arch) {
    vtr::ScopedStartFinishTimer timer("Placement");

    try_place(vpr_setup.PlacerOpts,
              vpr_setup.AnnealSched,
              vpr_setup.RouterOpts,
              vpr_setup.AnalysisOpts,
              arch.Chans,
              &vpr_setup.RoutingArch,
              vpr_setup.Segments,
              arch.Directs,
              arch.num_directs);

    auto& filename_opts = vpr_setup.FileNameOpts;
    auto& cluster_ctx = g_vpr_ctx.clustering();

    print_place(filename_opts.NetFile.c_str(),
                cluster_ctx.clb_nlist.netlist_id().c_str(),
                filename_opts.PlaceFile.c_str());
}

void vpr_load_placement(t_vpr_setup& vpr_setup, const t_arch& arch) {
    vtr::ScopedStartFinishTimer timer("Load Placement");

    const auto& device_ctx = g_vpr_ctx.device();
    auto& place_ctx = g_vpr_ctx.mutable_placement();
    const auto& filename_opts = vpr_setup.FileNameOpts;

    //Load an existing placement from a file
    read_place(filename_opts.NetFile.c_str(), filename_opts.PlaceFile.c_str(), filename_opts.verify_file_digests, device_ctx.grid);

    //Ensure placement macros are loaded so that they can be drawn after placement (e.g. during routing)
    place_ctx.pl_macros = alloc_and_load_placement_macros(arch.Directs, arch.num_directs);
}

RouteStatus vpr_route_flow(t_vpr_setup& vpr_setup, const t_arch& arch) {
    VTR_LOG("\n");

    RouteStatus route_status;

    const auto& router_opts = vpr_setup.RouterOpts;
    const auto& filename_opts = vpr_setup.FileNameOpts;

    if (router_opts.doRouting == STAGE_SKIP) {
        //Assume successful
        route_status = RouteStatus(true, -1);
    } else { //Do or load
        int chan_width = router_opts.fixed_channel_width;

        //Initialize the delay calculator
        vtr::t_chunk net_delay_ch;
        vtr::vector<ClusterNetId, float*> net_delay = alloc_net_delay(&net_delay_ch);

        std::shared_ptr<SetupHoldTimingInfo> timing_info = nullptr;
        std::shared_ptr<RoutingDelayCalculator> routing_delay_calc = nullptr;
        if (vpr_setup.Timing.timing_analysis_enabled) {
            auto& atom_ctx = g_vpr_ctx.atom();

            routing_delay_calc = std::make_shared<RoutingDelayCalculator>(atom_ctx.nlist, atom_ctx.lookup, net_delay);

            timing_info = make_setup_hold_timing_info(routing_delay_calc);
        }

        if (router_opts.doRouting == STAGE_DO) {
            //Do the actual routing
            if (NO_FIXED_CHANNEL_WIDTH == chan_width) {
                //Find minimum channel width
                route_status = vpr_route_min_W(vpr_setup, arch, timing_info, routing_delay_calc, net_delay);
            } else {
                //Route at specified channel width
                route_status = vpr_route_fixed_W(vpr_setup, arch, chan_width, timing_info, routing_delay_calc, net_delay);
            }

            //Save the routing in the .route file
            print_route(filename_opts.PlaceFile.c_str(), filename_opts.RouteFile.c_str());
        } else {
            VTR_ASSERT(router_opts.doRouting == STAGE_LOAD);

            //Load a previous routing
            route_status = vpr_load_routing(vpr_setup, arch, chan_width, timing_info, net_delay);
        }

        //Post-implementation

        std::string graphics_msg;
        if (route_status.success()) {
            //Sanity check the routing
            if (!router_opts.disable_check_route) {
                check_route(router_opts.route_type);
            }
            get_serial_num();

            //Update status
            VTR_LOG("Circuit successfully routed with a channel width factor of %d.\n", route_status.chan_width());
            graphics_msg = vtr::string_fmt("Routing succeeded with a channel width factor of %d.", route_status.chan_width());
        } else {
            //Update status
            VTR_LOG("Circuit is unroutable with a channel width factor of %d.\n", route_status.chan_width());
            graphics_msg = vtr::string_fmt("Routing failed with a channel width factor of %d. ILLEGAL routing shown.", route_status.chan_width());
        }

        //Echo files
        if (vpr_setup.Timing.timing_analysis_enabled) {
            if (isEchoFileEnabled(E_ECHO_FINAL_ROUTING_TIMING_GRAPH)) {
                auto& timing_ctx = g_vpr_ctx.timing();
                tatum::write_echo(getEchoFileName(E_ECHO_FINAL_ROUTING_TIMING_GRAPH),
                                  *timing_ctx.graph, *timing_ctx.constraints, *routing_delay_calc, timing_info->analyzer());
            }

            if (isEchoFileEnabled(E_ECHO_ROUTING_SINK_DELAYS)) {
                //TODO: implement
            }
        }

        if (router_opts.switch_usage_analysis) {
            print_switch_usage();
        }

        //Update interactive graphics
        update_screen(ScreenUpdatePriority::MAJOR, graphics_msg.c_str(), ROUTING, timing_info);
        free_net_delay(net_delay, &net_delay_ch);
    }

    return route_status;
}

RouteStatus vpr_route_fixed_W(t_vpr_setup& vpr_setup,
                              const t_arch& arch,
                              int fixed_channel_width,
                              std::shared_ptr<SetupHoldTimingInfo> timing_info,
                              std::shared_ptr<RoutingDelayCalculator> delay_calc,
                              vtr::vector<ClusterNetId, float*>& net_delay) {
    if (router_needs_lookahead(vpr_setup.RouterOpts.router_algorithm)) {
        // Prime lookahead cache to avoid adding lookahead computation cost to
        // the routing timer.
        get_cached_router_lookahead(
            vpr_setup.RouterOpts.lookahead_type,
            vpr_setup.RouterOpts.write_router_lookahead,
            vpr_setup.RouterOpts.read_router_lookahead,
            vpr_setup.Segments);
    }

    vtr::ScopedStartFinishTimer timer("Routing");

    if (NO_FIXED_CHANNEL_WIDTH == fixed_channel_width || fixed_channel_width <= 0) {
        VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "Fixed channel width must be specified when routing at fixed channel width (was %d)", fixed_channel_width);
    }

    bool status = try_route(fixed_channel_width,
                            vpr_setup.RouterOpts,
                            vpr_setup.AnalysisOpts,
                            &vpr_setup.RoutingArch,
                            vpr_setup.Segments,
                            net_delay,
                            timing_info,
                            delay_calc,
                            arch.Chans,
                            arch.Directs, arch.num_directs,
                            ScreenUpdatePriority::MAJOR);

    return RouteStatus(status, fixed_channel_width);
}

RouteStatus vpr_route_min_W(t_vpr_setup& vpr_setup,
                            const t_arch& arch,
                            std::shared_ptr<SetupHoldTimingInfo> timing_info,
                            std::shared_ptr<RoutingDelayCalculator> delay_calc,
                            vtr::vector<ClusterNetId, float*>& net_delay) {
    // Note that lookahead cache is not primed here because
    // binary_search_place_and_route will change the channel width, and result
    // in the lookahead cache being recomputed.
    vtr::ScopedStartFinishTimer timer("Routing");

    auto& router_opts = vpr_setup.RouterOpts;
    int min_W = binary_search_place_and_route(vpr_setup.PlacerOpts,
                                              vpr_setup.AnnealSched,
                                              router_opts,
                                              vpr_setup.AnalysisOpts,
                                              vpr_setup.FileNameOpts,
                                              &arch,
                                              router_opts.verify_binary_search,
                                              router_opts.min_channel_width_hint,
                                              &vpr_setup.RoutingArch,
                                              vpr_setup.Segments,
                                              net_delay,
                                              timing_info,
                                              delay_calc);

    bool status = (min_W > 0);
    return RouteStatus(status, min_W);
}

RouteStatus vpr_load_routing(t_vpr_setup& vpr_setup,
                             const t_arch& /*arch*/,
                             int fixed_channel_width,
                             std::shared_ptr<SetupHoldTimingInfo> timing_info,
                             vtr::vector<ClusterNetId, float*>& net_delay) {
    vtr::ScopedStartFinishTimer timer("Load Routing");
    if (NO_FIXED_CHANNEL_WIDTH == fixed_channel_width) {
        VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "Fixed channel width must be specified when loading routing (was %d)", fixed_channel_width);
    }

    auto& filename_opts = vpr_setup.FileNameOpts;

    //Load the routing from a file
    bool is_legal = read_route(filename_opts.RouteFile.c_str(), vpr_setup.RouterOpts, filename_opts.verify_file_digests);

    if (vpr_setup.Timing.timing_analysis_enabled) {
        //Update timing info
        load_net_delay_from_routing(net_delay);

        timing_info->update();
    }
    init_draw_coords(fixed_channel_width);

    return RouteStatus(is_legal, fixed_channel_width);
}

void vpr_create_rr_graph(t_vpr_setup& vpr_setup, const t_arch& arch, int chan_width_fac) {
    auto& device_ctx = g_vpr_ctx.mutable_device();
    auto det_routing_arch = &vpr_setup.RoutingArch;
    auto& router_opts = vpr_setup.RouterOpts;

    t_chan_width chan_width = init_chan(chan_width_fac, arch.Chans);

    t_graph_type graph_type;
    if (router_opts.route_type == GLOBAL) {
        graph_type = GRAPH_GLOBAL;
    } else {
        graph_type = (det_routing_arch->directionality == BI_DIRECTIONAL ? GRAPH_BIDIR : GRAPH_UNIDIR);
    }

    int warnings = 0;

    //Clean-up any previous RR graph
    free_rr_graph();

    //Create the RR graph
    create_rr_graph(graph_type,
                    device_ctx.physical_tile_types,
                    device_ctx.grid,
                    chan_width,
                    device_ctx.num_arch_switches,
                    det_routing_arch,
                    vpr_setup.Segments,
                    router_opts.base_cost_type,
                    router_opts.trim_empty_channels,
                    router_opts.trim_obs_channels,
                    router_opts.clock_modeling,
                    arch.Directs, arch.num_directs,
                    &warnings);
    //Initialize drawing, now that we have an RR graph
    init_draw_coords(chan_width_fac);
}

void vpr_init_graphics(const t_vpr_setup& vpr_setup, const t_arch& arch) {
    /* Startup X graphics */
    init_graphics_state(vpr_setup.ShowGraphics, vpr_setup.GraphPause,
                        vpr_setup.RouterOpts.route_type, vpr_setup.SaveGraphics);
    if (vpr_setup.ShowGraphics || vpr_setup.SaveGraphics)
        alloc_draw_structs(&arch);
}

void vpr_close_graphics(const t_vpr_setup& /*vpr_setup*/) {
    /* Close down X Display */
    free_draw_structs();
}

/* Since the parameters of a switch may change as a function of its fanin,
 * to get an estimation of inter-cluster delays we need a reasonable estimation
 * of the fan-ins of switches that connect clusters together. These switches are
 * 1) opin to wire switch
 * 2) wire to wire switch
 * 3) wire to ipin switch
 * We can estimate the fan-in of these switches based on the Fc_in/Fc_out of
 * a logic block, and the switch block Fs value */
static void get_intercluster_switch_fanin_estimates(const t_vpr_setup& vpr_setup,
                                                    const t_arch& arch,
                                                    const int wire_segment_length,
                                                    int* opin_switch_fanin,
                                                    int* wire_switch_fanin,
                                                    int* ipin_switch_fanin) {
    e_directionality directionality;
    int Fs;
    float Fc_in, Fc_out;
    int W = 100; //W is unknown pre-packing, so *if* we need W here, we will assume a value of 100

    directionality = vpr_setup.RoutingArch.directionality;
    Fs = vpr_setup.RoutingArch.Fs;
    Fc_in = 0, Fc_out = 0;

    //Build a dummy 10x10 device to determine the 'best' block type to use
    auto grid = create_device_grid(vpr_setup.device_layout, arch.grid_layouts, 10, 10);

    auto type = physical_tile_type(find_most_common_block_type(grid));
    /* get Fc_in/out for most common block (e.g. logic blocks) */
    VTR_ASSERT(type->fc_specs.size() > 0);

    //Estimate the maximum Fc_in/Fc_out

    for (const t_fc_specification& fc_spec : type->fc_specs) {
        float Fc = fc_spec.fc_value;

        if (fc_spec.fc_value_type == e_fc_value_type::ABSOLUTE) {
            //Convert to estimated fractional
            Fc /= W;
        }
        VTR_ASSERT_MSG(Fc >= 0 && Fc <= 1., "Fc should be fractional");

        for (int ipin : fc_spec.pins) {
            int iclass = type->pin_class[ipin];
            e_pin_type pin_type = type->class_inf[iclass].type;

            if (pin_type == DRIVER) {
                Fc_out = std::max(Fc, Fc_out);
            } else {
                VTR_ASSERT(pin_type == RECEIVER);
                Fc_in = std::max(Fc, Fc_in);
            }
        }
    }

    /* Estimates of switch fan-in are done as follows:
     * 1) opin to wire switch:
     * 2 CLBs connect to a channel, each with #opins/4 pins. Each pin has Fc_out*W
     * switches, and then we assume the switches are distributed evenly over the W wires.
     * In the unidirectional case, all these switches are then crammed down to W/wire_segment_length wires.
     *
     * Unidirectional: 2 * #opins_per_side * Fc_out * wire_segment_length
     * Bidirectional:  2 * #opins_per_side * Fc_out
     *
     * 2) wire to wire switch
     * A wire segment in a switchblock connects to Fs other wires. Assuming these connections are evenly
     * distributed, each target wire receives Fs connections as well. In the unidirectional case,
     * source wires can only connect to W/wire_segment_length wires.
     *
     * Unidirectional: Fs * wire_segment_length
     * Bidirectional:  Fs
     *
     * 3) wire to ipin switch
     * An input pin of a CLB simply receives Fc_in connections.
     *
     * Unidirectional: Fc_in
     * Bidirectional:  Fc_in
     */

    /* Fan-in to opin/ipin/wire switches depends on whether the architecture is unidirectional/bidirectional */
    (*opin_switch_fanin) = 2 * type->num_drivers / 4 * Fc_out;
    (*wire_switch_fanin) = Fs;
    (*ipin_switch_fanin) = Fc_in;
    if (directionality == UNI_DIRECTIONAL) {
        /* adjustments to opin-to-wire and wire-to-wire switch fan-ins */
        (*opin_switch_fanin) *= wire_segment_length;
        (*wire_switch_fanin) *= wire_segment_length;
    } else if (directionality == BI_DIRECTIONAL) {
        /* no adjustments need to be made here */
    } else {
        VPR_FATAL_ERROR(VPR_ERROR_PACK, "Unrecognized directionality: %d\n", (int)directionality);
    }
}

/* Free architecture data structures */
void free_device(const t_det_routing_arch& routing_arch) {
    auto& device_ctx = g_vpr_ctx.mutable_device();

    device_ctx.chan_width.x_list.clear();
    device_ctx.chan_width.y_list.clear();
    device_ctx.chan_width.max = device_ctx.chan_width.x_max = device_ctx.chan_width.y_max = device_ctx.chan_width.x_min = device_ctx.chan_width.y_min = 0;

    for (int iswitch : {routing_arch.delayless_switch, routing_arch.global_route_switch}) {
        if (device_ctx.arch_switch_inf[iswitch].name) {
            vtr::free(device_ctx.arch_switch_inf[iswitch].name);
            device_ctx.arch_switch_inf[iswitch].name = nullptr;
        }
    }
    delete[] device_ctx.arch_switch_inf;
    device_ctx.arch_switch_inf = nullptr;
    free_complex_block_types();
    free_chunk_memory_trace();
}

static void free_complex_block_types() {
    auto& device_ctx = g_vpr_ctx.mutable_device();

    free_type_descriptors(device_ctx.logical_block_types);
    free_type_descriptors(device_ctx.physical_tile_types);
    free_pb_graph_edges();
}

void free_circuit() {
    //Free new net structures
    auto& cluster_ctx = g_vpr_ctx.mutable_clustering();
    for (auto blk_id : cluster_ctx.clb_nlist.blocks())
        cluster_ctx.clb_nlist.remove_block(blk_id);

    cluster_ctx.clb_nlist = ClusteredNetlist();
}

static void free_atoms() {
    auto& atom_ctx = g_vpr_ctx.mutable_atom();
    atom_ctx.nlist = AtomNetlist();
    atom_ctx.lookup = AtomLookup();
}

static void free_placement() {
    auto& place_ctx = g_vpr_ctx.mutable_placement();
    place_ctx.block_locs.clear();
    place_ctx.grid_blocks.clear();
}

static void free_routing() {
    auto& routing_ctx = g_vpr_ctx.mutable_routing();
    routing_ctx.trace.clear();
    routing_ctx.trace_nodes.clear();
    routing_ctx.net_rr_terminals.clear();
    routing_ctx.rr_blk_source.clear();
    routing_ctx.rr_blk_source.clear();
    routing_ctx.rr_node_route_inf.clear();
    routing_ctx.net_status.clear();
    routing_ctx.route_bb.clear();
}

void vpr_free_vpr_data_structures(t_arch& Arch,
                                  t_vpr_setup& vpr_setup) {
    free_all_lb_type_rr_graph(vpr_setup.PackerRRGraph);
    free_circuit();
    free_arch(&Arch);
    free_device(vpr_setup.RoutingArch);
    free_echo_file_info();
    free_placement();
    free_routing();
    free_atoms();
}

void vpr_free_all(t_arch& Arch,
                  t_vpr_setup& vpr_setup) {
    free_rr_graph();
    if (vpr_setup.RouterOpts.doRouting) {
        free_route_structs();
    }
    free_trace_structs();
    vpr_free_vpr_data_structures(Arch, vpr_setup);
}

/****************************************************************************************************
 * Advanced functions
 *  Used when you need fine-grained control over VPR that the main VPR operations do not enable
 ****************************************************************************************************/

/* Read in user options */
void vpr_read_options(const int argc, const char** argv, t_options* options) {
    *options = read_options(argc, argv);
}

/* Read in arch and circuit */
void vpr_setup_vpr(t_options* Options,
                   const bool TimingEnabled,
                   const bool readArchFile,
                   t_file_name_opts* FileNameOpts,
                   t_arch* Arch,
                   t_model** user_models,
                   t_model** library_models,
                   t_netlist_opts* NetlistOpts,
                   t_packer_opts* PackerOpts,
                   t_placer_opts* PlacerOpts,
                   t_annealing_sched* AnnealSched,
                   t_router_opts* RouterOpts,
                   t_analysis_opts* AnalysisOpts,
                   t_det_routing_arch* RoutingArch,
                   std::vector<t_lb_type_rr_node>** PackerRRGraph,
                   std::vector<t_segment_inf>& Segments,
                   t_timing_inf* Timing,
                   bool* ShowGraphics,
                   int* GraphPause,
                   bool* SaveGraphics,
                   t_power_opts* PowerOpts) {
    SetupVPR(Options,
             TimingEnabled,
             readArchFile,
             FileNameOpts,
             Arch,
             user_models,
             library_models,
             NetlistOpts,
             PackerOpts,
             PlacerOpts,
             AnnealSched,
             RouterOpts,
             AnalysisOpts,
             RoutingArch,
             PackerRRGraph,
             Segments,
             Timing,
             ShowGraphics,
             GraphPause,
             SaveGraphics,
             PowerOpts);
}

void vpr_check_arch(const t_arch& Arch) {
    CheckArch(Arch);
}

/* Verify settings don't conflict or otherwise not make sense */
void vpr_check_setup(const t_packer_opts& PackerOpts,
                     const t_placer_opts& PlacerOpts,
                     const t_router_opts& RouterOpts,
                     const t_det_routing_arch& RoutingArch,
                     const std::vector<t_segment_inf>& Segments,
                     const t_timing_inf& Timing,
                     const t_chan_width_dist& Chans) {
    CheckSetup(PackerOpts, PlacerOpts, RouterOpts, RoutingArch,
               Segments, Timing, Chans);
}

/* Show current setup */
void vpr_show_setup(const t_vpr_setup& vpr_setup) {
    ShowSetup(vpr_setup);
}

bool vpr_analysis_flow(t_vpr_setup& vpr_setup, const t_arch& Arch, const RouteStatus& route_status) {
    auto& analysis_opts = vpr_setup.AnalysisOpts;

    if (analysis_opts.doAnalysis == STAGE_SKIP) return true; //Skipped

    if (analysis_opts.doAnalysis == STAGE_AUTO && !route_status.success()) return false; //Not run

    VTR_ASSERT_MSG(analysis_opts.doAnalysis == STAGE_DO
                       || (analysis_opts.doAnalysis == STAGE_AUTO && route_status.success()),
                   "Analysis should run only if forced, or implementation legal");

    if (!route_status.success()) {
        VTR_LOG("\n");
        VTR_LOG("*****************************************************************************************\n");
        VTR_LOG_WARN("The following analysis results are for an illegal circuit implementation\n");
        VTR_LOG("*****************************************************************************************\n");
    }

    vpr_analysis(vpr_setup, Arch, route_status);

    return true;
}

void vpr_analysis(t_vpr_setup& vpr_setup, const t_arch& Arch, const RouteStatus& route_status) {
    auto& route_ctx = g_vpr_ctx.routing();
    auto& atom_ctx = g_vpr_ctx.atom();

    //Check the first index to see if a pointer exists
    //TODO: Implement a better error check
    if (route_ctx.trace.empty()) {
        VPR_FATAL_ERROR(VPR_ERROR_ANALYSIS, "No routing loaded -- can not perform post-routing analysis");
    }

    vtr::vector<ClusterNetId, float*> net_delay;
    vtr::t_chunk net_delay_ch;
    if (vpr_setup.TimingEnabled) {
        //Load the net delays
        net_delay = alloc_net_delay(&net_delay_ch);
        load_net_delay_from_routing(net_delay);
    }

    routing_stats(vpr_setup.RouterOpts.full_stats, vpr_setup.RouterOpts.route_type,
                  vpr_setup.Segments,
                  vpr_setup.RoutingArch.R_minW_nmos,
                  vpr_setup.RoutingArch.R_minW_pmos,
                  Arch.grid_logic_tile_area,
                  vpr_setup.RoutingArch.directionality,
                  vpr_setup.RoutingArch.wire_to_rr_ipin_switch);

    if (vpr_setup.TimingEnabled) {
        //Do final timing analysis
        auto analysis_delay_calc = std::make_shared<AnalysisDelayCalculator>(atom_ctx.nlist, atom_ctx.lookup, net_delay);
        auto timing_info = make_setup_hold_timing_info(analysis_delay_calc);
        timing_info->update();

        if (isEchoFileEnabled(E_ECHO_ANALYSIS_TIMING_GRAPH)) {
            auto& timing_ctx = g_vpr_ctx.timing();
            tatum::write_echo(getEchoFileName(E_ECHO_ANALYSIS_TIMING_GRAPH),
                              *timing_ctx.graph, *timing_ctx.constraints, *analysis_delay_calc, timing_info->analyzer());
        }

        //Timing stats
        VTR_LOG("\n");
        generate_hold_timing_stats(/*prefix=*/"", *timing_info,
                                   *analysis_delay_calc, vpr_setup.AnalysisOpts);
        generate_setup_timing_stats(/*prefix=*/"", *timing_info,
                                    *analysis_delay_calc, vpr_setup.AnalysisOpts);

        //Write the post-syntesis netlist
        if (vpr_setup.AnalysisOpts.gen_post_synthesis_netlist) {
            netlist_writer(atom_ctx.nlist.netlist_name().c_str(), analysis_delay_calc);
        }

        //Do power analysis
        if (vpr_setup.PowerOpts.do_power) {
            vpr_power_estimation(vpr_setup, Arch, *timing_info, route_status);
        }

        //Clean-up the net delays
        free_net_delay(net_delay, &net_delay_ch);
    }
}

/* This function performs power estimation. It relies on the
 * placement/routing results, as well as the critical path.
 * Power estimation can be performed as part of a full or
 * partial flow. More information on the power estimation functions of
 * VPR can be found here:
 *   http://docs.verilogtorouting.org/en/latest/vtr/power_estimation/
 */
void vpr_power_estimation(const t_vpr_setup& vpr_setup,
                          const t_arch& Arch,
                          const SetupTimingInfo& timing_info,
                          const RouteStatus& route_status) {
    /* Ensure we are only using 1 clock */
    if (timing_info.critical_paths().size() != 1) {
        VPR_FATAL_ERROR(VPR_ERROR_POWER, "Power analysis only supported on single-clock circuits");
    }

    auto& power_ctx = g_vpr_ctx.mutable_power();

    /* Get the critical path of this clock */
    power_ctx.solution_inf.T_crit = timing_info.least_slack_critical_path().delay();
    VTR_ASSERT(power_ctx.solution_inf.T_crit > 0.);

    /* Get the channel width */
    power_ctx.solution_inf.channel_width = route_status.chan_width();
    VTR_ASSERT(power_ctx.solution_inf.channel_width > 0.);

    VTR_LOG("\n\nPower Estimation:\n");
    VTR_LOG("-----------------\n");

    VTR_LOG("Initializing power module\n");

    /* Initialize the power module */
    bool power_error = power_init(vpr_setup.FileNameOpts.PowerFile.c_str(),
                                  vpr_setup.FileNameOpts.CmosTechFile.c_str(), &Arch, &vpr_setup.RoutingArch);
    if (power_error) {
        VTR_LOG_ERROR("Power initialization failed.\n");
    }

    if (!power_error) {
        float power_runtime_s = 0;

        VTR_LOG("Running power estimation\n");

        /* Run power estimation */
        e_power_ret_code power_ret_code = power_total(&power_runtime_s, vpr_setup,
                                                      &Arch, &vpr_setup.RoutingArch);

        /* Check for errors/warnings */
        if (power_ret_code == POWER_RET_CODE_ERRORS) {
            VTR_LOG_ERROR("Power estimation failed. See power output for error details.\n");
        } else if (power_ret_code == POWER_RET_CODE_WARNINGS) {
            VTR_LOG_WARN("Power estimation completed with warnings. See power output for more details.\n");
        } else {
            VTR_ASSERT(power_ret_code == POWER_RET_CODE_SUCCESS);
        }
        VTR_LOG("Power estimation took %g seconds\n", power_runtime_s);
    }

    /* Uninitialize power module */
    if (!power_error) {
        VTR_LOG("Uninitializing power module\n");
        power_error = power_uninit();
        if (power_error) {
            VTR_LOG_ERROR("Power uninitialization failed.\n");
        }
    }

    VTR_LOG("\n");
}

void vpr_print_error(const VprError& vpr_error) {
    /* Determine the type of VPR error, To-do: can use some enum-to-string mechanism */
    char* error_type = nullptr;
    try {
        switch (vpr_error.type()) {
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
            case VPR_ERROR_INTERRUPTED:
                error_type = vtr::strdup("Interrupted");
                break;
            default:
                error_type = vtr::strdup("Unrecognized Error");
                break;
        }
    } catch (const vtr::VtrError& e) {
        error_type = nullptr;
    }

    //We can't pass std::string's through va_args functions,
    //so we need to copy them and pass via c_str()
    std::string msg = vpr_error.what();
    std::string filename = vpr_error.filename();

    VTR_LOG_ERROR("\nType: %s\nFile: %s\nLine: %d\nMessage: %s\n",
                  error_type, filename.c_str(), vpr_error.line(),
                  msg.c_str());
}
