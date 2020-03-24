#include "read_options.h"
#include "constant_nets.h"
#include "clock_modeling.h"
#include "vpr_error.h"

#include "argparse.hpp"

#include "vtr_memory.h"
#include "vtr_log.h"
#include "vtr_util.h"
#include "vtr_path.h"

using argparse::ConvertedValue;
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
    ConvertedValue<bool> from_str(std::string str) {
        ConvertedValue<bool> conv_value;
        if (str == "on")
            conv_value.set_value(true);
        else if (str == "off")
            conv_value.set_value(false);
        else {
            std::stringstream msg;
            msg << "Invalid conversion from '" << str << "' to boolean (expected one of: " << argparse::join(default_choices(), ", ") << ")";
            conv_value.set_error(msg.str());
        }
        return conv_value;
        ;
    }

    ConvertedValue<std::string> to_str(bool val) {
        ConvertedValue<std::string> conv_value;

        if (val)
            conv_value.set_value("on");
        else
            conv_value.set_value("off");

        return conv_value;
    }

    std::vector<std::string> default_choices() {
        return {"on", "off"};
    }
};

struct ParseCircuitFormat {
    ConvertedValue<e_circuit_format> from_str(std::string str) {
        ConvertedValue<e_circuit_format> conv_value;
        if (str == "auto")
            conv_value.set_value(e_circuit_format::AUTO);
        else if (str == "blif")
            conv_value.set_value(e_circuit_format::BLIF);
        else if (str == "eblif")
            conv_value.set_value(e_circuit_format::EBLIF);
        else {
            std::stringstream msg;
            msg << "Invalid conversion from '" << str << "' to e_circuit_format (expected one of: " << argparse::join(default_choices(), ", ") << ")";
            conv_value.set_error(msg.str());
        }
        return conv_value;
    }

    ConvertedValue<std::string> to_str(e_circuit_format val) {
        ConvertedValue<std::string> conv_value;

        if (val == e_circuit_format::AUTO)
            conv_value.set_value("auto");
        else if (val == e_circuit_format::BLIF)
            conv_value.set_value("blif");
        else {
            VTR_ASSERT(val == e_circuit_format::EBLIF);
            conv_value.set_value("eblif");
        }

        return conv_value;
    }

    std::vector<std::string> default_choices() {
        return {"auto", "blif", "eblif"};
    }
};
struct ParseRoutePredictor {
    ConvertedValue<e_routing_failure_predictor> from_str(std::string str) {
        ConvertedValue<e_routing_failure_predictor> conv_value;
        if (str == "safe")
            conv_value.set_value(SAFE);
        else if (str == "aggressive")
            conv_value.set_value(AGGRESSIVE);
        else if (str == "off")
            conv_value.set_value(OFF);
        else {
            std::stringstream msg;
            msg << "Invalid conversion from '" << str << "' to e_routing_failure_predictor (expected one of: " << argparse::join(default_choices(), ", ") << ")";
            conv_value.set_error(msg.str());
        }
        return conv_value;
    }

    ConvertedValue<std::string> to_str(e_routing_failure_predictor val) {
        ConvertedValue<std::string> conv_value;
        if (val == SAFE)
            conv_value.set_value("safe");
        else if (val == AGGRESSIVE)
            conv_value.set_value("aggressive");
        else {
            VTR_ASSERT(val == OFF);
            conv_value.set_value("off");
        }

        return conv_value;
    }

    std::vector<std::string> default_choices() {
        return {"safe", "aggressive", "off"};
    }
};

struct ParseRouterAlgorithm {
    ConvertedValue<e_router_algorithm> from_str(std::string str) {
        ConvertedValue<e_router_algorithm> conv_value;
        if (str == "breadth_first")
            conv_value.set_value(BREADTH_FIRST);
        else if (str == "timing_driven")
            conv_value.set_value(TIMING_DRIVEN);
        else {
            std::stringstream msg;
            msg << "Invalid conversion from '" << str << "' to e_router_algorithm (expected one of: " << argparse::join(default_choices(), ", ") << ")";
            conv_value.set_error(msg.str());
        }
        return conv_value;
    }

    ConvertedValue<std::string> to_str(e_router_algorithm val) {
        ConvertedValue<std::string> conv_value;
        if (val == BREADTH_FIRST)
            conv_value.set_value("breadth_first");
        else {
            VTR_ASSERT(val == TIMING_DRIVEN);
            conv_value.set_value("timing_driven");
        }
        return conv_value;
    }

    std::vector<std::string> default_choices() {
        return {"breadth_first", "timing_driven"};
    }
};

struct RouteBudgetsAlgorithm {
    ConvertedValue<e_routing_budgets_algorithm> from_str(std::string str) {
        ConvertedValue<e_routing_budgets_algorithm> conv_value;
        if (str == "minimax")
            conv_value.set_value(MINIMAX);
        else if (str == "scale_delay")
            conv_value.set_value(SCALE_DELAY);
        else if (str == "disable")
            conv_value.set_value(DISABLE);
        else {
            std::stringstream msg;
            msg << "Invalid conversion from '" << str << "' to e_routing_budget_algorithm (expected one of: " << argparse::join(default_choices(), ", ") << ")";
            conv_value.set_error(msg.str());
        }

        return conv_value;
    }

    ConvertedValue<std::string> to_str(e_routing_budgets_algorithm val) {
        ConvertedValue<std::string> conv_value;
        if (val == MINIMAX)
            conv_value.set_value("minimax");
        else if (val == DISABLE)
            conv_value.set_value("disable");
        else {
            VTR_ASSERT(val == SCALE_DELAY);
            conv_value.set_value("scale_delay");
        }
        return conv_value;
    }

    std::vector<std::string> default_choices() {
        return {"minimax", "scale_delay", "disable"};
    }
};

struct ParseRouteType {
    ConvertedValue<e_route_type> from_str(std::string str) {
        ConvertedValue<e_route_type> conv_value;
        if (str == "global")
            conv_value.set_value(GLOBAL);
        else if (str == "detailed")
            conv_value.set_value(DETAILED);
        else {
            std::stringstream msg;
            msg << "Invalid conversion from '" << str << "' to e_router_algorithm (expected one of: " << argparse::join(default_choices(), ", ") << ")";
            conv_value.set_error(msg.str());
        }
        return conv_value;
    }

    ConvertedValue<std::string> to_str(e_route_type val) {
        ConvertedValue<std::string> conv_value;
        if (val == GLOBAL)
            conv_value.set_value("global");
        else {
            VTR_ASSERT(val == DETAILED);
            conv_value.set_value("detailed");
        }
        return conv_value;
    }

    std::vector<std::string> default_choices() {
        return {"global", "detailed"};
    }
};

struct ParseBaseCost {
    ConvertedValue<e_base_cost_type> from_str(std::string str) {
        ConvertedValue<e_base_cost_type> conv_value;
        if (str == "delay_normalized")
            conv_value.set_value(DELAY_NORMALIZED);
        else if (str == "delay_normalized_length")
            conv_value.set_value(DELAY_NORMALIZED_LENGTH);
        else if (str == "delay_normalized_frequency")
            conv_value.set_value(DELAY_NORMALIZED_FREQUENCY);
        else if (str == "delay_normalized_length_frequency")
            conv_value.set_value(DELAY_NORMALIZED_LENGTH_FREQUENCY);
        else if (str == "demand_only_normalized_length")
            conv_value.set_value(DEMAND_ONLY_NORMALIZED_LENGTH);
        else if (str == "demand_only")
            conv_value.set_value(DEMAND_ONLY);
        else {
            std::stringstream msg;
            msg << "Invalid conversion from '" << str << "' to e_router_algorithm (expected one of: " << argparse::join(default_choices(), ", ") << ")";
            conv_value.set_error(msg.str());
        }
        return conv_value;
    }

    ConvertedValue<std::string> to_str(e_base_cost_type val) {
        ConvertedValue<std::string> conv_value;
        if (val == DELAY_NORMALIZED)
            conv_value.set_value("delay_normalized");
        else if (val == DELAY_NORMALIZED_LENGTH)
            conv_value.set_value("delay_normalized_length");
        else if (val == DELAY_NORMALIZED_FREQUENCY)
            conv_value.set_value("delay_normalized_frequency");
        else if (val == DELAY_NORMALIZED_LENGTH_FREQUENCY)
            conv_value.set_value("delay_normalized_length_frequency");
        else if (val == DEMAND_ONLY_NORMALIZED_LENGTH)
            conv_value.set_value("demand_only_normalized_length");
        else {
            VTR_ASSERT(val == DEMAND_ONLY);
            conv_value.set_value("demand_only");
        }
        return conv_value;
    }

    std::vector<std::string> default_choices() {
        return {"demand_only", "demand_only_normalized_length", "delay_normalized", "delay_normalized_length", "delay_normalized_frequency", "delay_normalized_length_frequency"};
    }
};

struct ParsePlaceAlgorithm {
    ConvertedValue<e_place_algorithm> from_str(std::string str) {
        ConvertedValue<e_place_algorithm> conv_value;
        if (str == "bounding_box")
            conv_value.set_value(BOUNDING_BOX_PLACE);
        else if (str == "path_timing_driven")
            conv_value.set_value(PATH_TIMING_DRIVEN_PLACE);
        else {
            std::stringstream msg;
            msg << "Invalid conversion from '" << str << "' to e_router_algorithm (expected one of: " << argparse::join(default_choices(), ", ") << ")";
            conv_value.set_error(msg.str());
        }
        return conv_value;
    }

    ConvertedValue<std::string> to_str(e_place_algorithm val) {
        ConvertedValue<std::string> conv_value;
        if (val == BOUNDING_BOX_PLACE)
            conv_value.set_value("bounding_box");
        else {
            VTR_ASSERT(val == PATH_TIMING_DRIVEN_PLACE);
            conv_value.set_value("path_timing_driven");
        }
        return conv_value;
    }

    std::vector<std::string> default_choices() {
        return {"bounding_box", "path_timing_driven"};
    }
};

struct ParseClusterSeed {
    ConvertedValue<e_cluster_seed> from_str(std::string str) {
        ConvertedValue<e_cluster_seed> conv_value;
        if (str == "timing")
            conv_value.set_value(e_cluster_seed::TIMING);
        else if (str == "max_inputs")
            conv_value.set_value(e_cluster_seed::MAX_INPUTS);
        else if (str == "blend")
            conv_value.set_value(e_cluster_seed::BLEND);
        else if (str == "max_pins")
            conv_value.set_value(e_cluster_seed::MAX_PINS);
        else if (str == "max_input_pins")
            conv_value.set_value(e_cluster_seed::MAX_INPUT_PINS);
        else if (str == "blend2")
            conv_value.set_value(e_cluster_seed::BLEND2);
        else {
            std::stringstream msg;
            msg << "Invalid conversion from '" << str << "' to e_router_algorithm (expected one of: " << argparse::join(default_choices(), ", ") << ")";
            conv_value.set_error(msg.str());
        }
        return conv_value;
    }

    ConvertedValue<std::string> to_str(e_cluster_seed val) {
        ConvertedValue<std::string> conv_value;
        if (val == e_cluster_seed::TIMING)
            conv_value.set_value("timing");
        else if (val == e_cluster_seed::MAX_INPUTS)
            conv_value.set_value("max_inputs");
        else if (val == e_cluster_seed::BLEND)
            conv_value.set_value("blend");
        else if (val == e_cluster_seed::MAX_PINS)
            conv_value.set_value("max_pins");
        else if (val == e_cluster_seed::MAX_INPUT_PINS)
            conv_value.set_value("max_input_pins");
        else {
            VTR_ASSERT(val == e_cluster_seed::BLEND2);
            conv_value.set_value("blend2");
        }
        return conv_value;
    }

    std::vector<std::string> default_choices() {
        return {"timing", "max_inputs", "blend", "max_pins", "max_input_pins", "blend2"};
    }
};

struct ParseConstantNetMethod {
    ConvertedValue<e_constant_net_method> from_str(std::string str) {
        ConvertedValue<e_constant_net_method> conv_value;
        if (str == "global")
            conv_value.set_value(CONSTANT_NET_GLOBAL);
        else if (str == "route")
            conv_value.set_value(CONSTANT_NET_ROUTE);
        else {
            std::stringstream msg;
            msg << "Invalid conversion from '" << str << "' to e_constant_net_method (expected one of: " << argparse::join(default_choices(), ", ") << ")";
            conv_value.set_error(msg.str());
        }
        return conv_value;
    }

    ConvertedValue<std::string> to_str(e_constant_net_method val) {
        ConvertedValue<std::string> conv_value;
        if (val == CONSTANT_NET_GLOBAL)
            conv_value.set_value("global");
        else {
            VTR_ASSERT(val == CONSTANT_NET_ROUTE);
            conv_value.set_value("route");
        }
        return conv_value;
    }

    std::vector<std::string> default_choices() {
        return {"global", "route"};
    }
};

struct ParseTimingReportDetail {
    ConvertedValue<e_timing_report_detail> from_str(std::string str) {
        ConvertedValue<e_timing_report_detail> conv_value;
        if (str == "netlist")
            conv_value.set_value(e_timing_report_detail::NETLIST);
        else if (str == "aggregated")
            conv_value.set_value(e_timing_report_detail::AGGREGATED);
        else {
            std::stringstream msg;
            msg << "Invalid conversion from '" << str << "' to e_timing_report_detail (expected one of: " << argparse::join(default_choices(), ", ") << ")";
            conv_value.set_error(msg.str());
        }
        return conv_value;
    }

    ConvertedValue<std::string> to_str(e_timing_report_detail val) {
        ConvertedValue<std::string> conv_value;
        if (val == e_timing_report_detail::NETLIST)
            conv_value.set_value("netlist");
        else {
            VTR_ASSERT(val == e_timing_report_detail::AGGREGATED);
            conv_value.set_value("aggregated");
        }
        return conv_value;
    }

    std::vector<std::string> default_choices() {
        return {"netlist", "aggregated"};
    }
};

struct ParseClockModeling {
    ConvertedValue<e_clock_modeling> from_str(std::string str) {
        ConvertedValue<e_clock_modeling> conv_value;
        if (str == "ideal")
            conv_value.set_value(IDEAL_CLOCK);
        else if (str == "route")
            conv_value.set_value(ROUTED_CLOCK);
        else if (str == "dedicated_network")
            conv_value.set_value(DEDICATED_NETWORK);
        else {
            std::stringstream msg;
            msg << "Invalid conversion from '"
                << str
                << "' to e_clock_modeling (expected one of: "
                << argparse::join(default_choices(), ", ") << ")";
            conv_value.set_error(msg.str());
        }
        return conv_value;
    }

    ConvertedValue<std::string> to_str(e_clock_modeling val) {
        ConvertedValue<std::string> conv_value;
        if (val == IDEAL_CLOCK)
            conv_value.set_value("ideal");
        else if (val == ROUTED_CLOCK)
            conv_value.set_value("route");
        else {
            VTR_ASSERT(val == DEDICATED_NETWORK);
            conv_value.set_value("dedicated_network");
        }
        return conv_value;
    }

    std::vector<std::string> default_choices() {
        return {"ideal", "route", "dedicated_network"};
    }
};

struct ParseUnrelatedClustering {
    ConvertedValue<e_unrelated_clustering> from_str(std::string str) {
        ConvertedValue<e_unrelated_clustering> conv_value;
        if (str == "on")
            conv_value.set_value(e_unrelated_clustering::ON);
        else if (str == "off")
            conv_value.set_value(e_unrelated_clustering::OFF);
        else if (str == "auto")
            conv_value.set_value(e_unrelated_clustering::AUTO);
        else {
            std::stringstream msg;
            msg << "Invalid conversion from '"
                << str
                << "' to e_unrelated_clustering (expected one of: "
                << argparse::join(default_choices(), ", ") << ")";
            conv_value.set_error(msg.str());
        }
        return conv_value;
    }

    ConvertedValue<std::string> to_str(e_unrelated_clustering val) {
        ConvertedValue<std::string> conv_value;
        if (val == e_unrelated_clustering::ON)
            conv_value.set_value("on");
        else if (val == e_unrelated_clustering::OFF)
            conv_value.set_value("off");
        else {
            VTR_ASSERT(val == e_unrelated_clustering::AUTO);
            conv_value.set_value("auto");
        }
        return conv_value;
    }

    std::vector<std::string> default_choices() {
        return {"on", "off", "auto"};
    }
};

struct ParseBalanceBlockTypeUtil {
    ConvertedValue<e_balance_block_type_util> from_str(std::string str) {
        ConvertedValue<e_balance_block_type_util> conv_value;
        if (str == "on")
            conv_value.set_value(e_balance_block_type_util::ON);
        else if (str == "off")
            conv_value.set_value(e_balance_block_type_util::OFF);
        else if (str == "auto")
            conv_value.set_value(e_balance_block_type_util::AUTO);
        else {
            std::stringstream msg;
            msg << "Invalid conversion from '"
                << str
                << "' to e_balance_block_type_util (expected one of: "
                << argparse::join(default_choices(), ", ") << ")";
            conv_value.set_error(msg.str());
        }
        return conv_value;
    }

    ConvertedValue<std::string> to_str(e_balance_block_type_util val) {
        ConvertedValue<std::string> conv_value;
        if (val == e_balance_block_type_util::ON)
            conv_value.set_value("on");
        else if (val == e_balance_block_type_util::OFF)
            conv_value.set_value("off");
        else {
            VTR_ASSERT(val == e_balance_block_type_util::AUTO);
            conv_value.set_value("auto");
        }
        return conv_value;
    }

    std::vector<std::string> default_choices() {
        return {"on", "off", "auto"};
    }
};

struct ParseConstGenInference {
    ConvertedValue<e_const_gen_inference> from_str(std::string str) {
        ConvertedValue<e_const_gen_inference> conv_value;
        if (str == "none")
            conv_value.set_value(e_const_gen_inference::NONE);
        else if (str == "comb")
            conv_value.set_value(e_const_gen_inference::COMB);
        else if (str == "comb_seq")
            conv_value.set_value(e_const_gen_inference::COMB_SEQ);
        else {
            std::stringstream msg;
            msg << "Invalid conversion from '"
                << str
                << "' to e_const_gen_inference (expected one of: "
                << argparse::join(default_choices(), ", ") << ")";
            conv_value.set_error(msg.str());
        }
        return conv_value;
    }

    ConvertedValue<std::string> to_str(e_const_gen_inference val) {
        ConvertedValue<std::string> conv_value;
        if (val == e_const_gen_inference::NONE)
            conv_value.set_value("none");
        else if (val == e_const_gen_inference::COMB)
            conv_value.set_value("comb");
        else {
            VTR_ASSERT(val == e_const_gen_inference::COMB_SEQ);
            conv_value.set_value("comb_seq");
        }
        return conv_value;
    }

    std::vector<std::string> default_choices() {
        return {"none", "comb", "comb_seq"};
    }
};

struct ParseIncrRerouteDelayRipup {
    ConvertedValue<e_incr_reroute_delay_ripup> from_str(std::string str) {
        ConvertedValue<e_incr_reroute_delay_ripup> conv_value;
        if (str == "on")
            conv_value.set_value(e_incr_reroute_delay_ripup::ON);
        else if (str == "off")
            conv_value.set_value(e_incr_reroute_delay_ripup::OFF);
        else if (str == "auto")
            conv_value.set_value(e_incr_reroute_delay_ripup::AUTO);
        else {
            std::stringstream msg;
            msg << "Invalid conversion from '"
                << str
                << "' to e_incr_reroute_delay_ripup (expected one of: "
                << argparse::join(default_choices(), ", ") << ")";
            conv_value.set_error(msg.str());
        }
        return conv_value;
    }

    ConvertedValue<std::string> to_str(e_incr_reroute_delay_ripup val) {
        ConvertedValue<std::string> conv_value;
        if (val == e_incr_reroute_delay_ripup::ON)
            conv_value.set_value("on");
        else if (val == e_incr_reroute_delay_ripup::OFF)
            conv_value.set_value("off");
        else {
            VTR_ASSERT(val == e_incr_reroute_delay_ripup::AUTO);
            conv_value.set_value("auto");
        }
        return conv_value;
    }

    std::vector<std::string> default_choices() {
        return {"on", "off", "auto"};
    }
};

struct ParseRouteBBUpdate {
    ConvertedValue<e_route_bb_update> from_str(std::string str) {
        ConvertedValue<e_route_bb_update> conv_value;
        if (str == "static")
            conv_value.set_value(e_route_bb_update::STATIC);
        else if (str == "dynamic")
            conv_value.set_value(e_route_bb_update::DYNAMIC);
        else {
            std::stringstream msg;
            msg << "Invalid conversion from '"
                << str
                << "' to e_route_bb_update (expected one of: "
                << argparse::join(default_choices(), ", ") << ")";
            conv_value.set_error(msg.str());
        }
        return conv_value;
    }

    ConvertedValue<std::string> to_str(e_route_bb_update val) {
        ConvertedValue<std::string> conv_value;
        if (val == e_route_bb_update::STATIC)
            conv_value.set_value("static");
        else {
            VTR_ASSERT(val == e_route_bb_update::DYNAMIC);
            conv_value.set_value("dynamic");
        }
        return conv_value;
    }

    std::vector<std::string> default_choices() {
        return {"static", "dynamic"};
    }
};

struct ParseRouterLookahead {
    ConvertedValue<e_router_lookahead> from_str(std::string str) {
        ConvertedValue<e_router_lookahead> conv_value;
        if (str == "classic")
            conv_value.set_value(e_router_lookahead::CLASSIC);
        else if (str == "map")
            conv_value.set_value(e_router_lookahead::MAP);
        else {
            std::stringstream msg;
            msg << "Invalid conversion from '"
                << str
                << "' to e_router_lookahead (expected one of: "
                << argparse::join(default_choices(), ", ") << ")";
            conv_value.set_error(msg.str());
        }
        return conv_value;
    }

    ConvertedValue<std::string> to_str(e_router_lookahead val) {
        ConvertedValue<std::string> conv_value;
        if (val == e_router_lookahead::CLASSIC)
            conv_value.set_value("classic");
        else {
            VTR_ASSERT(val == e_router_lookahead::MAP);
            conv_value.set_value("map");
        }
        return conv_value;
    }

    std::vector<std::string> default_choices() {
        return {"classic", "map"};
    }
};

struct ParsePlaceDelayModel {
    ConvertedValue<PlaceDelayModelType> from_str(std::string str) {
        ConvertedValue<PlaceDelayModelType> conv_value;
        if (str == "delta")
            conv_value.set_value(PlaceDelayModelType::DELTA);
        else if (str == "delta_override")
            conv_value.set_value(PlaceDelayModelType::DELTA_OVERRIDE);
        else {
            std::stringstream msg;
            msg << "Invalid conversion from '" << str << "' to PlaceDelayModelType (expected one of: " << argparse::join(default_choices(), ", ") << ")";
            conv_value.set_error(msg.str());
        }
        return conv_value;
    }

    ConvertedValue<std::string> to_str(PlaceDelayModelType val) {
        ConvertedValue<std::string> conv_value;
        if (val == PlaceDelayModelType::DELTA)
            conv_value.set_value("delta");
        else if (val == PlaceDelayModelType::DELTA_OVERRIDE)
            conv_value.set_value("delta_override");
        else {
            std::stringstream msg;
            msg << "Unrecognized PlaceDelayModelType";
            conv_value.set_error(msg.str());
        }
        return conv_value;
    }

    std::vector<std::string> default_choices() {
        return {"delta", "delta_override"};
    }
};

struct ParseReducer {
    ConvertedValue<e_reducer> from_str(std::string str) {
        ConvertedValue<e_reducer> conv_value;
        if (str == "min")
            conv_value.set_value(e_reducer::MIN);
        else if (str == "max")
            conv_value.set_value(e_reducer::MAX);
        else if (str == "median")
            conv_value.set_value(e_reducer::MEDIAN);
        else if (str == "arithmean")
            conv_value.set_value(e_reducer::ARITHMEAN);
        else if (str == "geomean")
            conv_value.set_value(e_reducer::GEOMEAN);
        else {
            std::stringstream msg;
            msg << "Invalid conversion from '" << str << "' to e_reducer (expected one of: " << argparse::join(default_choices(), ", ") << ")";
            conv_value.set_error(msg.str());
        }
        return conv_value;
    }

    ConvertedValue<std::string> to_str(e_reducer val) {
        ConvertedValue<std::string> conv_value;
        if (val == e_reducer::MIN)
            conv_value.set_value("min");
        else if (val == e_reducer::MAX)
            conv_value.set_value("max");
        else if (val == e_reducer::MEDIAN)
            conv_value.set_value("median");
        else if (val == e_reducer::ARITHMEAN)
            conv_value.set_value("arithmean");
        else {
            VTR_ASSERT(val == e_reducer::GEOMEAN);
            conv_value.set_value("geomean");
        }
        return conv_value;
    }

    std::vector<std::string> default_choices() {
        return {"min", "max", "median", "arithmean", "geomean"};
    }
};

static argparse::ArgumentParser create_arg_parser(std::string prog_name, t_options& args) {
    std::string description =
        "Implements the specified circuit onto the target FPGA architecture"
        " by performing packing/placement/routing, and analyzes the result.\n"
        "\n"
        "Attempts to find the minimum routable channel width, unless a fixed"
        " channel width is specified with --route_chan_width.";
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
        .help("Circuit file (or circuit name if --circuit_file specified)");

    auto& stage_grp = parser.add_argument_group("stage options");

    stage_grp.add_argument<bool, ParseOnOff>(args.do_packing, "--pack")
        .help("Run packing")
        .action(argparse::Action::STORE_TRUE)
        .default_value("off");

    stage_grp.add_argument<bool, ParseOnOff>(args.do_placement, "--place")
        .help("Run placement")
        .action(argparse::Action::STORE_TRUE)
        .default_value("off");

    stage_grp.add_argument<bool, ParseOnOff>(args.do_routing, "--route")
        .help("Run routing")
        .action(argparse::Action::STORE_TRUE)
        .default_value("off");

    stage_grp.add_argument<bool, ParseOnOff>(args.do_analysis, "--analysis")
        .help("Run analysis")
        .action(argparse::Action::STORE_TRUE)
        .default_value("off");

    stage_grp.epilog(
        "If none of the stage options are specified, all stages are run.\n"
        "Analysis is always run after routing, unless the implementation\n"
        "is illegal.\n"
        "\n"
        "If the implementation is illegal analysis can be forced by explicitly\n"
        "specifying the --analysis option.");

    auto& gfx_grp = parser.add_argument_group("graphics options");

    gfx_grp.add_argument<bool, ParseOnOff>(args.show_graphics, "--disp")
        .help("Enable or disable interactive graphics")
        .default_value("off");

    gfx_grp.add_argument(args.GraphPause, "--auto")
        .help(
            "Controls how often VPR pauses for interactive"
            " graphics (requiring Proceed to be clicked)."
            " Higher values pause less frequently")
        .default_value("1")
        .choices({"0", "1", "2"})
        .show_in(argparse::ShowIn::HELP_ONLY);

    auto& gen_grp = parser.add_argument_group("general options");

    gen_grp.add_argument(args.show_help, "--help", "-h")
        .help("Show this help message then exit")
        .action(argparse::Action::HELP);

    gen_grp.add_argument<bool, ParseOnOff>(args.show_version, "--version")
        .help("Show version information then exit")
        .action(argparse::Action::VERSION);

    gen_grp.add_argument<std::string>(args.device_layout, "--device")
        .help(
            "Controls which device layout/floorplan is used from the architecture file."
            " 'auto' uses the smallest device which satisfies the circuit's resource requirements.")
        .metavar("DEVICE_NAME")
        .default_value("auto");

    gen_grp.add_argument<size_t>(args.num_workers, "--num_workers", "-j")
        .help(
            "Controls how many parallel workers VPR may use:\n"
            " *  1 implies VPR will execute serially,\n"
            " * >1 implies VPR may execute in parallel with up to the\n"
            "      specified concurrency, and\n"
            " *  0 implies VPR may execute in parallel with up to the\n"
            "      maximum concurrency supported by the host machine.\n"
            "If this option is not specified it may be set from the 'VPR_NUM_WORKERS' "
            "environment variable; otherwise the default is used.")
        .default_value("1");

    gen_grp.add_argument<bool, ParseOnOff>(args.timing_analysis, "--timing_analysis")
        .help("Controls whether timing analysis (and timing driven optimizations) are enabled.")
        .default_value("on");

    gen_grp.add_argument<bool, ParseOnOff>(args.CreateEchoFile, "--echo_file")
        .help(
            "Generate echo files of key internal data structures."
            " Useful for debugging VPR, and typically end in .echo")
        .default_value("off")
        .show_in(argparse::ShowIn::HELP_ONLY);

    gen_grp.add_argument<bool, ParseOnOff>(args.verify_file_digests, "--verify_file_digests")
        .help(
            "Verify that files loaded by VPR (e.g. architecture, netlist,"
            " previous packing/placement/routing) are consistent")
        .default_value("on")
        .show_in(argparse::ShowIn::HELP_ONLY);

    gen_grp.add_argument(args.target_device_utilization, "--target_utilization")
        .help(
            "Sets the target device utilization."
            " This corresponds to the maximum target fraction of device grid-tiles to be used."
            " A value of 1.0 means the smallest device (which fits the circuit) will be used.")
        .default_value("1.0")
        .show_in(argparse::ShowIn::HELP_ONLY);

    gen_grp.add_argument<e_constant_net_method, ParseConstantNetMethod>(args.constant_net_method, "--constant_net_method")
        .help(
            "Specifies how constant nets (i.e. those driven to a constant\n"
            "value) are handled:\n"
            " * global: Treat constant nets as globals (not routed)\n"
            " * route : Treat constant nets as normal nets (routed)\n"
            " * dedicated_network : Build a dedicated clock network based on the\n"
            "                       clock network specified in the architecture file\n")
        .default_value("global")
        .show_in(argparse::ShowIn::HELP_ONLY);

    gen_grp.add_argument<e_clock_modeling, ParseClockModeling>(args.clock_modeling, "--clock_modeling")
        .help(
            "Specifies how clock nets are handled\n"
            " * ideal: Treat clock pins as ideal\n"
            "          (i.e. no routing delays on clocks)\n"
            " * route: Treat the clock pins as normal nets\n"
            "          (i.e. routed using inter-block routing)\n")
        .default_value("ideal")
        .show_in(argparse::ShowIn::HELP_ONLY);

    gen_grp.add_argument<bool, ParseOnOff>(args.exit_before_pack, "--exit_before_pack")
        .help("Causes VPR to exit before packing starts (useful for statistics collection)")
        .default_value("off")
        .show_in(argparse::ShowIn::HELP_ONLY);

    gen_grp.add_argument<bool, ParseOnOff>(args.strict_checks, "--strict_checks")
        .help(
            "Controls whether VPR enforces some consistency checks strictly (as errors) or treats them as warnings."
            " Usually these checks indicate an issue with either the targetted architecture, or consistency issues"
            " with VPR's internal data structures/algorithms (possibly harming optimization quality)."
            " In specific circumstances on specific architectures these checks may be too restrictive and can be turned off."
            " However exercise extreme caution when turning this option off -- be sure you completely understand why the issue"
            " is being flagged, and why it is OK to treat as a warning instead of an error.")
        .default_value("on")
        .show_in(argparse::ShowIn::HELP_ONLY);

    auto& file_grp = parser.add_argument_group("file options");

    file_grp.add_argument(args.BlifFile, "--circuit_file")
        .help("Path to technology mapped circuit")
        .show_in(argparse::ShowIn::HELP_ONLY);

    file_grp.add_argument<e_circuit_format, ParseCircuitFormat>(args.circuit_format, "--circuit_format")
        .help(
            "File format for the input atom-level circuit/netlist.\n"
            " * auto: infer from file extension\n"
            " * blif: Strict structural BLIF format\n"
            " * eblif: Structural BLIF format with the extensions:\n"
            "           .conn  - Connection between two wires\n"
            "           .cname - Custom name for atom primitive\n"
            "           .param - Parameter on atom primitive\n"
            "           .attr  - Attribute on atom primitive\n")
        .default_value("auto")
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
        .help(
            "The routing resource graph file to load."
            " The loaded routing resource graph overrides any routing architecture specified in the architecture file.")
        .metavar("RR_GRAPH_FILE")
        .show_in(argparse::ShowIn::HELP_ONLY);

    file_grp.add_argument(args.write_rr_graph_file, "--write_rr_graph")
        .help("Writes the routing resource graph to the specified file")
        .metavar("RR_GRAPH_FILE")
        .show_in(argparse::ShowIn::HELP_ONLY);

    file_grp.add_argument(args.out_file_prefix, "--outfile_prefix")
        .help("Prefix for output files")
        .show_in(argparse::ShowIn::HELP_ONLY);

    auto& netlist_grp = parser.add_argument_group("netlist options");

    netlist_grp.add_argument<bool, ParseOnOff>(args.absorb_buffer_luts, "--absorb_buffer_luts")
        .help("Controls whether LUTS programmed as buffers are absorbed by downstream logic")
        .default_value("on")
        .show_in(argparse::ShowIn::HELP_ONLY);

    netlist_grp.add_argument<e_const_gen_inference, ParseConstGenInference>(args.const_gen_inference, "--const_gen_inference")
        .help(
            "Controls how constant generators are detected\n"
            " * none    : No constant generator inference is performed\n"
            " * comb    : Only combinational primitives are considered\n"
            "             for constant generator inference (always safe)\n"
            " * comb_seq: Both combinational and sequential primitives\n"
            "             are considered for constant generator inference\n"
            "             (usually safe)\n")
        .default_value("comb_seq")
        .show_in(argparse::ShowIn::HELP_ONLY);

    netlist_grp.add_argument<bool, ParseOnOff>(args.sweep_dangling_primary_ios, "--sweep_dangling_primary_ios")
        .help("Controls whether dangling primary inputs and outputs are removed from the netlist")
        .default_value("on")
        .show_in(argparse::ShowIn::HELP_ONLY);

    netlist_grp.add_argument<bool, ParseOnOff>(args.sweep_dangling_nets, "--sweep_dangling_nets")
        .help("Controls whether dangling nets are removed from the netlist")
        .default_value("on")
        .show_in(argparse::ShowIn::HELP_ONLY);

    netlist_grp.add_argument<bool, ParseOnOff>(args.sweep_dangling_blocks, "--sweep_dangling_blocks")
        .help("Controls whether dangling blocks are removed from the netlist")
        .default_value("on")
        .show_in(argparse::ShowIn::HELP_ONLY);

    netlist_grp.add_argument<bool, ParseOnOff>(args.sweep_constant_primary_outputs, "--sweep_constant_primary_outputs")
        .help("Controls whether primary outputs driven by constant values are removed from the netlist")
        .default_value("off")
        .show_in(argparse::ShowIn::HELP_ONLY);

    netlist_grp.add_argument(args.netlist_verbosity, "--netlist_verbosity")
        .help(
            "Controls how much detail netlist processing produces about detected netlist"
            " characteristics (e.g. constant generator detection) and applied netlist"
            " modifications (e.g. swept netlist components)."
            " Larger values produce more detail.")
        .default_value("1")
        .show_in(argparse::ShowIn::HELP_ONLY);

    auto& pack_grp = parser.add_argument_group("packing options");

    pack_grp.add_argument<bool, ParseOnOff>(args.connection_driven_clustering, "--connection_driven_clustering")
        .help(
            "Controls whether or not packing prioritizes the absorption of nets with fewer"
            " connections into a complex logic block over nets with more connections")
        .default_value("on")
        .show_in(argparse::ShowIn::HELP_ONLY);

    pack_grp.add_argument<e_unrelated_clustering, ParseUnrelatedClustering>(args.allow_unrelated_clustering, "--allow_unrelated_clustering")
        .help(
            "Controls whether primitives with no attraction to a cluster can be packed into it.\n"
            "Turning unrelated clustering on can increase packing density (fewer blocks are used), but at the cost of worse routability.\n"
            " * on  : Unrelated clustering enabled\n"
            " * off : Unrelated clustering disabled\n"
            " * auto: Dynamically enabled/disabled (based on density)\n")
        .default_value("auto")
        .show_in(argparse::ShowIn::HELP_ONLY);

    pack_grp.add_argument(args.alpha_clustering, "--alpha_clustering")
        .help(
            "Parameter that weights the optimization of timing vs area. 0.0 focuses solely on"
            " area, 1.0 solely on timing.")
        .default_value("0.75")
        .show_in(argparse::ShowIn::HELP_ONLY);

    pack_grp.add_argument(args.beta_clustering, "--beta_clustering")
        .help(
            "Parameter that weights the absorption of small nets vs signal sharing."
            " 0.0 focuses solely on sharing, 1.0 solely on small net absoprtion."
            " Only meaningful if --connection_driven_clustering=on")
        .default_value("0.9")
        .show_in(argparse::ShowIn::HELP_ONLY);

    pack_grp.add_argument<bool, ParseOnOff>(args.timing_driven_clustering, "--timing_driven_clustering")
        .help("Controls whether custering optimizes for timing")
        .default_value("on")
        .show_in(argparse::ShowIn::HELP_ONLY);

    pack_grp.add_argument<e_cluster_seed, ParseClusterSeed>(args.cluster_seed_type, "--cluster_seed_type")
        .help(
            "Controls how primitives are chosen as seeds."
            " (Default: blend2 if timing driven, max_inputs otherwise)")
        .show_in(argparse::ShowIn::HELP_ONLY);

    pack_grp.add_argument<bool, ParseOnOff>(args.enable_clustering_pin_feasibility_filter, "--clustering_pin_feasibility_filter")
        .help(
            "Controls whether the pin counting feasibility filter is used during clustering."
            " When enabled the clustering engine counts the number of available pins in"
            " groups/classes of mutually connected pins within a cluster."
            " These counts are used to quickly filter out candidate primitives/atoms/molecules"
            " for which the cluster has insufficient pins to route (without performing a full routing)."
            " This reduces packer run-time")
        .default_value("on")
        .show_in(argparse::ShowIn::HELP_ONLY);

    pack_grp.add_argument<e_balance_block_type_util, ParseBalanceBlockTypeUtil>(args.balance_block_type_utilization, "--balance_block_type_utilization")
        .help(
            "If enabled, when a primitive can potentially be mapped to multiple block types the packer will\n"
            "pick the block type which (currently) has the lowest utilization.\n"
            " * on  : Try to balance block type utilization\n"
            " * off : Do not try to balance block type utilization\n"
            " * auto: Dynamically enabled/disabled (based on density)\n")
        .default_value("auto")
        .show_in(argparse::ShowIn::HELP_ONLY);

    pack_grp.add_argument(args.target_external_pin_util, "--target_ext_pin_util")
        .help(
            "Sets the external pin utilization target during clustering.\n"
            "Value Ranges: [1.0, 0.0]\n"
            "* 1.0 : The packer to pack as densely as possible (i.e. try\n"
            "        to use 100% of cluster external pins)\n"
            "* 0.0 : The packer to pack as loosely as possible (i.e. each\n"
            "        block will contain a single mollecule).\n"
            "        Values in between trade-off pin usage and\n"
            "        packing density.\n"
            "\n"
            "Typically packing less densely improves routability, at\n"
            "the cost of using more clusters. Note that these settings are\n"
            "only guidelines, the packer will use up to 1.0 utilization if\n"
            "a molecule would not otherwise pack into any cluster type.\n"
            "\n"
            "This option can take multiple specifications in several\n"
            "formats:\n"
            "* auto (i.e. 'auto'): VPR will determine the target pin\n"
            "                      utilizations automatically\n"
            "* Single Value (e.g. '0.7'): the input pin utilization for\n"
            "                             all block types (output pin\n"
            "                             utilization defaults to 1.0)\n"
            "* Double Value (e.g. '0.7,0.8'): the input and output pin\n"
            "                             utilization for all block types\n"
            "* Block Value (e.g. 'clb:0.7', 'clb:0.7,0.8'): the pin\n"
            "                             utilization for a specific\n"
            "                             block type\n"
            "These can be used in combination. For example:\n"
            "   '--target_ext_pin_util 0.9 clb:0.7'\n"
            "would set the input pin utilization of clb blocks to 0.7,\n"
            "and all other blocks to 0.9.\n")
        .nargs('+')
        .default_value({"auto"})
        .show_in(argparse::ShowIn::HELP_ONLY);

    pack_grp.add_argument<bool, ParseOnOff>(args.pack_prioritize_transitive_connectivity, "--pack_prioritize_transitive_connectivity")
        .help("Whether transitive connectivity is prioritized over high-fanout connectivity during packing")
        .default_value("on")
        .show_in(argparse::ShowIn::HELP_ONLY);

    pack_grp.add_argument(args.pack_high_fanout_threshold, "--pack_high_fanout_threshold")
        .help(
            "Sets the high fanout threshold during clustering.\n"
            "\n"
            "Typically reducing the threshold reduces packing density\n"
            "and improves routability."
            "\n"
            "This option can take multiple specifications in several\n"
            "formats:\n"
            "* auto (i.e. 'auto'): VPR will determine the target pin\n"
            "                      utilizations automatically\n"
            "* Single Value (e.g. '256'): the high fanout threshold\n"
            "                             for all block types\n"
            "* Block Value (e.g. 'clb:16'): the high fanout threshold\n"
            "                               for a specific block type\n"
            "These can be used in combination. For example:\n"
            "   '--pack_high_fanout_threshold 256 clb:16'\n"
            "would set the high fanout threshold for clb blocks to 16\n"
            "and all other blocks to 256\n")
        .nargs('+')
        .default_value({"auto"})
        .show_in(argparse::ShowIn::HELP_ONLY);

    pack_grp.add_argument(args.pack_transitive_fanout_threshold, "--pack_transitive_fanout_threshold")
        .help("Packer transitive fanout threshold")
        .default_value("4")
        .show_in(argparse::ShowIn::HELP_ONLY);

    pack_grp.add_argument<int>(args.pack_verbosity, "--pack_verbosity")
        .help("Controls how verbose clustering's output is. Higher values produce more output (useful for debugging architecture packing problems)")
        .default_value("2")
        .show_in(argparse::ShowIn::HELP_ONLY);

    auto& place_grp = parser.add_argument_group("placement options");

    place_grp.add_argument(args.Seed, "--seed")
        .help("Placement random number generator seed")
        .default_value("1")
        .show_in(argparse::ShowIn::HELP_ONLY);

    place_grp.add_argument<bool, ParseOnOff>(args.ShowPlaceTiming, "--enable_timing_computations")
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
        .help(
            "Temperature scaling factor for manual annealing schedule."
            " Old temperature is multiplied by alpha_t")
        .default_value("0.8")
        .show_in(argparse::ShowIn::HELP_ONLY);

    place_grp.add_argument(args.pad_loc_file, "--fix_pins")
        .help(
            "Fixes I/O pad locations during placement. Valid options:\n"
            " * 'free' allows placement to optimize pad locations\n"
            " * 'random' fixes pad locations to arbitraray locations\n"
            " * path to a file specifying pad locations (.place format with only pads specified).")
        .default_value("free")
        .show_in(argparse::ShowIn::HELP_ONLY);

    place_grp.add_argument<e_place_algorithm, ParsePlaceAlgorithm>(args.PlaceAlgorithm, "--place_algorithm")
        .help("Controls which placement algorithm is used")
        .default_value("path_timing_driven")
        .choices({"bounding_box", "path_timing_driven"})
        .show_in(argparse::ShowIn::HELP_ONLY);

    place_grp.add_argument(args.PlaceChanWidth, "--place_chan_width")
        .help(
            "Sets the assumed channel width during placement. "
            "If --place_chan_width is unspecified, but --route_chan_width is specified the "
            "--route_chan_width value will be used (otherwise the default value is used).")
        .default_value("100")
        .show_in(argparse::ShowIn::HELP_ONLY);

    place_grp.add_argument(args.place_rlim_escape_fraction, "--place_rlim_escape")
        .help(
            "The fraction of moves which are allowed to ignore the region limit."
            " For example, a value of 0.1 means 10%% of moves are allowed to ignore the region limit.")
        .default_value("0.0")
        .show_in(argparse::ShowIn::HELP_ONLY);

    auto& place_timing_grp = parser.add_argument_group("timing-driven placement options");

    place_timing_grp.add_argument(args.PlaceTimingTradeoff, "--timing_tradeoff")
        .help(
            "Trade-off control between delay and wirelength during placement."
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
        .help(
            "Controls how critical a connection is as a function of slack at the start of placement."
            " A value of zero treats all connections as equally critical (regardless of slack)."
            " Values larger than 1.0 cause low slack connections to be treated more critically."
            " The value increases to --td_place_exp_last during placement.")
        .default_value("1.0")
        .show_in(argparse::ShowIn::HELP_ONLY);

    place_timing_grp.add_argument(args.place_exp_last, "--td_place_exp_last")
        .help("Controls how critical a connection is as a function of slack at the end of placement.")
        .default_value("8.0")
        .show_in(argparse::ShowIn::HELP_ONLY);

    place_timing_grp.add_argument<PlaceDelayModelType, ParsePlaceDelayModel>(args.place_delay_model, "--place_delay_model")
        .help(
            "This option controls what information is considered and how"
            " the placement delay model is constructed.\n"
            "Valid options:\n"
            " * 'delta' uses differences in position only\n"
            " * 'delta_override' uses differences in position with overrides for direct connects\n")
        .default_value("delta")
        .show_in(argparse::ShowIn::HELP_ONLY);

    place_timing_grp.add_argument<e_reducer, ParseReducer>(args.place_delay_model_reducer, "--place_delay_model_reducer")
        .help("When calculating delta delays for the placment delay model how are multiple values combined?")
        .default_value("min")
        .show_in(argparse::ShowIn::HELP_ONLY);

    place_timing_grp.add_argument(args.place_delay_offset, "--place_delay_offset")
        .help(
            "A constant offset (in seconds) applied to the placer's delay model.")
        .default_value("0.0")
        .show_in(argparse::ShowIn::HELP_ONLY);

    place_timing_grp.add_argument(args.place_delay_ramp_delta_threshold, "--place_delay_ramp_delta_threshold")
        .help(
            "The delta distance beyond which --place_delay_ramp is applied."
            " Negative values disable the placer delay ramp.")
        .default_value("-1")
        .show_in(argparse::ShowIn::HELP_ONLY);

    place_timing_grp.add_argument(args.place_delay_ramp_slope, "--place_delay_ramp_slope")
        .help("The slope of the ramp (in seconds per grid tile) which is applied to the placer delay model for delta distance beyond --place_delay_ramp_delta_threshold")
        .default_value("0.0e-9")
        .show_in(argparse::ShowIn::HELP_ONLY);

    place_timing_grp.add_argument(args.place_tsu_rel_margin, "--place_tsu_rel_margin")
        .help(
            "Specifies the scaling factor for cell setup times used by the placer."
            " This effectively controls whether the placer should try to achieve extra margin on setup paths."
            " For example a value of 1.1 corresponds to requesting 10%% setup margin.")
        .default_value("1.0")
        .show_in(argparse::ShowIn::HELP_ONLY);

    place_timing_grp.add_argument(args.place_tsu_abs_margin, "--place_tsu_abs_margin")
        .help(
            "Specifies an absolute offest added to cell setup times used by the placer."
            " This effectively controls whether the placer should try to achieve extra margin on setup paths."
            " For example a value of 500e-12 corresponds to requesting an extra 500ps of setup margin.")
        .default_value("0.0")
        .show_in(argparse::ShowIn::HELP_ONLY);

    place_timing_grp.add_argument(args.post_place_timing_report_file, "--post_place_timing_report")
        .help("Name of the post-placement timing report file (not generated if unspecfied)")
        .default_value("")
        .show_in(argparse::ShowIn::HELP_ONLY);

    auto& route_grp = parser.add_argument_group("routing options");

    route_grp.add_argument(args.max_router_iterations, "--max_router_iterations")
        .help(
            "Maximum number of Pathfinder-based routing iterations before the circuit is"
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
        .help(
            "Sets the growth factor by which the present overuse penalty factor is"
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

    route_grp.add_argument<e_base_cost_type, ParseBaseCost>(args.base_cost_type, "--base_cost_type")
        .help(
            "Sets the basic cost of routing resource nodes:\n"
            " * demand_only: based on expected demand of node type\n"
            " * demand_only_normalized_length: based on expected \n"
            "      demand of node type normalized by length\n"
            " * delay_normalized: like demand_only but normalized\n"
            "      to magnitude of typical routing resource delay\n"
            " * delay_normalized_length: like delay_normalized but\n"
            "      scaled by routing resource length\n"
            " * delay_normalized_freqeuncy: like delay_normalized\n"
            "      but scaled inversely by segment type frequency\n"
            " * delay_normalized_length_freqeuncy: like delay_normalized\n"
            "      but scaled by routing resource length, and inversely\n"
            "      by segment type frequency\n"
            "(Default: demand_only for breadth-first router,\n"
            "          delay_normalized_length for timing-driven router)")
        .show_in(argparse::ShowIn::HELP_ONLY);

    route_grp.add_argument(args.bend_cost, "--bend_cost")
        .help("The cost of a bend. (Default: 1.0 for global routing, 0.0 for detailed routing)")
        .show_in(argparse::ShowIn::HELP_ONLY);

    route_grp.add_argument<e_route_type, ParseRouteType>(args.RouteType, "--route_type")
        .help("Specifies whether global, or combined global and detailed routing is performed.")
        .default_value("detailed")
        .choices({"global", "detailed"})
        .show_in(argparse::ShowIn::HELP_ONLY);

    route_grp.add_argument(args.RouteChanWidth, "--route_chan_width")
        .help(
            "Specifies a fixed channel width to route at."
            " A value of -1 indicates that the minimum channel width should be determined")
        .default_value("-1")
        .metavar("CHANNEL_WIDTH");

    route_grp.add_argument(args.min_route_chan_width_hint, "--min_route_chan_width_hint")
        .help(
            "Hint to the router what the minimum routable channel width is."
            " Good hints can speed-up determining the minimum channel width.")
        .show_in(argparse::ShowIn::HELP_ONLY);

    route_grp.add_argument<bool, ParseOnOff>(args.verify_binary_search, "--verify_binary_search")
        .help(
            "Force the router to verify the minimum channel width by routing at"
            " consecutively lower channel widths until two consecutive failures are observed.")
        .default_value("off")
        .show_in(argparse::ShowIn::HELP_ONLY);

    route_grp.add_argument<e_router_algorithm, ParseRouterAlgorithm>(args.RouterAlgorithm, "--router_algorithm")
        .help(
            "Specifies the router algorithm to use.\n"
            " * breadth_first: focuses solely on routability [DEPRECATED, inferior quality & run-time]\n"
            " * timing_driven: focuses on routability and circuit speed\n")
        .default_value("timing_driven")
        .choices({"breadth_first", "timing_driven"})
        .show_in(argparse::ShowIn::HELP_ONLY);

    route_grp.add_argument(args.min_incremental_reroute_fanout, "--min_incremental_reroute_fanout")
        .help("The net fanout threshold above which nets will be re-routed incrementally.")
        .default_value("16")
        .show_in(argparse::ShowIn::HELP_ONLY);

    auto& route_timing_grp = parser.add_argument_group("timing-driven routing options");

    route_timing_grp.add_argument(args.astar_fac, "--astar_fac")
        .help(
            "Controls the directedness of the the timing-driven router's exploration."
            " Values between 1 and 2 are resonable; higher values trade some quality for reduced run-time")
        .default_value("1.2")
        .show_in(argparse::ShowIn::HELP_ONLY);

    route_timing_grp.add_argument(args.max_criticality, "--max_criticality")
        .help(
            "Sets the maximum fraction of routing cost derived from delay (vs routability) for any net."
            " 0.0 means no attention is paid to delay, 1.0 means nets on the critical path ignore congestion")
        .default_value("0.99")
        .show_in(argparse::ShowIn::HELP_ONLY);

    route_timing_grp.add_argument(args.criticality_exp, "--criticality_exp")
        .help(
            "Controls the delay-routability trade-off for nets as a function of slack."
            " 0.0 implies all nets treated equally regardless of slack."
            " At large values (>> 1) only nets on the critical path will consider delay.")
        .default_value("1.0")
        .show_in(argparse::ShowIn::HELP_ONLY);

    route_timing_grp.add_argument(args.router_init_wirelength_abort_threshold, "--router_init_wirelength_abort_threshold")
        .help(
            "The first routing iteration wirelength abort threshold."
            " If the first routing iteration uses more than this fraction of available wirelength routing is aborted.")
        .default_value("0.85")
        .show_in(argparse::ShowIn::HELP_ONLY);

    route_timing_grp.add_argument<e_incr_reroute_delay_ripup, ParseIncrRerouteDelayRipup>(args.incr_reroute_delay_ripup, "--incremental_reroute_delay_ripup")
        .help("Controls whether incremental net routing will rip-up (and re-route) a critical connection for delay, even if the routing is legal.")
        .default_value("auto")
        .show_in(argparse::ShowIn::HELP_ONLY);

    route_timing_grp.add_argument<e_routing_failure_predictor, ParseRoutePredictor>(args.routing_failure_predictor, "--routing_failure_predictor")
        .help(
            "Controls how aggressively the router will predict a routing as unsuccessful"
            " and give up early. This can significantly reducing the run-time required"
            " to find the minimum channel width.\n"
            " * safe: Only abort when it is extremely unlikely a routing will succeed\n"
            " * aggressive: Further reduce run-time by giving up earlier. This may increase the reported minimum channel width\n"
            " * off: Only abort when the maximum number of iterations is reached\n")
        .default_value("safe")
        .choices({"safe", "aggressive", "off"})
        .show_in(argparse::ShowIn::HELP_ONLY);

    route_timing_grp.add_argument<e_routing_budgets_algorithm, RouteBudgetsAlgorithm>(args.routing_budgets_algorithm, "--routing_budgets_algorithm")
        .help(
            "Controls how the routing budgets are created.\n"
            " * slack: Sets the budgets depending on the amount slack between connections and the current delay values. [EXPERIMENTAL]\n"
            " * criticality: Sets the minimum budgets to 0 and the maximum budgets as a function of delay and criticality (net delay/ pin criticality) [EXPERIMENTAL]\n"
            " * disable: Removes the routing budgets, use the default VPR and ignore hold time constraints\n")
        .default_value("disable")
        .choices({"minimax", "scale_delay", "disable"})
        .show_in(argparse::ShowIn::HELP_ONLY);

    route_timing_grp.add_argument<bool, ParseOnOff>(args.save_routing_per_iteration, "--save_routing_per_iteration")
        .help(
            "Controls whether VPR saves the current routing to a file after each routing iteration."
            " May be helpful for debugging.")
        .default_value("off")
        .show_in(argparse::ShowIn::HELP_ONLY);

    route_timing_grp.add_argument<float>(args.congested_routing_iteration_threshold_frac, "--congested_routing_iteration_threshold")
        .help(
            "Controls when the router enters a high effort mode to resolve lingering routing congestion."
            " Value is the fraction of max_router_iterations beyond which the routing is deemed congested.")
        .default_value("1.0")
        .show_in(argparse::ShowIn::HELP_ONLY);

    route_timing_grp.add_argument<e_route_bb_update, ParseRouteBBUpdate>(args.route_bb_update, "--route_bb_update")
        .help(
            "Controls how the router's net bounding boxes are updated:\n"
            " * static : bounding boxes are never updated\n"
            " * dynamic: bounding boxes are updated dynamically as routing progresses\n")
        .default_value("dynamic")
        .show_in(argparse::ShowIn::HELP_ONLY);

    route_timing_grp.add_argument<int>(args.router_high_fanout_threshold, "--router_high_fanout_threshold")
        .help(
            "Specifies the net fanout beyond which a net is considered high fanout."
            " Values less than zero disable special behaviour for high fanout nets")
        .default_value("64")
        .show_in(argparse::ShowIn::HELP_ONLY);

    route_timing_grp.add_argument<e_router_lookahead, ParseRouterLookahead>(args.router_lookahead_type, "--router_lookahead")
        .help(
            "Controls what lookahead the router uses to calculate cost of completing a connection.\n"
            " * classic: The classic VPR lookahead\n"
            " * map: A more advanced lookahead which accounts for diverse wire type\n")
        .default_value("classic")
        .show_in(argparse::ShowIn::HELP_ONLY);

    route_timing_grp.add_argument(args.router_max_convergence_count, "--router_max_convergence_count")
        .help(
            "Controls how many times the router is allowed to converge to a legal routing before halting."
            " If multiple legal solutions are found the best quality implementation is used.")
        .default_value("1")
        .show_in(argparse::ShowIn::HELP_ONLY);

    route_timing_grp.add_argument(args.router_reconvergence_cpd_threshold, "--router_reconvergence_cpd_threshold")
        .help(
            "Specifies the minimum potential CPD improvement for which the router will"
            " continue to attempt re-convergent routing."
            " For example, a value of 0.99 means the router will not give up on reconvergent"
            " routing if it thinks a > 1% CPD reduction is possible.")
        .default_value("0.99")
        .show_in(argparse::ShowIn::HELP_ONLY);

    route_timing_grp.add_argument(args.router_first_iteration_timing_report_file, "--router_first_iter_timing_report")
        .help("Name of the post first routing iteration timing report file (not generated if unspecfied)")
        .default_value("")
        .show_in(argparse::ShowIn::HELP_ONLY);

    route_timing_grp.add_argument(args.router_debug_net, "--router_debug_net")
        .help(
            "Controls when router debugging is enabled.\n"
            " * For values >= 0, the value is taken as the net ID for\n"
            "   which to enable router debug output.\n"
            " * For value == -1, router debug output is enabled for\n"
            "   all nets.\n"
            " * For values < -1, all net-sbased router debug output is disabled.\n"
            "Note if VPR as compiled without debug logging enabled this will produce only limited output.\n")
        .default_value("-2")
        .show_in(argparse::ShowIn::HELP_ONLY);

    route_timing_grp.add_argument(args.router_debug_sink_rr, "--router_debug_sink_rr")
        .help(
            "Controls when router debugging is enabled for the specified sink RR.\n"
            " * For values >= 0, the value is taken as the sink RR Node ID for\n"
            "   which to enable router debug output.\n"
            " * For values < 0, sink-based router debug output is disabled.\n"
            "Note if VPR as compiled without debug logging enabled this will produce only limited output.\n")
        .default_value("-2")
        .show_in(argparse::ShowIn::HELP_ONLY);

    auto& analysis_grp = parser.add_argument_group("analysis options");

    analysis_grp.add_argument<bool, ParseOnOff>(args.full_stats, "--full_stats")
        .help("Print extra statistics about the circuit and it's routing (useful for wireability analysis)")
        .default_value("off")
        .show_in(argparse::ShowIn::HELP_ONLY);

    analysis_grp.add_argument<bool, ParseOnOff>(args.Generate_Post_Synthesis_Netlist, "--gen_post_synthesis_netlist")
        .help(
            "Generates the post-synthesis netlist (in BLIF and Verilog) along with delay information (in SDF)."
            " Used for post-implementation simulation and verification")
        .default_value("off")
        .show_in(argparse::ShowIn::HELP_ONLY);

    analysis_grp.add_argument(args.timing_report_npaths, "--timing_report_npaths")
        .help("Controls how many timing paths are reported.")
        .default_value("100")
        .show_in(argparse::ShowIn::HELP_ONLY);

    analysis_grp.add_argument<e_timing_report_detail, ParseTimingReportDetail>(args.timing_report_detail, "--timing_report_detail")
        .help(
            "Controls how much detail is provided in timing reports.\n"
            " * netlist: Shows only netlist pins\n"
            " * aggregated: Like 'netlist', but also shows aggregated intra-block/iter-block delays\n"
            //" * routing: Lke 'aggregated' but shows detailed routing instead of aggregated inter-block delays\n" //TODO: implement
            )
        .default_value("netlist")
        .show_in(argparse::ShowIn::HELP_ONLY);

    analysis_grp.add_argument<bool, ParseOnOff>(args.timing_report_skew, "--timing_report_skew")
        .help("Controls whether skew timing reports are generated\n")
        .default_value("off")
        .show_in(argparse::ShowIn::HELP_ONLY);

    auto& power_grp = parser.add_argument_group("power analysis options");

    power_grp.add_argument<bool, ParseOnOff>(args.do_power, "--power")
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
        std::string blif_file = name_ext[0] + name_ext[1];
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
            VTR_LOG_WARN("Command-line argument '%s' has no effect since timing analysis is disabled\n",
                         args.timing_driven_clustering.argument_name().c_str());
        }
        args.timing_driven_clustering.set(args.timing_analysis, Provenance::INFERRED);
    }

    if (args.cluster_seed_type.provenance() != Provenance::SPECIFIED) {
        if (args.timing_driven_clustering) {
            args.cluster_seed_type.set(e_cluster_seed::BLEND2, Provenance::INFERRED);
        } else {
            args.cluster_seed_type.set(e_cluster_seed::MAX_INPUTS, Provenance::INFERRED);
        }
    }

    /*
     * Placement
     */

    //Which placement algorithm to use?
    if (args.PlaceAlgorithm.provenance() != Provenance::SPECIFIED) {
        if (args.timing_analysis) {
            args.PlaceAlgorithm.set(PATH_TIMING_DRIVEN_PLACE, Provenance::INFERRED);
        } else {
            args.PlaceAlgorithm.set(BOUNDING_BOX_PLACE, Provenance::INFERRED);
        }
    }

    //Place chan width follows Route chan width if unspecified
    if (args.PlaceChanWidth.provenance() != Provenance::SPECIFIED && args.RouteChanWidth.provenance() == Provenance::SPECIFIED) {
        args.PlaceChanWidth.set(args.RouteChanWidth.value(), Provenance::INFERRED);
    }

    //Do we calculate timing info during placement?
    if (args.ShowPlaceTiming.provenance() != Provenance::SPECIFIED) {
        args.ShowPlaceTiming.set(args.timing_analysis, Provenance::INFERRED);
    }

    //Are we using the automatic, or user-specified annealing schedule?
    if (args.PlaceInitT.provenance() == Provenance::SPECIFIED
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
    //Base cost type
    if (args.base_cost_type.provenance() != Provenance::SPECIFIED) {
        if (args.RouterAlgorithm == BREADTH_FIRST) {
            args.base_cost_type.set(DEMAND_ONLY, Provenance::INFERRED);
        } else {
            VTR_ASSERT(args.RouterAlgorithm == TIMING_DRIVEN);

            if (args.RouteType == DETAILED) {
                if (args.timing_analysis) {
                    args.base_cost_type.set(DELAY_NORMALIZED_LENGTH, Provenance::INFERRED);
                } else {
                    args.base_cost_type.set(DEMAND_ONLY_NORMALIZED_LENGTH, Provenance::INFERRED);
                }
            } else {
                VTR_ASSERT(args.RouteType == GLOBAL);
                //Global RR graphs don't have valid timing, so use demand base cost
                args.base_cost_type.set(DEMAND_ONLY_NORMALIZED_LENGTH, Provenance::INFERRED);
            }
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
    if (args.read_rr_graph_file.provenance() == Provenance::SPECIFIED
        && args.RouteChanWidth.provenance() != Provenance::SPECIFIED) {
        vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__,
                  "--route_chan_width option must be specified if --read_rr_graph is requested (%s)\n",
                  args.read_rr_graph_file.argument_name().c_str());
    }

    if (!args.enable_clustering_pin_feasibility_filter && (args.target_external_pin_util.provenance() == Provenance::SPECIFIED)) {
        vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__,
                  "%s option must be enabled for %s to have any effect\n",
                  args.enable_clustering_pin_feasibility_filter.argument_name().c_str(),
                  args.target_external_pin_util.argument_name().c_str());
    }

    return true;
}
