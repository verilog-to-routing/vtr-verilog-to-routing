#include "read_options.h"

#include "argparse.hpp"

#include "vtr_memory.h"
#include "vtr_log.h"
#include "vtr_util.h"

#include "vpr_error.h"

using argparse::Provenance;

static argparse::ArgumentParser create_arg_parser(std::string prog_name, t_options& args);
static void set_conditional_defaults(t_options& args);
static bool verify_args(const t_options& args);

//Read and process VPR's command-line aruments
t_options read_options(int argc, const char** argv) {
    t_options args = t_options(); //Explicitly initialize for zero initialization

    auto parser = create_arg_parser(argv[0], args);

    parser.parse_args(argc, argv);

    set_conditional_defaults(args);

    verify_args(args);

    return args;
}

struct ParseOnOff {
    bool from_str(std::string str) {
        if      (str == "on")  return true;
        else if (str == "off") return false;
        std::stringstream msg;
        msg << "Invalid conversion from '" << str << "' to boolean (expected one of: " << argparse::join(default_choices(), ", ") << ")";
        throw argparse::ArgParseConversionError(msg.str());
    }

    std::string to_str(bool val) {
        if (val) return "on";
        return "off";
    }

    std::vector<std::string> default_choices() {
        return {"on", "off"};
    }
};

struct ParseRoutePredictor {
    e_routing_failure_predictor from_str(std::string str) {
        if      (str == "safe")  return SAFE;
        else if (str == "aggressive") return AGGRESSIVE;
        else if (str == "off") return OFF;
        std::stringstream msg;
        msg << "Invalid conversion from '" << str << "' to e_routing_failure_predictor (expected one of: " << argparse::join(default_choices(), ", ") << ")";
        throw argparse::ArgParseConversionError(msg.str());
    }

    std::string to_str(e_routing_failure_predictor val) {
        if (val == SAFE) return "safe";
        else if (val == AGGRESSIVE) return "aggressive";
        VTR_ASSERT(val == OFF);
        return "off";
    }

    std::vector<std::string> default_choices() {
        return {"safe", "aggressive", "off"};
    }
};

struct ParseRouterAlgorithm {
    e_router_algorithm from_str(std::string str) {
        if      (str == "breadth_first")  return BREADTH_FIRST;
        else if (str == "timing_driven") return TIMING_DRIVEN;
        std::stringstream msg;
        msg << "Invalid conversion from '" << str << "' to e_router_algorithm (expected one of: " << argparse::join(default_choices(), ", ") << ")";
        throw argparse::ArgParseConversionError(msg.str());
    }

    std::string to_str(e_router_algorithm val) {
        if (val == BREADTH_FIRST) return "breadth_first";
        VTR_ASSERT(val == TIMING_DRIVEN);
        return "timing_driven";
    }

    std::vector<std::string> default_choices() {
        return {"breadth_first", "timing_driven"};
    }
};

struct RouteBudgetsAlgorithm {
    e_routing_budgets_algorithm from_str(std::string str) {
        if      (str == "minimax")  return MINIMAX;
        else if (str == "scale_delay") return SCALE_DELAY;
        else if (str == "disable") return DISABLE;
        std::stringstream msg;
        msg << "Invalid conversion from '" << str << "' to e_routing_budget_algorithm (expected one of: " << argparse::join(default_choices(), ", ") << ")";
        throw argparse::ArgParseConversionError(msg.str());
    }

    std::string to_str(e_routing_budgets_algorithm val) {
        if (val == MINIMAX) return "minimax";
        if (val == DISABLE) return "disable";
        VTR_ASSERT(val == SCALE_DELAY);
        return "scale_delay";
    }

    std::vector<std::string> default_choices() {
        return {"minimax", "scale_delay", "disable"};
    }
};

struct ParseRouteType {
    e_route_type from_str(std::string str) {
        if      (str == "global")  return GLOBAL;
        else if (str == "detailed") return DETAILED;
        std::stringstream msg;
        msg << "Invalid conversion from '" << str << "' to e_router_algorithm (expected one of: " << argparse::join(default_choices(), ", ") << ")";
        throw argparse::ArgParseConversionError(msg.str());
    }

    std::string to_str(e_route_type val) {
        if (val == GLOBAL) return "global";
        VTR_ASSERT(val == DETAILED);
        return "detailed";
    }

    std::vector<std::string> default_choices() {
        return {"global", "detailed"};
    }
};

struct ParseBaseCost {
    e_base_cost_type from_str(std::string str) {
        if      (str == "delay_normalized")  return DELAY_NORMALIZED;
        else if (str == "demand_only") return DEMAND_ONLY;
        std::stringstream msg;
        msg << "Invalid conversion from '" << str << "' to e_router_algorithm (expected one of: " << argparse::join(default_choices(), ", ") << ")";
        throw argparse::ArgParseConversionError(msg.str());
    }

    std::string to_str(e_base_cost_type val) {
        if (val == DELAY_NORMALIZED) return "delay_normalized";
        VTR_ASSERT(val == DEMAND_ONLY);
        return "demand_only";
    }

    std::vector<std::string> default_choices() {
        return {"delay_normalized", "demand_only"};
    }
};

struct ParsePlaceAlgorithm {
    e_place_algorithm from_str(std::string str) {
        if      (str == "bounding_box")  return BOUNDING_BOX_PLACE;
        else if (str == "path_timing_driven") return PATH_TIMING_DRIVEN_PLACE;
        std::stringstream msg;
        msg << "Invalid conversion from '" << str << "' to e_router_algorithm (expected one of: " << argparse::join(default_choices(), ", ") << ")";
        throw argparse::ArgParseConversionError(msg.str());
    }

    std::string to_str(e_place_algorithm val) {
        if (val == BOUNDING_BOX_PLACE) return "bounding_box";
        VTR_ASSERT(val == PATH_TIMING_DRIVEN_PLACE);
        return "path_timing_driven";
    }

    std::vector<std::string> default_choices() {
        return {"bounding_box", "path_timing_driven"};
    }
};

struct ParseClusterSeed {
    e_cluster_seed from_str(std::string str) {
        if      (str == "timing")  return VPACK_TIMING;
        else if (str == "max_inputs") return VPACK_MAX_INPUTS;
        else if (str == "blend") return VPACK_BLEND;
        std::stringstream msg;
        msg << "Invalid conversion from '" << str << "' to e_router_algorithm (expected one of: " << argparse::join(default_choices(), ", ") << ")";
        throw argparse::ArgParseConversionError(msg.str());
    }

    std::string to_str(e_cluster_seed val) {
        if (val == VPACK_TIMING) return "timing";
        else if (val == VPACK_MAX_INPUTS) return "max_inputs";
        VTR_ASSERT(val == VPACK_BLEND);
        return "blend";
    }

    std::vector<std::string> default_choices() {
        return {"timing", "max_inputs", "blend"};
    }
};

static argparse::ArgumentParser create_arg_parser(std::string prog_name, t_options& args) {
    std::string description = "Implements the specified circuit onto the target FPGA architecture"
                              " by performing packing/placement/routing, and analyzes the result.\n"
                              "\n"
                              "Attempts to find the minimum routable channel width, unless a fixed"
                              " channel width is specified with --route_chan_width."
                              ;
    auto parser = argparse::ArgumentParser(prog_name, description);

    std::string epilog = vtr::replace_all( 
        "Usage Examples\n"
        "--------------\n"
        "   #Find the minimum routable channel width of my_circuit on my_arch\n"
        "   {prog} my_arch.xml my_circuit.blif\n"
        "\n"
        "   #Show interactive graphics\n"
        "   {prog} my_arch.xml my_circuit.blif --disp on\n"
        "\n"
        "   #Implement at a fixed channel width of 100\n"
        "   {prog} my_arch.xml my_circuit.blif --route_chan_width 100\n"
        "\n"
        "   #Perform packing and placement only\n"
        "   {prog} my_arch.xml my_circuit.blif --pack --place\n"
        "\n"
        "   #Generate post-implementation netlist\n"
        "   {prog} my_arch.xml my_circuit.blif --gen_post_synthesis_netlist on\n"
        "\n"
        "   #Write routing-resource graph to a file\n"
        "   {prog} my_arch.xml my_circuit.blif --write_rr_graph my_rr_graph.xml\n"
        "\n"
        "\n"
        "For additional documentation see: https://docs.verilogtorouting.org",
        "{prog}", parser.prog());
    parser.epilog(epilog);


    auto& pos_grp = parser.add_argument_group("positional arguments");
    pos_grp.add_argument(args.ArchFile, "architecture")
            .help("FPGA Architecture description file (XML)");

    pos_grp.add_argument(args.CircuitName, "circuit")
            .help("Circuit file (or circuit name if --blif_file specified)");


    auto& stage_grp = parser.add_argument_group("stage options");

    stage_grp.add_argument<bool,ParseOnOff>(args.do_packing, "--pack")
            .help("Run packing")
            .action(argparse::Action::STORE_TRUE)
            .default_value("off");

    stage_grp.add_argument<bool,ParseOnOff>(args.do_placement, "--place")
            .help("Run placement")
            .action(argparse::Action::STORE_TRUE)
            .default_value("off");

    stage_grp.add_argument<bool,ParseOnOff>(args.do_routing, "--route")
            .help("Run routing")
            .action(argparse::Action::STORE_TRUE)
            .default_value("off");

    stage_grp.add_argument<bool,ParseOnOff>(args.do_analysis, "--analysis")
            .help("Run analysis")
            .action(argparse::Action::STORE_TRUE)
            .default_value("off");

    stage_grp.epilog("If none of the stage options are specified, all stages are run.\n"
                     "Analysis is always run after routing.");


    auto& gfx_grp = parser.add_argument_group("graphics options");

    gfx_grp.add_argument<bool,ParseOnOff>(args.show_graphics, "--disp")
            .help("Enable or disable interactive graphics")
            .default_value("off");

    gfx_grp.add_argument(args.GraphPause, "--auto")
            .help("Controls how often VPR pauses for interactive"
                  " graphics (requiring Proceed to be clicked)."
                  " Higher values pause less frequently")
            .default_value("1")
            .choices({"0", "1", "2"})
            .show_in(argparse::ShowIn::HELP_ONLY);


    auto& gen_grp = parser.add_argument_group("general options");

    gen_grp.add_argument(args.show_help, "--help", "-h")
            .help("Show this help message then exit")
            .action(argparse::Action::HELP);

    gen_grp.add_argument<bool,ParseOnOff>(args.show_version, "--version")
            .help("Show version information then exit")
            .action(argparse::Action::VERSION);

    gen_grp.add_argument<std::string>(args.device_layout, "--device")
            .help("Controls which device layout/floorplan is used from the architecture file."
                  " 'auto' uses the smallest device which satisfies the circuit's resource requirements.")
            .metavar("DEVICE_NAME")
            .default_value("auto");

    gen_grp.add_argument<size_t>(args.num_workers, "--num_workers", "-j")
            .help("Controls how many workers VPR may use. Values > 1 imply VPR may execute in parallel.")
            .default_value("1");

    gen_grp.add_argument<bool,ParseOnOff>(args.timing_analysis, "--timing_analysis")
            .help("Controls whether timing analysis (and timing driven optimizations) are enabled.")
            .default_value("on");

#ifdef ENABLE_CLASSIC_VPR_STA
    gen_grp.add_argument(args.SlackDefinition, "--slack_definition")
            .help("Sets the slack definition used by the classic timing analyzer")
            .default_value("R")
            .choices({"R", "I", "S", "G", "C", "N"})
            .show_in(argparse::ShowIn::HELP_ONLY);
#endif

    gen_grp.add_argument<bool,ParseOnOff>(args.CreateEchoFile, "--echo_file")
            .help("Generate echo files of key internal data structures."
                  " Useful for debugging VPR, and typically end in .echo")
            .default_value("off")
            .show_in(argparse::ShowIn::HELP_ONLY);

    gen_grp.add_argument<bool,ParseOnOff>(args.verify_file_digests, "--verify_file_digests")
            .help("Verify that files loaded by VPR (e.g. architecture, netlist,"
                  " previous packing/placement/routing) are consistent")
            .default_value("on")
            .show_in(argparse::ShowIn::HELP_ONLY);

    gen_grp.add_argument(args.target_device_utilization, "--target_utilization")
            .help("Sets the target device utilization."
                  " This corresponds to the maximum target fraction of device grid-tiles to be used."
                  " A value of 1.0 means the smallest device (which fits the circuit) will be used.")
            .default_value("1.0")
            .show_in(argparse::ShowIn::HELP_ONLY);

    auto& file_grp = parser.add_argument_group("filename options");

    file_grp.add_argument(args.BlifFile, "--blif_file")
            .help("Path to technology mapped circuit in BLIF format")
            .show_in(argparse::ShowIn::HELP_ONLY);

    file_grp.add_argument(args.NetFile, "--net_file")
            .help("Path to packed netlist file")
            .show_in(argparse::ShowIn::HELP_ONLY);

    file_grp.add_argument(args.PlaceFile, "--place_file")
            .help("Path to placement file")
            .show_in(argparse::ShowIn::HELP_ONLY);

    file_grp.add_argument(args.RouteFile, "--route_file")
            .help("Path to routing file")
            .show_in(argparse::ShowIn::HELP_ONLY);

    file_grp.add_argument(args.SDCFile, "--sdc_file")
            .help("Path to timing constraints file in SDC format")
            .show_in(argparse::ShowIn::HELP_ONLY);

    file_grp.add_argument(args.read_rr_graph_file, "--read_rr_graph")
            .help("The routing resource graph file to load."
                  " The loaded routing resource graph overrides any routing architecture specified in the architecture file.")
            .metavar("RR_GRAPH_FILE")
            .show_in(argparse::ShowIn::HELP_ONLY);

    file_grp.add_argument(args.write_rr_graph_file, "--write_rr_graph")
            .help("Writes the routing resource graph to the specified file")
            .metavar("RR_GRAPH_FILE")
            .show_in(argparse::ShowIn::HELP_ONLY);

    file_grp.add_argument(args.dump_rr_structs_file, "--dump_rr_structs_file")
            .help("Dumps RR graph related strcutres to a file")
            .show_in(argparse::ShowIn::HELP_ONLY);

    file_grp.add_argument(args.out_file_prefix, "--outfile_prefix")
            .help("Prefix for output files")
            .show_in(argparse::ShowIn::HELP_ONLY);


    auto& netlist_grp = parser.add_argument_group("netlist options");

    netlist_grp.add_argument<bool,ParseOnOff>(args.absorb_buffer_luts, "--absorb_buffer_luts")
            .help("Controls whether LUTS programmed as buffers are absorbed by downstream logic")
            .default_value("on")
            .show_in(argparse::ShowIn::HELP_ONLY);

    netlist_grp.add_argument<bool,ParseOnOff>(args.sweep_dangling_primary_ios, "--sweep_dangling_primary_ios")
            .help("Controls whether dangling primary inputs and outputs are removed from the netlist")
            .default_value("on")
            .show_in(argparse::ShowIn::HELP_ONLY);

    netlist_grp.add_argument<bool,ParseOnOff>(args.sweep_dangling_nets, "--sweep_dangling_nets")
            .help("Controls whether dangling nets are removed from the netlist")
            .default_value("on")
            .show_in(argparse::ShowIn::HELP_ONLY);

    netlist_grp.add_argument<bool,ParseOnOff>(args.sweep_dangling_blocks, "--sweep_dangling_blocks")
            .help("Controls whether dangling blocks are removed from the netlist")
            .default_value("on")
            .show_in(argparse::ShowIn::HELP_ONLY);

    netlist_grp.add_argument<bool,ParseOnOff>(args.sweep_constant_primary_outputs, "--sweep_constant_primary_outputs")
            .help("Controls whether primary outputs driven by constant values are removed from the netlist")
            .default_value("off")
            .show_in(argparse::ShowIn::HELP_ONLY);


    auto& pack_grp = parser.add_argument_group("packing options");

    pack_grp.add_argument<bool,ParseOnOff>(args.connection_driven_clustering, "--connection_driven_clustering")
            .help("Controls whether or not packing prioritizes the absorption of nets with fewer"
                  " connections into a complex logic block over nets with more connections")
            .default_value("on")
            .show_in(argparse::ShowIn::HELP_ONLY);

    pack_grp.add_argument<bool,ParseOnOff>(args.allow_unrelated_clustering, "--allow_unrelated_clustering")
            .help("Controls whether or not primitives with no attraction to the current cluster"
                  " can be packed into it")
            .default_value("on")
            .show_in(argparse::ShowIn::HELP_ONLY);

    pack_grp.add_argument(args.alpha_clustering, "--alpha_clustering")
            .help("Parameter that weights the optimization of timing vs area. 0.0 focuses solely on"
                  " area, 1.0 solely on timing.")
            .default_value("0.75")
            .show_in(argparse::ShowIn::HELP_ONLY);

    pack_grp.add_argument(args.beta_clustering, "--beta_clustering")
            .help("Parameter that weights the absorption of small nets vs signal sharing."
                  " 0.0 focuses solely on sharing, 1.0 solely on small net absoprtion."
                  " Only meaningful if --connection_driven_clustering=on")
            .default_value("0.9")
            .show_in(argparse::ShowIn::HELP_ONLY);

    pack_grp.add_argument<bool,ParseOnOff>(args.timing_driven_clustering, "--timing_driven_clustering")
            .help("Controls whether custering optimizes for timing")
            .default_value("on")
            .show_in(argparse::ShowIn::HELP_ONLY);

    pack_grp.add_argument<e_cluster_seed,ParseClusterSeed>(args.cluster_seed_type, "--cluster_seed_type")
            .help("Controls how primitives are chosen as seeds."
                  " (Default: blend if timing driven, max_inputs otherwise)")
            .choices({"blend", "timing", "max_inputs"})
            .show_in(argparse::ShowIn::HELP_ONLY);


    auto& place_grp = parser.add_argument_group("placement options");

    place_grp.add_argument(args.Seed, "--seed")
            .help("Placement random number generator seed")
            .default_value("1")
            .show_in(argparse::ShowIn::HELP_ONLY);

    place_grp.add_argument<bool,ParseOnOff>(args.ShowPlaceTiming, "--enable_timing_computations")
            .help("Displays delay statistics even if placement is not timing driven")
            .show_in(argparse::ShowIn::HELP_ONLY);

    place_grp.add_argument(args.PlaceInnerNum, "--inner_num")
            .help("Controls number of moves per temperature: inner_num * num_blocks ^ (4/3)")
            .default_value("1.0")
            .show_in(argparse::ShowIn::HELP_ONLY);

    place_grp.add_argument(args.PlaceInitT, "--init_t")
            .help("Initial temperature for manual annealing schedule")
            .default_value("100.0")
            .show_in(argparse::ShowIn::HELP_ONLY);

    place_grp.add_argument(args.PlaceExitT, "--exit_t")
            .help("Temperature at which annealing which terminate for manual annealing schedule")
            .default_value("0.01")
            .show_in(argparse::ShowIn::HELP_ONLY);

    place_grp.add_argument(args.PlaceAlphaT, "--alpha_t")
            .help("Temperature scaling factor for manual annealing schedule."
                  " Old temperature is multiplied by alpha_t")
            .default_value("0.01")
            .show_in(argparse::ShowIn::HELP_ONLY);

    place_grp.add_argument(args.pad_loc_file, "--fix_pins")
            .help("Fixes I/O pad locations during placement. Valid options:\n"
                  " * 'free' allows placement to optimize pad locations\n"
                  " * 'random' fixes pad locations to arbitraray locations\n"
                  " * path to a file specifying pad locations (.place format with only pads specified).")
            .default_value("free")
            .show_in(argparse::ShowIn::HELP_ONLY);

    place_grp.add_argument<e_place_algorithm,ParsePlaceAlgorithm>(args.PlaceAlgorithm, "--place_algorithm")
            .help("Controls which placement algorithm is used")
            .default_value("path_timing_driven")
            .choices({"bounding_box", "path_timing_driven"})
            .show_in(argparse::ShowIn::HELP_ONLY);

    place_grp.add_argument(args.PlaceChanWidth, "--place_chan_width")
            .help("Sets the assumed channel width during placement")
            .default_value("100")
            .show_in(argparse::ShowIn::HELP_ONLY);


    auto& place_timing_grp = parser.add_argument_group("timing-driven placement options");

    place_timing_grp.add_argument(args.PlaceTimingTradeoff, "--timing_tradeoff")
            .help("Trade-off control between delay and wirelength during placement."
                  " 0.0 focuses completely on wirelength, 1.0 completely on timing")
            .default_value("0.5")
            .show_in(argparse::ShowIn::HELP_ONLY);

    place_timing_grp.add_argument(args.RecomputeCritIter, "--recompute_crit_iter")
            .help("Controls how many temperature updates occur between timing analysis during placement")
            .default_value("1")
            .show_in(argparse::ShowIn::HELP_ONLY);

    place_timing_grp.add_argument(args.inner_loop_recompute_divider, "--inner_loop_recompute_divider")
            .help("Controls how many timing analysies are perform per temperature during placement")
            .default_value("0")
            .show_in(argparse::ShowIn::HELP_ONLY);

    place_timing_grp.add_argument(args.place_exp_first, "--td_place_exp_first")
            .help("Controls how critical a connection is as a function of slack at the start of placement."
                  " A value of zero treats all connections as equally critical (regardless of slack)."
                  " Values larger than 1.0 cause low slack connections to be treated more critically."
                  " The value increases to --td_place_exp_last during placement.")
            .default_value("1.0")
            .show_in(argparse::ShowIn::HELP_ONLY);

    place_timing_grp.add_argument(args.place_exp_last, "--td_place_exp_last")
            .help("Controls how critical a connection is as a function of slack at the end of placement.")
            .default_value("8.0")
            .show_in(argparse::ShowIn::HELP_ONLY);


    auto& route_grp = parser.add_argument_group("routing options");

    route_grp.add_argument(args.max_router_iterations, "--max_router_iterations")
            .help("Maximum number of Pathfinder-based routing iterations before the circuit is"
                  " declared unroutable at a given channel width")
            .default_value("50")
            .show_in(argparse::ShowIn::HELP_ONLY);

    route_grp.add_argument(args.first_iter_pres_fac, "--first_iter_pres_fac")
            .help("Sets the present overuse factor for the first routing iteration")
            .default_value("0.0")
            .show_in(argparse::ShowIn::HELP_ONLY);

    route_grp.add_argument(args.initial_pres_fac, "--initial_pres_fac")
            .help("Sets the present overuse factor for the second routing iteration")
            .default_value("0.5")
            .show_in(argparse::ShowIn::HELP_ONLY);

    route_grp.add_argument(args.pres_fac_mult, "--pres_fac_mult")
            .help("Sets the growth factor by which the present overuse penalty factor is"
                  " multiplied after each routing iteration")
            .default_value("1.3")
            .show_in(argparse::ShowIn::HELP_ONLY);

    route_grp.add_argument(args.acc_fac, "--acc_fac")
            .help("Specifies the accumulated overuse factor (historical congestion cost factor)")
            .default_value("1.0")
            .show_in(argparse::ShowIn::HELP_ONLY);

    route_grp.add_argument(args.bb_factor, "--bb_factor")
            .help("Sets the distance (in channels) outside a connection's bounding box which can be explored during routing")
            .default_value("3")
            .show_in(argparse::ShowIn::HELP_ONLY);

    route_grp.add_argument<e_base_cost_type,ParseBaseCost>(args.base_cost_type, "--base_cost_type")
            .help("Sets the basic cost of routing resource nodes:\n"
                  " * demand_only: based on expected demand of node type\n"
                  " * delay_normalized: like demand_only but normalized to magnitude of typical routing resource delay\n"
                  "(Default: demand_only for breadth-first router, delay_normalized for timing-driven router)")
            .choices({"demand_only", "delay_normalized"})
            .show_in(argparse::ShowIn::HELP_ONLY);

    route_grp.add_argument(args.bend_cost, "--bend_cost")
            .help("The cost of a bend. (Default: 1.0 for global routing, 0.0 for detailed routing)")
            .show_in(argparse::ShowIn::HELP_ONLY);

    route_grp.add_argument<e_route_type,ParseRouteType>(args.RouteType, "--route_type")
            .help("Specifies whether global, or combined global and detailed routing is performed.")
            .default_value("detailed")
            .choices({"global", "detailed"})
            .show_in(argparse::ShowIn::HELP_ONLY);

    route_grp.add_argument(args.RouteChanWidth, "--route_chan_width")
            .help("Specifies a fixed channel width to route at.")
            .default_value("-1")
            .metavar("CHANNEL_WIDTH");

    route_grp.add_argument(args.min_route_chan_width_hint, "--min_route_chan_width_hint")
            .help("Hint to the router what the minimum routable channel width is."
                  " Good hints can speed-up determining the minimum channel width.")
            .show_in(argparse::ShowIn::HELP_ONLY);

    route_grp.add_argument<bool,ParseOnOff>(args.verify_binary_search, "--verify_binary_search")
            .help("Force the router to verify the minimum channel width by routing at"
                  " consecutively lower channel widths until two consecutive failures are observed.")
            .default_value("off")
            .show_in(argparse::ShowIn::HELP_ONLY);

    route_grp.add_argument<e_router_algorithm,ParseRouterAlgorithm>(args.RouterAlgorithm, "--router_algorithm")
            .help("Specifies the router algorithm to use.\n"
                  " * breadth_first: focuses solely on routability\n"
                  " * timing driven: focuses on routability and circuit speed\n")
            .default_value("timing_driven")
            .choices({"breadth_first", "timing_driven"})
            .show_in(argparse::ShowIn::HELP_ONLY);

    route_grp.add_argument(args.min_incremental_reroute_fanout, "--min_incremental_reroute_fanout")
            .help("The net fanout threshold above which nets will be re-routed incrementally.")
            /* Based on testing, choosing a low threshold can lead to instability
               where sometimes route time and critical path are degraded. 64 seems
               to be a reasonable choice for most circuits. For nets with a greater
               distribution of high fanout nets, choose a larger threshold */
            .default_value("64")
            .show_in(argparse::ShowIn::HELP_ONLY);


    auto& route_timing_grp = parser.add_argument_group("timing-driven routing options");

    route_timing_grp.add_argument(args.astar_fac, "--astar_fac")
            .help("Controls the directedness of the the timing-driven router's exploration."
                  " Values between 1 and 2 are resonable; higher values trade some quality for reduced run-time")
            .default_value("1.2")
            .show_in(argparse::ShowIn::HELP_ONLY);

    route_timing_grp.add_argument(args.max_criticality, "--max_criticality")
            .help("Sets the maximum fraction of routing cost derived from delay (vs routability) for any net."
                  " 0.0 means no attention is paid to delay, 1.0 means nets on the critical path ignore congestion")
            .default_value("0.99")
            .show_in(argparse::ShowIn::HELP_ONLY);

    route_timing_grp.add_argument(args.criticality_exp, "--criticality_exp")
            .help("Controls the delay-routability trade-off for nets as a function of slack."
                  " 0.0 implies all nets treated equally regardless of slack."
                  " At large values (>> 1) only nets on the critical path will consider delay.")
            .default_value("1.0")
            .show_in(argparse::ShowIn::HELP_ONLY);

    route_timing_grp.add_argument<e_routing_failure_predictor,ParseRoutePredictor>(args.routing_failure_predictor, "--routing_failure_predictor")
            .help("Controls how aggressively the router will predict a routing as unsuccessful"
                  " and give up early. This can significantly reducing the run-time required"
                  " to find the minimum channel width.\n"
                  " * safe: Only abort when it is extremely unlikely a routing will succeed\n"
                  " * aggressive: Further reduce run-time by giving up earlier. This may increase the reported minimum channel width\n"
                  " * off: Only abort when the maximum number of iterations is reached\n")
            .default_value("safe")
            .choices({"safe", "aggressive", "off"})
            .show_in(argparse::ShowIn::HELP_ONLY);
            
    route_timing_grp.add_argument<e_routing_budgets_algorithm,RouteBudgetsAlgorithm>(args.routing_budgets_algorithm, "--routing_budgets_algorithm")
            .help("Controls how the routing budgets are created.\n"
                  " * slack: Sets the budgets depending on the amount slack between connections and the current delay values.\n"
                  " * criticality: Sets the minimum budgets to 0 and the maximum budgets as a function of delay and criticality (net delay/ pin criticality)\n"
                  " * disable: Removes the routing budgets, use the default VPR and ignore hold time constraints\n")
            .default_value("minimax")
            .choices({"minimax", "scale_delay", "disable"})
            .show_in(argparse::ShowIn::HELP_ONLY);

    auto& analysis_grp = parser.add_argument_group("analysis options");

    analysis_grp.add_argument<bool,ParseOnOff>(args.full_stats, "--full_stats")
            .help("Print extra statistics about the circuit and it's routing (useful for wireability analysis)")
            .default_value("off")
            .show_in(argparse::ShowIn::HELP_ONLY);

    analysis_grp.add_argument<bool,ParseOnOff>(args.Generate_Post_Synthesis_Netlist, "--gen_post_synthesis_netlist")
            .help("Generates the post-synthesis netlist (in BLIF and Verilog) along with delay information (in SDF)."
                  " Used for post-implementation simulation and verification")
            .default_value("off")
            .show_in(argparse::ShowIn::HELP_ONLY);


    auto& power_grp = parser.add_argument_group("power analysis options");

    power_grp.add_argument<bool,ParseOnOff>(args.do_power, "--power")
            .help("Enable power estimation")
            .action(argparse::Action::STORE_TRUE)
            .default_value("off")
            .show_in(argparse::ShowIn::HELP_ONLY);

    power_grp.add_argument(args.CmosTechFile, "--tech_properties")
            .help("XML file containing CMOS technology properties (see documentation).")
            .show_in(argparse::ShowIn::HELP_ONLY);

    power_grp.add_argument(args.ActFile, "--activity_file")
            .help("Signal activities file for all nets (see documentation).")
            .show_in(argparse::ShowIn::HELP_ONLY);

    return parser;
}

static void set_conditional_defaults(t_options& args) {
    //Some arguments are set conditionally based on other options.
    //These are resolved here.

    /*
     * Stages
     */
    if(   !args.do_packing
       && !args.do_placement
       && !args.do_routing
       && !args.do_analysis) {
        //If no stage options are specified, run all
        args.do_packing.set(true, Provenance::INFERRED);
        args.do_placement.set(true, Provenance::INFERRED);
        args.do_routing.set(true, Provenance::INFERRED);
        args.do_analysis.set(true, Provenance::INFERRED);
    }

    //Always run analysis after routing
    if(args.do_routing && !args.do_analysis) {
        args.do_analysis.set(true, Provenance::INFERRED);
    }

    /*
     * Filenames
     */

    //We may have recieved the full circuit filepath in the circuit name,
    //remove the extension and any leading path elements
    VTR_ASSERT(args.CircuitName.provenance() == Provenance::SPECIFIED);
    auto name_ext = vtr::split_ext(args.CircuitName);

	if (args.BlifFile.provenance() != Provenance::SPECIFIED) {
        //If the blif file wasn't explicitly specified, interpret the circuit name
        //as the blif file, and split off the extension
        args.CircuitName.set(vtr::basename(name_ext[0]), Provenance::SPECIFIED);
    }

    std::string default_output_name = args.CircuitName;

	if (args.BlifFile.provenance() != Provenance::SPECIFIED) {
        //Use the full path specified in the original circuit name,
        //and append the expected .blif extension
        std::string blif_file = name_ext[0] + ".blif";
		args.BlifFile.set(blif_file, Provenance::INFERRED);
	}

	if (args.SDCFile.provenance() != Provenance::SPECIFIED) {
        //Use the full path specified in the original circuit name,
        //and append the expected .sdc extension
        std::string sdc_file = default_output_name + ".sdc";
		args.SDCFile.set(sdc_file, Provenance::INFERRED);
	}

	if (args.NetFile.provenance() != Provenance::SPECIFIED) {
        std::string net_file = args.out_file_prefix;
        net_file += default_output_name + ".net";
		args.NetFile.set(net_file, Provenance::INFERRED);
	}

	if (args.PlaceFile.provenance() != Provenance::SPECIFIED) {
        std::string place_file = args.out_file_prefix;
        place_file += default_output_name + ".place";
		args.PlaceFile.set(place_file, Provenance::INFERRED);
	}

	if (args.RouteFile.provenance() != Provenance::SPECIFIED) {
        std::string route_file = args.out_file_prefix;
        route_file += default_output_name + ".route";
		args.RouteFile.set(route_file, Provenance::INFERRED);
	}

	if (args.ActFile.provenance() != Provenance::SPECIFIED) {
        std::string activity_file = args.out_file_prefix;
        activity_file += default_output_name + ".act";
		args.ActFile.set(activity_file, Provenance::INFERRED);
	}

	if (args.PowerFile.provenance() != Provenance::SPECIFIED) {
        std::string power_file = args.out_file_prefix;
        power_file += default_output_name + ".power";
		args.PowerFile.set(power_file, Provenance::INFERRED);
	}

    /*
     * Packing
     */
    if (args.timing_driven_clustering && !args.timing_analysis) {
        if (args.timing_driven_clustering.provenance() == Provenance::SPECIFIED) {
            vtr::printf_warning(__FILE__, __LINE__, "Command-line argument '%s' has no effect since timing analysis is disabled\n",
                                args.timing_driven_clustering.argument_name().c_str()); 
        }
        args.timing_driven_clustering.set(args.timing_analysis, Provenance::INFERRED);
    }

    /*
     * Placement
     */

    //Which placement algorithm to use?
    if (args.PlaceAlgorithm.provenance() != Provenance::SPECIFIED) {
        if(args.timing_analysis) {
            args.PlaceAlgorithm.set(PATH_TIMING_DRIVEN_PLACE, Provenance::INFERRED);
        } else {
            args.PlaceAlgorithm.set(BOUNDING_BOX_PLACE, Provenance::INFERRED);
        }
    }

    //Do we calculate timing info during placement?
    if (args.ShowPlaceTiming.provenance() != Provenance::SPECIFIED) {
        args.ShowPlaceTiming.set(args.timing_analysis, Provenance::INFERRED);
    }

    //Are we using the automatic, or user-specified annealing schedule?
    if (   args.PlaceInitT.provenance() == Provenance::SPECIFIED
        || args.PlaceExitT.provenance() == Provenance::SPECIFIED
        || args.PlaceAlphaT.provenance() == Provenance::SPECIFIED) {
        args.anneal_sched_type.set(USER_SCHED, Provenance::INFERRED);
    } else {
        args.anneal_sched_type.set(AUTO_SCHED, Provenance::INFERRED);
    }

    //Are the pad locations specified?
    if (std::string(args.pad_loc_file) == "free") {
        args.pad_loc_type.set(FREE, Provenance::INFERRED);

        args.pad_loc_file.set("", Provenance::SPECIFIED);
    } else if (std::string(args.pad_loc_file) == "random") {
        args.pad_loc_type.set(RANDOM, Provenance::INFERRED);

        args.pad_loc_file.set("", Provenance::SPECIFIED);
    } else {
        args.pad_loc_type.set(USER, Provenance::INFERRED);
        VTR_ASSERT(!args.pad_loc_file.value().empty());
    }

    /*
     * Routing
     */
    //Which routing algorithm to use?
    if (args.RouterAlgorithm.provenance() != Provenance::SPECIFIED) {
        if(args.timing_analysis && args.RouteType != GLOBAL) {
            args.RouterAlgorithm.set(TIMING_DRIVEN, Provenance::INFERRED);
        } else {
            args.RouterAlgorithm.set(NO_TIMING, Provenance::INFERRED);
        }
    }

    //Base cost type
    if (args.base_cost_type.provenance() != Provenance::SPECIFIED) {
        if (args.RouterAlgorithm == BREADTH_FIRST || args.RouterAlgorithm == NO_TIMING) {
            args.base_cost_type.set(DEMAND_ONLY, Provenance::INFERRED);
        } else {
            VTR_ASSERT(args.RouterAlgorithm == TIMING_DRIVEN);
            args.base_cost_type.set(DELAY_NORMALIZED, Provenance::INFERRED);
        }
    }

    //Bend cost
    if (args.bend_cost.provenance() != Provenance::SPECIFIED) {
        if (args.RouteType == GLOBAL) {
            args.bend_cost.set(1., Provenance::INFERRED);
        } else {
            VTR_ASSERT(args.RouteType == DETAILED);
            args.bend_cost.set(0., Provenance::INFERRED);
        }
    }
}

static bool verify_args(const t_options& args) {
    /*
     * Check for conflicting paramaters
     */
    if(args.dump_rr_structs_file.provenance() == Provenance::SPECIFIED
       && args.RouteChanWidth.provenance() != Provenance::SPECIFIED) {
		vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__,
				"--route_chan_width option must be specified if dumping rr structs is requested (%s)\n",
                args.dump_rr_structs_file.argument_name().c_str());
    }

    if(args.read_rr_graph_file.provenance() == Provenance::SPECIFIED
       && args.RouteChanWidth.provenance() != Provenance::SPECIFIED) {
		vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__,
				"--route_chan_width option must be specified if --read_rr_graph is requested (%s)\n",
                args.read_rr_graph_file.argument_name().c_str());
    }
    return true;
}
