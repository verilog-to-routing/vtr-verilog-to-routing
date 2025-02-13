/**
 * @file
 * @author Jason Luu
 * @date June 21, 2012
 *
 * @brief General API for VPR
 *
 * Other software tools should generally call just the functions defined here
 * For advanced/power users, you can call functions defined elsewhere in VPR or
 * modify the data structures directly at your discretion but be aware
 * that doing so can break the correctness of VPR
 */

#include <cstdio>
#include <cstring>
#include <cmath>
#include <memory>

#include "FlatPlacementInfo.h"
#include "cluster_util.h"
#include "place_macro.h"
#include "verify_placement.h"
#include "vpr_context.h"
#include "vtr_assert.h"
#include "vtr_log.h"
#include "vtr_version.h"
#include "vtr_time.h"

#include "vpr_types.h"
#include "vpr_utils.h"
#include "globals.h"
#include "atom_netlist.h"
#include "read_netlist.h"
#include "check_netlist.h"
#include "draw.h"
#include "place_and_route.h"
#include "pack.h"
#include "place.h"
#include "SetupGrid.h"
#include "setup_clocks.h"
#include "setup_noc.h"
#include "read_xml_noc_traffic_flows_file.h"
#include "noc_routing_algorithm_creator.h"
#include "stats.h"
#include "read_options.h"
#include "echo_files.h"
#include "SetupVPR.h"
#include "ShowSetup.h"
#include "CheckArch.h"
#include "CheckSetup.h"
#include "rr_graph.h"
#include "pb_type_graph.h"
#include "route.h"
#include "route_export.h"
#include "vpr_api.h"
#include "read_sdc.h"
#include "power.h"
#include "pack_types.h"
#include "lb_type_rr_graph.h"
#include "read_activity.h"
#include "net_delay.h"
#include "concrete_timing_info.h"
#include "netlist_writer.h"
#include "AnalysisDelayCalculator.h"
#include "check_route.h"
#include "constant_nets.h"
#include "atom_netlist_utils.h"
#include "output_clustering.h"
#include "vpr_constraints_reader.h"
#include "place_constraints.h"
#include "place_util.h"
#include "timing_fail_error.h"
#include "analytical_placement_flow.h"
#include "verify_clustering.h"

#include "vpr_constraints_writer.h"

#include "pack_report.h"
#include "overuse_report.h"

#include "timing_graph_builder.h"
#include "timing_reports.h"
#include "tatum/echo_writer.hpp"

#include "read_route.h"
#include "read_place.h"

#include "arch_util.h"

#include "post_routing_pb_pin_fixup.h"
#include "sync_netlists_to_routing_flat.h"

#include "load_flat_place.h"

#ifdef VPR_USE_TBB
#    define TBB_PREVIEW_GLOBAL_CONTROL 1 /* Needed for compatibility with old TBB versions */
#    include <tbb/task_arena.h>
#    include <tbb/global_control.h>
#endif

#ifndef NO_SERVER
#include "gateio.h"
#include "serverupdate.h"
#endif /* NO_SERVER */

/* Local subroutines */
static void free_complex_block_types();

static void free_device(const t_det_routing_arch& routing_arch);
static void free_circuit();

/* Local subroutines end */

///@brief Display general VPR information
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

/**
 * @brief Initialize VPR
 *
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

/**
 * @brief  Initialize VPR with options
 *
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
        //Use default concurrency (i.e. maximum concurrency)
        num_workers = tbb::this_task_arena::max_concurrency();
    }

    VTR_LOG("Using up to %zu parallel worker(s)\n", num_workers);
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
    vpr_setup->two_stage_clock_routing = options->two_stage_clock_routing;
    vpr_setup->exit_before_pack = options->exit_before_pack;
    vpr_setup->num_workers = num_workers;

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
    for (const std::string& func_name : vtr::split(options->disable_errors.value(), ":")) {
        map_error_activation_status(func_name);
    }

    /*
     * Initialize the functions names for which
     * warnings are being suppressed
     */
    std::vector<std::string> split_warning_option = vtr::split(options->suppress_warnings.value(), ",");
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
    for (const std::string& func_name : vtr::split(warn_functions, std::string(":"))) {
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
             &vpr_setup->APOpts,
             &vpr_setup->RouterOpts,
             &vpr_setup->AnalysisOpts,
             &vpr_setup->NocOpts,
             &vpr_setup->ServerOpts,
             &vpr_setup->RoutingArch,
             &vpr_setup->PackerRRGraph,
             vpr_setup->Segments,
             &vpr_setup->Timing,
             &vpr_setup->ShowGraphics,
             &vpr_setup->GraphPause,
             &vpr_setup->SaveGraphics,
             &vpr_setup->GraphicsCommands,
             &vpr_setup->PowerOpts,
             vpr_setup);

    /* Check inputs are reasonable */
    CheckArch(*arch);

    /* Verify settings don't conflict or otherwise not make sense */
    CheckSetup(vpr_setup->PackerOpts,
               vpr_setup->PlacerOpts,
               vpr_setup->APOpts,
               vpr_setup->RouterOpts,
               vpr_setup->ServerOpts,
               vpr_setup->RoutingArch, vpr_setup->Segments, vpr_setup->Timing, arch->Chans);

    /* flush any messages to user still in stdout that hasn't gotten displayed */
    fflush(stdout);

    /* Read blif file and sweep unused components */
    auto& atom_ctx = g_vpr_ctx.mutable_atom();
    atom_ctx.nlist = read_and_process_circuit(options->circuit_format, *vpr_setup, *arch);

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
        {
            set_terminate_if_timing_fails(options->terminate_if_timing_fails);
        }
    }

    //Initialize vpr floorplanning and routing constraints
    auto& filename_opts = vpr_setup->FileNameOpts;
    if (!filename_opts.read_vpr_constraints_file.empty()) {
        load_vpr_constraints_file(filename_opts.read_vpr_constraints_file.c_str());

        // Check if there are route constraints specified, and if the clock modeling setting is explicitly specified
        // If both conditions are met, issue a warning that the route constraints will override the clock modeling setting.
        if (g_vpr_ctx.routing().constraints.get_num_route_constraints() && options->clock_modeling.provenance() == argparse::Provenance::SPECIFIED) {
            VTR_LOG_WARN("Route constraint(s) detected and will override clock modeling setting.\n");
        }
    }

    fflush(stdout);

    auto& device_ctx = g_vpr_ctx.mutable_device();
    device_ctx.pad_loc_type = vpr_setup->PlacerOpts.pad_loc_type;
}

bool vpr_flow(t_vpr_setup& vpr_setup, t_arch& arch) {
    if (vpr_setup.exit_before_pack) {
        VTR_LOG_WARN("Exiting before packing as requested.\n");
        return true;
    }

#ifdef VPR_USE_TBB
    /* Set this here, because tbb::global_control doesn't control anything once it's out of scope
     * (contrary to the name). */
    tbb::global_control c(tbb::global_control::max_allowed_parallelism, vpr_setup.num_workers);
#endif

    { //Pack
        bool pack_success = vpr_pack_flow(vpr_setup, arch);

        if (!pack_success) {
            return false; //Unimplementable
        }
    }

    // For the time being, we decided to create the flat graph after placement is done. Thus, the is_flat parameter for this function
    //, since it is called before routing, should be false.
    vpr_create_device(vpr_setup, arch, false);

    // TODO: Placer still assumes that cluster net list is used - graphics can not work with flat routing yet
    vpr_init_graphics(vpr_setup, arch, false);

    vpr_init_server(vpr_setup);

    { //Place
        const auto& placement_net_list = (const Netlist<>&)g_vpr_ctx.clustering().clb_nlist;
        const PlaceMacros& place_macros = *g_vpr_ctx.clustering().place_macros;
        bool place_success = vpr_place_flow(placement_net_list, vpr_setup, arch, place_macros);

        if (!place_success) {
            return false; //Unimplementable
        }
    }

    { // Analytical Place
        if (vpr_setup.APOpts.doAP == STAGE_DO) {
            // TODO: Make this return a bool if the placement was successful or not.
            run_analytical_placement_flow(vpr_setup);
        }
        // Print the placement generated by AP to a .place file.
        auto& filename_opts = vpr_setup.FileNameOpts;
        auto& cluster_ctx = g_vpr_ctx.clustering();
        const auto& block_locs = g_vpr_ctx.placement().block_locs();
        auto& placement_id = g_vpr_ctx.mutable_placement().placement_id;
        placement_id = print_place(filename_opts.NetFile.c_str(),
                                   cluster_ctx.clb_nlist.netlist_id().c_str(),
                                   filename_opts.PlaceFile.c_str(),
                                   block_locs);
    }

    bool is_flat = vpr_setup.RouterOpts.flat_routing;
    const Netlist<>& router_net_list = is_flat ? (const Netlist<>&)g_vpr_ctx.atom().nlist : (const Netlist<>&)g_vpr_ctx.clustering().clb_nlist;
    const PlaceMacros& place_macros = *g_vpr_ctx.clustering().place_macros;
    RouteStatus route_status;
    { //Route
        route_status = vpr_route_flow(router_net_list, vpr_setup, arch, place_macros, is_flat);
    }
    { //Analysis
        vpr_analysis_flow(router_net_list, vpr_setup, arch, route_status, is_flat);
    }

    //close the graphics
    vpr_close_graphics(vpr_setup);

    return route_status.success();
}

void vpr_create_device(t_vpr_setup& vpr_setup, const t_arch& arch, bool is_flat) {
    vtr::ScopedStartFinishTimer timer("Create Device");
    vpr_create_device_grid(vpr_setup, arch);

    vpr_setup_clock_networks(vpr_setup, arch);

    vpr_setup_noc(vpr_setup, arch);

    if (vpr_setup.PlacerOpts.place_chan_width != NO_FIXED_CHANNEL_WIDTH) {
        vpr_create_rr_graph(vpr_setup, arch, vpr_setup.PlacerOpts.place_chan_width, is_flat);
    }
}

/**
 * @brief Allocs globals: chan_width_x, chan_width_y, device_ctx.grid
 *
 * Depends on num_clbs, pins_per_clb
 */
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

    VTR_ASSERT_MSG(device_ctx.grid.get_num_layers() <= MAX_NUM_LAYERS,
                   "Number of layers should be less than MAX_NUM_LAYERS. "
                   "If you need more layers, please increase the value of MAX_NUM_LAYERS in vpr_types.h");

    /*
     *Report on the device
     */
    size_t num_grid_tiles = count_grid_tiles(device_ctx.grid);
    VTR_LOG("FPGA sized to %zu x %zu: %zu grid tiles (%s)\n", device_ctx.grid.width(), device_ctx.grid.height(), num_grid_tiles, device_ctx.grid.name().c_str());

    VTR_LOG("\n");
    VTR_LOG("Resource usage...\n");
    for (const auto& type : device_ctx.logical_block_types) {
        if (is_empty_type(&type)) continue;

        VTR_LOG("\tNetlist\n\t\t%d\tblocks of type: %s\n",
                num_type_instances[&type], type.name.c_str());

        VTR_LOG("\tArchitecture\n");
        for (const auto equivalent_tile : type.equivalent_tiles) {
            auto num_instances = 0;
            //get the number of equivalent tile across all layers
            num_instances = (int)device_ctx.grid.num_instances(equivalent_tile, -1);

            VTR_LOG("\t\t%d\tblocks of type: %s\n",
                    num_instances, equivalent_tile->name.c_str());
        }
    }
    VTR_LOG("\n");

    float device_utilization = calculate_device_utilization(device_ctx.grid, num_type_instances);
    VTR_LOG("Device Utilization: %.2f (target %.2f)\n", device_utilization, target_device_utilization);
    for (const auto& type : device_ctx.physical_tile_types) {
        if (is_empty_type(&type)) {
            continue;
        }

        if (device_ctx.grid.num_instances(&type, -1) != 0) {
            VTR_LOG("\tPhysical Tile %s:\n", type.name.c_str());

            auto equivalent_sites = get_equivalent_sites_set(&type);

            for (auto logical_block : equivalent_sites) {
                float util = 0.;
                size_t num_inst = device_ctx.grid.num_instances(&type, -1);
                if (num_inst != 0) {
                    util = float(num_type_instances[logical_block]) / num_inst;
                }
                VTR_LOG("\tBlock Utilization: %.2f Logical Block: %s\n", util, logical_block->name.c_str());
            }
        }
    }
    VTR_LOG("\n");

    if (!device_ctx.grid.limiting_resources().empty()) {
        std::vector<std::string> limiting_block_names;
        for (auto blk_type : device_ctx.grid.limiting_resources()) {
            limiting_block_names.emplace_back(blk_type->name);
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

/**
 * @brief If the user provided the "--noc on" option then the noc is
 *        setup by creating an internal model and storing the NoC
 *        constraints. Additionally, the graphics state is updated
 *        to include a NoC button to display it.
 *
 * @param vpr_setup A data structure that stores all the user provided option
 *                  to vpr.
 * @param arch Contains the parsed information from the architecture
 *             description file.
 */
void vpr_setup_noc(const t_vpr_setup& vpr_setup, const t_arch& arch) {
    // check if the user provided the option to model the noc
    if (vpr_setup.NocOpts.noc) {
        // create the NoC model based on the user description from the arch file
        setup_noc(arch);
        // read and store the noc traffic flow information
        read_xml_noc_traffic_flows_file(vpr_setup.NocOpts.noc_flows_file.c_str());
        // create the NoC routing algorithm based on the user input
        vpr_setup_noc_routing_algorithm(vpr_setup.NocOpts.noc_routing_algorithm);

#ifndef NO_GRAPHICS
        // setup the graphics
        // if the user turned on "noc" in the command line, then we also want them to have the option to display the noc, so set that option here to be able to display it.
        // if the "noc" was not turned on, then we don't need to provide the user with the option to display it
        t_draw_state* draw_state = get_draw_state_vars();
        draw_state->show_noc_button = true;
#endif
    }
}

/**
 * @brief Constructs a NoC routing algorithm that is identified by a user
 * provided string. Then the newly created routing algorithm is stored
 * inside the global NoC context.
 *
 * @param noc_routing_algorithm_name A user provided string that identifies a
 * NoC routing algorithm
 */
void vpr_setup_noc_routing_algorithm(const std::string& noc_routing_algorithm_name) {
    // Need to be able to modify the NoC context, since we will be adding the
    // newly created routing algorithm to it
    auto& noc_ctx = g_vpr_ctx.mutable_noc();

    noc_ctx.noc_flows_router = NocRoutingAlgorithmCreator::create_routing_algorithm(noc_routing_algorithm_name,
                                                                                    noc_ctx.noc_model);
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
            if (!status) {
                return status;
            }

            //TODO: to be consistent with placement/routing vpr_pack should really
            //      load the netlist data structures itself, instead of re-loading
            //      the netlist from the .net file

            //Load the result from the .net file
            vpr_load_packing(vpr_setup, arch);
        } else {
            VTR_ASSERT(packer_opts.doPacking == STAGE_LOAD);

            // generate a .net file by legalizing an input flat placement file
            if (packer_opts.load_flat_placement) {
                //Load and legalizer flat placement file
                vpr_load_flat_placement(vpr_setup, arch);

                //Load the result from the .net file
                vpr_load_packing(vpr_setup, arch);
            } else {
                //Load a previous packing from the .net file
                vpr_load_packing(vpr_setup, arch);
            }
        }
    }

    return status;
}

bool vpr_pack(t_vpr_setup& vpr_setup, const t_arch& arch) {
    vtr::ScopedStartFinishTimer timer("Packing");

    // Read in the flat placement if a flat placement file is provided and it
    // has not been loaded already.
    if (!vpr_setup.FileNameOpts.read_flat_place_file.empty() &&
        !g_vpr_ctx.atom().flat_placement_info.valid) {
        g_vpr_ctx.mutable_atom().flat_placement_info = read_flat_placement(
                                    vpr_setup.FileNameOpts.read_flat_place_file,
                                    g_vpr_ctx.atom().nlist);
    }

    return try_pack(&vpr_setup.PackerOpts, &vpr_setup.AnalysisOpts,
                    arch, vpr_setup.RoutingArch,
                    vpr_setup.user_models,
                    vpr_setup.library_models,
                    vpr_setup.PackerRRGraph, g_vpr_ctx.atom().flat_placement_info);
}

void vpr_load_packing(const t_vpr_setup& vpr_setup, const t_arch& arch) {
    vtr::ScopedStartFinishTimer timer("Load packing");

    VTR_ASSERT_MSG(!vpr_setup.FileNameOpts.NetFile.empty(),
                   "Must have valid .net filename to load packing");

    auto& cluster_ctx = g_vpr_ctx.mutable_clustering();
    const AtomContext& atom_ctx = g_vpr_ctx.atom();

    /* Ensure we have a clean start with void net remapping information */
    cluster_ctx.post_routing_clb_pin_nets.clear();
    cluster_ctx.pre_routing_net_pin_mapping.clear();

    cluster_ctx.clb_nlist = read_netlist(vpr_setup.FileNameOpts.NetFile.c_str(),
                                         &arch,
                                         vpr_setup.FileNameOpts.verify_file_digests,
                                         vpr_setup.PackerOpts.pack_verbosity);

    /* Load the mapping between clusters and their atoms */
    init_clb_atoms_lookup(cluster_ctx.atoms_lookup, atom_ctx, cluster_ctx.clb_nlist);

    process_constant_nets(g_vpr_ctx.mutable_atom().nlist,
                          atom_ctx.lookup,
                          cluster_ctx.clb_nlist,
                          vpr_setup.constant_net_method,
                          vpr_setup.PackerOpts.pack_verbosity);

    {
        std::ofstream ofs("packing_pin_util.rpt");
        report_packing_pin_usage(ofs, g_vpr_ctx);
    }

    // Load cluster_constraints data structure.
    load_cluster_constraints();

    /* Sanity check the resulting netlist */
    check_netlist(vpr_setup.PackerOpts.pack_verbosity);

    // Independently verify the clusterings to ensure the clustering can be
    // used for the rest of the VPR flow.
    unsigned num_errors = verify_clustering(g_vpr_ctx);
    if (num_errors == 0) {
        VTR_LOG("Completed clustering consistency check successfully.\n");
    } else {
        VPR_ERROR(VPR_ERROR_PACK,
                  "%u errors found while performing clustering consistency "
                  "check. Aborting program.\n",
                  num_errors);
    }

    /* Output the netlist stats to console and optionally to file. */
    writeClusteredNetlistStats(vpr_setup.FileNameOpts.write_block_usage);

    // print the total number of used physical blocks for each
    // physical block type after finishing the packing stage
    print_pb_type_count(g_vpr_ctx.clustering().clb_nlist);

    // Alloc and load the placement macros.
    cluster_ctx.place_macros = std::make_unique<PlaceMacros>(arch.directs,
                                                             g_vpr_ctx.device().physical_tile_types,
                                                             cluster_ctx.clb_nlist,
                                                             g_vpr_ctx.atom().nlist,
                                                             g_vpr_ctx.atom().lookup);
}

bool vpr_load_flat_placement(t_vpr_setup& vpr_setup, const t_arch& arch) {

    // set up the device grid for the legalizer
    auto& device_ctx = g_vpr_ctx.mutable_device();
    device_ctx.arch = &arch;
    device_ctx.grid = create_device_grid(vpr_setup.device_layout, arch.grid_layouts);
    if (device_ctx.grid.get_num_layers() > 1) {
        VPR_FATAL_ERROR(VPR_ERROR_PACK, "Legalizer currently only supports single layer devices.\n");
    }

    // load and legalize flat placement file, print .net and fix clusters files
    bool status = load_flat_placement(vpr_setup, arch);
    if (!status) {
        return status;
    }

    // echo flat placement (orphan clusters will have -1 for X, Y, subtile coordinates)
    if (getEchoEnabled() && isEchoFileEnabled(E_ECHO_FLAT_PLACE)) {
        write_flat_placement(getEchoFileName(E_ECHO_FLAT_PLACE),
                             g_vpr_ctx.clustering().clb_nlist,
                             g_vpr_ctx.placement().block_locs(),
                             g_vpr_ctx.clustering().atoms_lookup);
    }

    // reset the device grid
    device_ctx.grid.clear();

    // if running placement, use the fix clusters file produced by the legalizer
    if (vpr_setup.PlacerOpts.doPlacement) {
        vpr_setup.PlacerOpts.constraints_file = vpr_setup.FileNameOpts.write_constraints_file;
    }
    return true;
}

bool vpr_place_flow(const Netlist<>& net_list,
                    t_vpr_setup& vpr_setup,
                    const t_arch& arch,
                    const PlaceMacros& place_macros) {
    VTR_LOG("\n");
    const auto& placer_opts = vpr_setup.PlacerOpts;
    const auto& filename_opts = vpr_setup.FileNameOpts;
    if (placer_opts.doPlacement == STAGE_SKIP) {
        //pass
    } else {
        if (placer_opts.doPlacement == STAGE_DO) {
            //Do the actual placement
            vpr_place(net_list, vpr_setup, arch, place_macros);

        } else {
            VTR_ASSERT(placer_opts.doPlacement == STAGE_LOAD);

            //Load a previous placement
            vpr_load_placement(vpr_setup);
        }

        post_place_sync();
    }

    //Write out a vpr floorplanning constraints file if the option is specified
    if (!filename_opts.write_vpr_constraints_file.empty()) {
        write_vpr_floorplan_constraints(filename_opts.write_vpr_constraints_file.c_str(), placer_opts.place_constraint_expand, placer_opts.place_constraint_subtile,
                                        placer_opts.floorplan_num_horizontal_partitions, placer_opts.floorplan_num_vertical_partitions);
    }

    // Write out a flat placement file if the option is specified
    // A flat placement file includes cluster and intra-cluster placement coordinates for
    // each primitive and can be used to reconstruct a clustering and placement solution.
    if (!filename_opts.write_flat_place_file.empty()) {
        write_flat_placement(filename_opts.write_flat_place_file.c_str(),
                             g_vpr_ctx.clustering().clb_nlist,
                             g_vpr_ctx.placement().block_locs(),
                             g_vpr_ctx.clustering().atoms_lookup);
    }

    return true;
}

void vpr_place(const Netlist<>& net_list,
               t_vpr_setup& vpr_setup,
               const t_arch& arch,
               const PlaceMacros& place_macros) {
    bool is_flat = false;
    if (vpr_setup.PlacerOpts.place_algorithm.is_timing_driven()) {
        // Prime lookahead cache to avoid adding lookahead computation cost to
        // the placer timer.
        // Flat_routing is disabled in placement
        get_cached_router_lookahead(
            vpr_setup.RoutingArch,
            vpr_setup.RouterOpts.lookahead_type,
            vpr_setup.RouterOpts.write_router_lookahead,
            vpr_setup.RouterOpts.read_router_lookahead,
            vpr_setup.Segments,
            is_flat);
    }

    // Read in the flat placement if a flat placement file is provided and it
    // has not been loaded already.
    if (!vpr_setup.FileNameOpts.read_flat_place_file.empty() &&
        !g_vpr_ctx.atom().flat_placement_info.valid) {
        g_vpr_ctx.mutable_atom().flat_placement_info = read_flat_placement(
                                    vpr_setup.FileNameOpts.read_flat_place_file,
                                    g_vpr_ctx.atom().nlist);
    }

    try_place(net_list,
              place_macros,
              vpr_setup.PlacerOpts,
              vpr_setup.RouterOpts,
              vpr_setup.AnalysisOpts,
              vpr_setup.NocOpts,
              arch.Chans,
              &vpr_setup.RoutingArch,
              vpr_setup.Segments,
              arch.directs,
              g_vpr_ctx.atom().flat_placement_info,
              is_flat);

    auto& filename_opts = vpr_setup.FileNameOpts;
    auto& cluster_ctx = g_vpr_ctx.clustering();
    const auto& block_locs = g_vpr_ctx.placement().block_locs();
    auto& placement_id = g_vpr_ctx.mutable_placement().placement_id;

    placement_id = print_place(filename_opts.NetFile.c_str(),
                               cluster_ctx.clb_nlist.netlist_id().c_str(),
                               filename_opts.PlaceFile.c_str(),
                               block_locs);
}

void vpr_load_placement(t_vpr_setup& vpr_setup) {
    vtr::ScopedStartFinishTimer timer("Load Placement");

    const auto& device_ctx = g_vpr_ctx.device();
    auto& place_ctx = g_vpr_ctx.mutable_placement();
    auto& blk_loc_registry = place_ctx.mutable_blk_loc_registry();
    const auto& filename_opts = vpr_setup.FileNameOpts;

    //Initialize placement data structures, which will be filled when loading placement
    init_placement_context(blk_loc_registry);

    //Load an existing placement from a file
    place_ctx.placement_id = read_place(filename_opts.NetFile.c_str(), filename_opts.PlaceFile.c_str(),
                                        blk_loc_registry,
                                        filename_opts.verify_file_digests, device_ctx.grid);

    // Verify that the placement invariants are met after reading the placement
    // from a file.
    unsigned num_errors = verify_placement(g_vpr_ctx);
    if (num_errors == 0) {
        VTR_LOG("Completed placement consistency check successfully.\n");
    } else {
        VPR_ERROR(VPR_ERROR_PLACE,
                  "Completed placement consistency check, %d errors found.\n"
                  "Aborting program.\n",
                  num_errors);
    }
}

RouteStatus vpr_route_flow(const Netlist<>& net_list,
                           t_vpr_setup& vpr_setup,
                           const t_arch& arch,
                           const PlaceMacros& place_macros,
                           bool is_flat) {
    VTR_LOG("\n");

    RouteStatus route_status;

    const auto& router_opts = vpr_setup.RouterOpts;
    const auto& filename_opts = vpr_setup.FileNameOpts;
    const auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;

    if (router_opts.doRouting == STAGE_SKIP) {
        //Assume successful
        route_status = RouteStatus(true, -1);
    } else { //Do or load

        // set the net_is_ignored flag for nets that have route_model set to ideal in route constraints
        apply_route_constraints(g_vpr_ctx.routing().constraints);

        int chan_width = router_opts.fixed_channel_width;

        NetPinsMatrix<float> net_delay;

        net_delay = make_net_pins_matrix<float>(net_list);

        //Initialize the delay calculator
        std::shared_ptr<SetupHoldTimingInfo> timing_info = nullptr;
        std::shared_ptr<RoutingDelayCalculator> routing_delay_calc = nullptr;
        if (vpr_setup.Timing.timing_analysis_enabled) {
            auto& atom_ctx = g_vpr_ctx.atom();
            routing_delay_calc = std::make_shared<RoutingDelayCalculator>(atom_ctx.nlist, atom_ctx.lookup, net_delay, is_flat);
            timing_info = make_setup_hold_timing_info(routing_delay_calc, router_opts.timing_update_type);
#ifndef NO_SERVER
            if (g_vpr_ctx.server().gate_io.is_running()) {
                g_vpr_ctx.mutable_server().timing_info = timing_info;
                g_vpr_ctx.mutable_server().routing_delay_calc = routing_delay_calc;
            }
#endif /* NO_SERVER */
        } else {
            /* No delay calculator (segfault if the code calls into it) and wirelength driven routing */
            timing_info = make_constant_timing_info(0);
        }

        if (router_opts.doRouting == STAGE_DO) {
            //Do the actual routing
            if (NO_FIXED_CHANNEL_WIDTH == chan_width) {
                //Find minimum channel width
                route_status = vpr_route_min_W(net_list, vpr_setup, arch, timing_info, routing_delay_calc, net_delay, place_macros, is_flat);
            } else {
                //Route at specified channel width
                route_status = vpr_route_fixed_W(net_list, vpr_setup, arch, chan_width, timing_info, routing_delay_calc, net_delay, is_flat);
            }

            //Save the routing in the .route file
            print_route(net_list,
                        filename_opts.PlaceFile.c_str(),
                        filename_opts.RouteFile.c_str(),
                        is_flat);
        } else {
            VTR_ASSERT(router_opts.doRouting == STAGE_LOAD);
            //Load a previous routing
            //if the previous load file is generated using flat routing,
            //we need to create rr_graph with is_flat flag to add additional
            //internal nodes/edges.
            if (is_flat) {
                vpr_create_rr_graph(vpr_setup, arch, chan_width, is_flat);
            }

            route_status = vpr_load_routing(vpr_setup,
                                            arch,
                                            chan_width,
                                            timing_info,
                                            net_delay,
                                            is_flat);
        }

        //Post-implementation

        std::string graphics_msg;
        if (route_status.success()) {
            //Sanity check the routing
            check_route(net_list,
                        router_opts.route_type,
                        router_opts.check_route,
                        is_flat);
            get_serial_num(net_list);

            //Update status
            VTR_LOG("Circuit successfully routed with a channel width factor of %d.\n", route_status.chan_width());
            graphics_msg = vtr::string_fmt("Routing succeeded with a channel width factor of %d.\n", route_status.chan_width());
        } else {
            //Update status
            VTR_LOG("Circuit is unroutable with a channel width factor of %d.\n", route_status.chan_width());
            graphics_msg = vtr::string_fmt("Routing failed with a channel width factor of %d. ILLEGAL routing shown.", route_status.chan_width());

            //Generate a report on overused nodes if specified
            //Otherwise, remind the user of this possible report option
            if (router_opts.generate_rr_node_overuse_report) {
                VTR_LOG("See report_overused_nodes.rpt for a detailed report on the RR node overuse information.\n");
                report_overused_nodes(net_list,
                                      rr_graph,
                                      is_flat);

            } else {
                VTR_LOG("For a detailed report on the RR node overuse information (report_overused_nodes.rpt), specify --generate_rr_node_overuse_report on.\n");
            }
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
    }

    return route_status;
}

RouteStatus vpr_route_fixed_W(const Netlist<>& net_list,
                              t_vpr_setup& vpr_setup,
                              const t_arch& arch,
                              int fixed_channel_width,
                              std::shared_ptr<SetupHoldTimingInfo> timing_info,
                              std::shared_ptr<RoutingDelayCalculator> delay_calc,
                              NetPinsMatrix<float>& net_delay,
                              bool is_flat) {
    get_cached_router_lookahead(
        vpr_setup.RoutingArch,
        vpr_setup.RouterOpts.lookahead_type,
        vpr_setup.RouterOpts.write_router_lookahead,
        vpr_setup.RouterOpts.read_router_lookahead,
        vpr_setup.Segments,
        is_flat);

    vtr::ScopedStartFinishTimer timer("Routing");

    if (NO_FIXED_CHANNEL_WIDTH == fixed_channel_width || fixed_channel_width <= 0) {
        VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "Fixed channel width must be specified when routing at fixed channel width (was %d)", fixed_channel_width);
    }
    bool status = false;
    status = route(net_list,
                   fixed_channel_width,
                   vpr_setup.RouterOpts,
                   vpr_setup.AnalysisOpts,
                   &vpr_setup.RoutingArch,
                   vpr_setup.Segments,
                   net_delay,
                   timing_info,
                   delay_calc,
                   arch.Chans,
                   arch.directs,
                   ScreenUpdatePriority::MAJOR,
                   is_flat);

    return RouteStatus(status, fixed_channel_width);
}

RouteStatus vpr_route_min_W(const Netlist<>& net_list,
                            t_vpr_setup& vpr_setup,
                            const t_arch& arch,
                            std::shared_ptr<SetupHoldTimingInfo> timing_info,
                            std::shared_ptr<RoutingDelayCalculator> delay_calc,
                            NetPinsMatrix<float>& net_delay,
                            const PlaceMacros& place_macros,
                            bool is_flat) {
    // Note that lookahead cache is not primed here because
    // binary_search_place_and_route will change the channel width, and result
    // in the lookahead cache being recomputed.
    vtr::ScopedStartFinishTimer timer("Routing");

    auto& router_opts = vpr_setup.RouterOpts;
    // Placement does not use atom_net list regardless of the status of flat_routing.
    int min_W = binary_search_place_and_route((const Netlist<>&)g_vpr_ctx.clustering().clb_nlist,
                                              net_list,
                                              vpr_setup.PlacerOpts,
                                              router_opts,
                                              vpr_setup.AnalysisOpts,
                                              vpr_setup.NocOpts,
                                              vpr_setup.FileNameOpts,
                                              &arch,
                                              place_macros,
                                              router_opts.verify_binary_search,
                                              router_opts.min_channel_width_hint,
                                              &vpr_setup.RoutingArch,
                                              vpr_setup.Segments,
                                              net_delay,
                                              timing_info,
                                              delay_calc,
                                              is_flat);

    bool status = (min_W > 0);
    return RouteStatus(status, min_W);
}

RouteStatus vpr_load_routing(t_vpr_setup& vpr_setup,
                             const t_arch& /*arch*/,
                             int fixed_channel_width,
                             std::shared_ptr<SetupHoldTimingInfo> timing_info,
                             NetPinsMatrix<float>& net_delay,
                             bool is_flat) {
    vtr::ScopedStartFinishTimer timer("Load Routing");
    if (NO_FIXED_CHANNEL_WIDTH == fixed_channel_width) {
        VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "Fixed channel width must be specified when loading routing (was %d)", fixed_channel_width);
    }

    auto& filename_opts = vpr_setup.FileNameOpts;

    //Load the routing from a file
    bool is_legal = read_route(filename_opts.RouteFile.c_str(), vpr_setup.RouterOpts, filename_opts.verify_file_digests, is_flat);
    const Netlist<>& router_net_list = is_flat ? (const Netlist<>&)g_vpr_ctx.atom().nlist : (const Netlist<>&)g_vpr_ctx.clustering().clb_nlist;
    if (vpr_setup.Timing.timing_analysis_enabled) {
        //Update timing info
        load_net_delay_from_routing(router_net_list,
                                    net_delay);
        timing_info->update();
    }
    init_draw_coords(fixed_channel_width, g_vpr_ctx.placement().blk_loc_registry());

    return RouteStatus(is_legal, fixed_channel_width);
}

void vpr_create_rr_graph(t_vpr_setup& vpr_setup, const t_arch& arch, int chan_width_fac, bool is_flat) {
    auto& device_ctx = g_vpr_ctx.mutable_device();
    auto det_routing_arch = &vpr_setup.RoutingArch;
    auto& router_opts = vpr_setup.RouterOpts;

    t_graph_type graph_type;
    t_graph_type graph_directionality;
    if (router_opts.route_type == GLOBAL) {
        graph_type = GRAPH_GLOBAL;
        graph_directionality = GRAPH_BIDIR;
    } else {
        graph_type = (det_routing_arch->directionality == BI_DIRECTIONAL ? GRAPH_BIDIR : GRAPH_UNIDIR);
        graph_directionality = (det_routing_arch->directionality == BI_DIRECTIONAL ? GRAPH_BIDIR : GRAPH_UNIDIR);
    }

    t_chan_width chan_width = init_chan(chan_width_fac, arch.Chans, graph_directionality);

    int warnings = 0;

    //Clean-up any previous RR graph
    free_rr_graph();

    //Create the RR graph
    create_rr_graph(graph_type,
                    device_ctx.physical_tile_types,
                    device_ctx.grid,
                    chan_width,
                    det_routing_arch,
                    vpr_setup.Segments,
                    router_opts,
                    arch.directs,
                    &warnings,
                    is_flat);
    //Initialize drawing, now that we have an RR graph
    init_draw_coords(chan_width_fac, g_vpr_ctx.placement().blk_loc_registry());
}

void vpr_init_graphics(const t_vpr_setup& vpr_setup, const t_arch& arch, bool is_flat) {
    /* Startup X graphics */
    init_graphics_state(vpr_setup.ShowGraphics, vpr_setup.GraphPause,
                        vpr_setup.RouterOpts.route_type, vpr_setup.SaveGraphics,
                        vpr_setup.GraphicsCommands, is_flat);
    if (vpr_setup.ShowGraphics || vpr_setup.SaveGraphics || !vpr_setup.GraphicsCommands.empty())
        alloc_draw_structs(&arch);
}

void vpr_init_server(const t_vpr_setup& vpr_setup) {
#ifndef NO_SERVER
    if (vpr_setup.ServerOpts.is_server_mode_enabled) {
        /* Set up a server and its callback to be triggered at 100ms intervals by the timer's timeout event. */
        server::GateIO& gate_io = g_vpr_ctx.mutable_server().gate_io;
        if (!gate_io.is_running()) {
            gate_io.start(vpr_setup.ServerOpts.port_num);
            g_timeout_add(/*interval_ms*/ 100, server::update, &application);
        }
    }
#else
    (void)(vpr_setup);
#endif /* NO_SERVER */
}

void vpr_close_graphics(const t_vpr_setup& /*vpr_setup*/) {
    /* Close down X Display */
    free_draw_structs();
}

///@brief Free architecture data structures
void free_device(const t_det_routing_arch& /*routing_arch*/) {
    auto& device_ctx = g_vpr_ctx.mutable_device();

    device_ctx.chan_width.x_list.clear();
    device_ctx.chan_width.y_list.clear();
    device_ctx.chan_width.max = device_ctx.chan_width.x_max = device_ctx.chan_width.y_max = device_ctx.chan_width.x_min = device_ctx.chan_width.y_min = 0;

    device_ctx.arch_switch_inf.clear();

    device_ctx.all_sw_inf.clear();

    free_complex_block_types();
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
    for (ClusterBlockId blk_id : cluster_ctx.clb_nlist.blocks()) {
        cluster_ctx.clb_nlist.remove_block(blk_id);
    }

    cluster_ctx.clb_nlist = ClusteredNetlist();
}

static void free_atoms() {
    auto& atom_ctx = g_vpr_ctx.mutable_atom();
    atom_ctx.nlist = AtomNetlist();
    atom_ctx.lookup = AtomLookup();
}

static void free_placement() {
    auto& place_ctx = g_vpr_ctx.mutable_placement();
    place_ctx.mutable_block_locs().clear();
    place_ctx.mutable_grid_blocks().clear();
}

static void free_routing() {
    auto& routing_ctx = g_vpr_ctx.mutable_routing();
    routing_ctx.route_trees.clear();
    routing_ctx.trace_nodes.clear();
    routing_ctx.net_rr_terminals.clear();
    routing_ctx.rr_blk_source.clear();
    routing_ctx.rr_blk_source.clear();
    routing_ctx.rr_node_route_inf.clear();
    routing_ctx.net_status.clear();
    routing_ctx.route_bb.clear();
}

/**
 * @brief handles the deletion of NoC related data structures.
 */
static void free_noc() {}

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
    free_noc();
}

void vpr_free_all(t_arch& Arch,
                  t_vpr_setup& vpr_setup) {
    free_rr_graph();
    if (vpr_setup.RouterOpts.doRouting) {
        free_route_structs();
    }
    vpr_free_vpr_data_structures(Arch, vpr_setup);
}

/****************************************************************************************************
 * Advanced functions
 *  Used when you need fine-grained control over VPR that the main VPR operations do not enable
 ****************************************************************************************************/

///@brief Read in user options
void vpr_read_options(const int argc, const char** argv, t_options* options) {
    *options = read_options(argc, argv);
}

///@brief Read in arch and circuit
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
                   t_ap_opts* APOpts,
                   t_router_opts* RouterOpts,
                   t_analysis_opts* AnalysisOpts,
                   t_noc_opts* NocOpts,
                   t_server_opts* ServerOpts,
                   t_det_routing_arch* RoutingArch,
                   std::vector<t_lb_type_rr_node>** PackerRRGraph,
                   std::vector<t_segment_inf>& Segments,
                   t_timing_inf* Timing,
                   bool* ShowGraphics,
                   int* GraphPause,
                   bool* SaveGraphics,
                   std::string* GraphicsCommands,
                   t_power_opts* PowerOpts,
                   t_vpr_setup* vpr_setup) {
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
             APOpts,
             RouterOpts,
             AnalysisOpts,
             NocOpts,
             ServerOpts,
             RoutingArch,
             PackerRRGraph,
             Segments,
             Timing,
             ShowGraphics,
             GraphPause,
             SaveGraphics,
             GraphicsCommands,
             PowerOpts,
             vpr_setup);
}

void vpr_check_arch(const t_arch& Arch) {
    CheckArch(Arch);
}

///@brief Verify settings don't conflict or otherwise not make sense
void vpr_check_setup(const t_packer_opts& PackerOpts,
                     const t_placer_opts& PlacerOpts,
                     const t_ap_opts& APOpts,
                     const t_router_opts& RouterOpts,
                     const t_server_opts& ServerOpts,
                     const t_det_routing_arch& RoutingArch,
                     const std::vector<t_segment_inf>& Segments,
                     const t_timing_inf& Timing,
                     const t_chan_width_dist& Chans) {
    CheckSetup(PackerOpts,
               PlacerOpts,
               APOpts,
               RouterOpts,
               ServerOpts,
               RoutingArch,
               Segments,
               Timing,
               Chans);
}

///@brief Show current setup
void vpr_show_setup(const t_vpr_setup& vpr_setup) {
    ShowSetup(vpr_setup);
}

bool vpr_analysis_flow(const Netlist<>& net_list,
                       t_vpr_setup& vpr_setup,
                       const t_arch& Arch,
                       const RouteStatus& route_status,
                       bool is_flat) {
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

    /* If routing is successful, apply post-routing annotations
     * - apply logic block pin fix-up
     *
     * Note:
     *   - Turn on verbose output when users require verbose output
     *     for packer (default verbosity is set to 2 for compact logs)
     */
    if (route_status.success()) {
        if (is_flat) {
            sync_netlists_to_routing_flat();
        } else {
            sync_netlists_to_routing(net_list,
                                     g_vpr_ctx.device(),
                                     g_vpr_ctx.mutable_atom(),
                                     g_vpr_ctx.mutable_clustering(),
                                     g_vpr_ctx.placement(),
                                     vpr_setup.PackerOpts.pack_verbosity > 2);
        }

        std::string post_routing_packing_output_file_name = vpr_setup.PackerOpts.output_file + ".post_routing";
        write_packing_results_to_xml(vpr_setup.PackerOpts.global_clocks,
                                        Arch.architecture_id,
                                        post_routing_packing_output_file_name.c_str());
    } else {
        VTR_LOG_WARN("Synchronization between packing and routing results is not applied due to illegal circuit implementation\n");
    }
    VTR_LOG("\n");

    vpr_analysis(net_list,
                 vpr_setup,
                 Arch,
                 route_status,
                 is_flat);

    return true;
}

void vpr_analysis(const Netlist<>& net_list,
                  t_vpr_setup& vpr_setup,
                  const t_arch& Arch,
                  const RouteStatus& route_status,
                  bool is_flat) {
    auto& route_ctx = g_vpr_ctx.routing();
    auto& atom_ctx = g_vpr_ctx.atom();
    const auto& blk_loc_registry = g_vpr_ctx.placement().blk_loc_registry();

    if (route_ctx.route_trees.empty()) {
        VPR_FATAL_ERROR(VPR_ERROR_ANALYSIS, "No routing loaded -- can not perform post-routing analysis");
    }

    routing_stats(net_list,
                  vpr_setup.RouterOpts.full_stats,
                  vpr_setup.RouterOpts.route_type,
                  vpr_setup.Segments,
                  vpr_setup.RoutingArch.R_minW_nmos,
                  vpr_setup.RoutingArch.R_minW_pmos,
                  Arch.grid_logic_tile_area,
                  vpr_setup.RoutingArch.directionality,
                  vpr_setup.RoutingArch.wire_to_rr_ipin_switch,
                  is_flat);

    if (vpr_setup.TimingEnabled) {
        //Load the net delays

        NetPinsMatrix<float> net_delay = make_net_pins_matrix<float>(net_list);
        load_net_delay_from_routing(net_list, net_delay);

        //Do final timing analysis
        auto analysis_delay_calc = std::make_shared<AnalysisDelayCalculator>(atom_ctx.nlist, atom_ctx.lookup, net_delay, vpr_setup.RouterOpts.flat_routing);
        auto timing_info = make_setup_hold_timing_info(analysis_delay_calc, vpr_setup.AnalysisOpts.timing_update_type);
        timing_info->update();

        if (isEchoFileEnabled(E_ECHO_ANALYSIS_TIMING_GRAPH)) {
            auto& timing_ctx = g_vpr_ctx.timing();
            tatum::write_echo(getEchoFileName(E_ECHO_ANALYSIS_TIMING_GRAPH),
                              *timing_ctx.graph, *timing_ctx.constraints, *analysis_delay_calc, timing_info->analyzer());
        }

        //Timing stats
        VTR_LOG("\n");
        generate_hold_timing_stats(/*prefix=*/"", *timing_info, *analysis_delay_calc,
                                   vpr_setup.AnalysisOpts, vpr_setup.RouterOpts.flat_routing, blk_loc_registry);
        generate_setup_timing_stats(/*prefix=*/"", *timing_info, *analysis_delay_calc,
                                    vpr_setup.AnalysisOpts, vpr_setup.RouterOpts.flat_routing, blk_loc_registry);

        //Write the post-synthesis netlist
        if (vpr_setup.AnalysisOpts.gen_post_synthesis_netlist) {
            netlist_writer(atom_ctx.nlist.netlist_name(), analysis_delay_calc,
                           vpr_setup.AnalysisOpts);
        }

        //Write the post-implementation merged netlist
        if (vpr_setup.AnalysisOpts.gen_post_implementation_merged_netlist) {
            merged_netlist_writer(atom_ctx.nlist.netlist_name(), analysis_delay_calc, vpr_setup.AnalysisOpts);
        }

        //Do power analysis
        // TODO: Still assumes that cluster net list is used
        if (vpr_setup.PowerOpts.do_power) {
            vpr_power_estimation(vpr_setup, Arch, *timing_info, route_status);
        }
    }
}

/**
 * @brief Performs power estimation.
 *
 * It relies on the placement/routing results, as well as the critical path.
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
    const char* error_type = nullptr;
    try {
        switch (vpr_error.type()) {
            case VPR_ERROR_UNKNOWN:
                error_type = "Unknown";
                break;
            case VPR_ERROR_ARCH:
                error_type = "Architecture file";
                break;
            case VPR_ERROR_PACK:
                error_type = "Packing";
                break;
            case VPR_ERROR_PLACE:
                error_type = "Placement";
                break;
            case VPR_ERROR_AP:
                error_type = "Analytical Placement";
                break;
            case VPR_ERROR_ROUTE:
                error_type = "Routing";
                break;
            case VPR_ERROR_TIMING:
                error_type = "Timing";
                break;
            case VPR_ERROR_SDC:
                error_type = "SDC file";
                break;
            case VPR_ERROR_NET_F:
                error_type = "Netlist file";
                break;
            case VPR_ERROR_BLIF_F:
                error_type = "Blif file";
                break;
            case VPR_ERROR_PLACE_F:
                error_type = "Placement file";
                break;
            case VPR_ERROR_IMPL_NETLIST_WRITER:
                error_type = "Implementation Netlist Writer";
                break;
            case VPR_ERROR_ATOM_NETLIST:
                error_type = "Atom Netlist";
                break;
            case VPR_ERROR_POWER:
                error_type = "Power";
                break;
            case VPR_ERROR_ANALYSIS:
                error_type = "Analysis";
                break;
            case VPR_ERROR_OTHER:
                error_type = "Other";
                break;
            case VPR_ERROR_INTERRUPTED:
                error_type = "Interrupted";
                break;
            default:
                error_type = "Unrecognized Error";
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
