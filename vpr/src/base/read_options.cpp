#include "read_options.h"
#include "constant_nets.h"
#include "clock_modeling.h"
#include "vpr_error.h"

#include "argparse.hpp"

#include "ap_flow_enums.h"
#include "vtr_log.h"
#include "vtr_path.h"
#include "vtr_util.h"
#include <string>

using argparse::ConvertedValue;
using argparse::Provenance;

///@brief Read and process VPR's command-line arguments
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
        std::transform(str.begin(), str.end(), str.begin(), ::tolower);
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

struct ParseArchFormat {
    ConvertedValue<e_arch_format> from_str(const std::string& str) {
        ConvertedValue<e_arch_format> conv_value;
        if (str == "vtr")
            conv_value.set_value(e_arch_format::VTR);
        else if (str == "fpga-interchange")
            conv_value.set_value(e_arch_format::FPGAInterchange);
        else {
            std::stringstream msg;
            msg << "Invalid conversion from '" << str << "' to e_arch_format (expected one of: " << argparse::join(default_choices(), ", ") << ")";
            conv_value.set_error(msg.str());
        }
        return conv_value;
    }

    ConvertedValue<std::string> to_str(e_arch_format val) {
        ConvertedValue<std::string> conv_value;

        if (val == e_arch_format::VTR)
            conv_value.set_value("vtr");
        else {
            VTR_ASSERT(val == e_arch_format::FPGAInterchange);
            conv_value.set_value("fpga-interchange");
        }

        return conv_value;
    }

    std::vector<std::string> default_choices() {
        return {"vtr", "fpga-interchange"};
    }
};
struct ParseCircuitFormat {
    ConvertedValue<e_circuit_format> from_str(const std::string& str) {
        ConvertedValue<e_circuit_format> conv_value;
        if (str == "auto")
            conv_value.set_value(e_circuit_format::AUTO);
        else if (str == "blif")
            conv_value.set_value(e_circuit_format::BLIF);
        else if (str == "eblif")
            conv_value.set_value(e_circuit_format::EBLIF);
        else if (str == "fpga-interchange")
            conv_value.set_value(e_circuit_format::FPGA_INTERCHANGE);
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
        else if (val == e_circuit_format::EBLIF)
            conv_value.set_value("eblif");
        else {
            VTR_ASSERT(val == e_circuit_format::FPGA_INTERCHANGE);
            conv_value.set_value("fpga-interchange");
        }

        return conv_value;
    }

    std::vector<std::string> default_choices() {
        return {"auto", "blif", "eblif", "fpga-interchange"};
    }
};

struct ParseAPAnalyticalSolver {
    ConvertedValue<e_ap_analytical_solver> from_str(const std::string& str) {
        ConvertedValue<e_ap_analytical_solver> conv_value;
        if (str == "qp-hybrid")
            conv_value.set_value(e_ap_analytical_solver::QP_Hybrid);
        else if (str == "lp-b2b")
            conv_value.set_value(e_ap_analytical_solver::LP_B2B);
        else {
            std::stringstream msg;
            msg << "Invalid conversion from '" << str << "' to e_ap_analytical_solver (expected one of: " << argparse::join(default_choices(), ", ") << ")";
            conv_value.set_error(msg.str());
        }
        return conv_value;
    }

    ConvertedValue<std::string> to_str(e_ap_analytical_solver val) {
        ConvertedValue<std::string> conv_value;
        switch (val) {
            case e_ap_analytical_solver::QP_Hybrid:
                conv_value.set_value("qp-hybrid");
                break;
            case e_ap_analytical_solver::LP_B2B:
                conv_value.set_value("lp-b2b");
                break;
            default:
                VTR_ASSERT(false);
        }
        return conv_value;
    }

    std::vector<std::string> default_choices() {
        return {"qp-hybrid", "lp-b2b"};
    }
};

struct ParseAPPartialLegalizer {
    ConvertedValue<e_ap_partial_legalizer> from_str(const std::string& str) {
        ConvertedValue<e_ap_partial_legalizer> conv_value;
        if (str == "bipartitioning")
            conv_value.set_value(e_ap_partial_legalizer::BiPartitioning);
        else if (str == "flow-based")
            conv_value.set_value(e_ap_partial_legalizer::FlowBased);
        else {
            std::stringstream msg;
            msg << "Invalid conversion from '" << str << "' to e_ap_partial_legalizer (expected one of: " << argparse::join(default_choices(), ", ") << ")";
            conv_value.set_error(msg.str());
        }
        return conv_value;
    }

    ConvertedValue<std::string> to_str(e_ap_partial_legalizer val) {
        ConvertedValue<std::string> conv_value;
        switch (val) {
            case e_ap_partial_legalizer::BiPartitioning:
                conv_value.set_value("bipartitioning");
                break;
            case e_ap_partial_legalizer::FlowBased:
                conv_value.set_value("flow-based");
                break;
            default:
                VTR_ASSERT(false);
        }
        return conv_value;
    }

    std::vector<std::string> default_choices() {
        return {"bipartitioning", "flow-based"};
    }
};

struct ParseAPFullLegalizer {
    ConvertedValue<e_ap_full_legalizer> from_str(const std::string& str) {
        ConvertedValue<e_ap_full_legalizer> conv_value;
        if (str == "naive")
            conv_value.set_value(e_ap_full_legalizer::Naive);
        else if (str == "appack")
            conv_value.set_value(e_ap_full_legalizer::APPack);
        else if (str == "basic-min-disturbance")
            conv_value.set_value(e_ap_full_legalizer::Basic_Min_Disturbance);
        else {
            std::stringstream msg;
            msg << "Invalid conversion from '" << str << "' to e_ap_full_legalizer (expected one of: " << argparse::join(default_choices(), ", ") << ")";
            conv_value.set_error(msg.str());
        }
        return conv_value;
    }

    ConvertedValue<std::string> to_str(e_ap_full_legalizer val) {
        ConvertedValue<std::string> conv_value;
        switch (val) {
            case e_ap_full_legalizer::Naive:
                conv_value.set_value("naive");
                break;
            case e_ap_full_legalizer::APPack:
                conv_value.set_value("appack");
                break;
            case e_ap_full_legalizer::Basic_Min_Disturbance:
                conv_value.set_value("basic-min-disturbance");
            default:
                VTR_ASSERT(false);
        }
        return conv_value;
    }

    std::vector<std::string> default_choices() {
        return {"naive", "appack", "basic-min-disturbance"};
    }
};

struct ParseAPDetailedPlacer {
    ConvertedValue<e_ap_detailed_placer> from_str(const std::string& str) {
        ConvertedValue<e_ap_detailed_placer> conv_value;
        if (str == "none")
            conv_value.set_value(e_ap_detailed_placer::Identity);
        else if (str == "annealer")
            conv_value.set_value(e_ap_detailed_placer::Annealer);
        else {
            std::stringstream msg;
            msg << "Invalid conversion from '" << str << "' to e_ap_detailed_placer (expected one of: " << argparse::join(default_choices(), ", ") << ")";
            conv_value.set_error(msg.str());
        }
        return conv_value;
    }

    ConvertedValue<std::string> to_str(e_ap_detailed_placer val) {
        ConvertedValue<std::string> conv_value;
        switch (val) {
            case e_ap_detailed_placer::Identity:
                conv_value.set_value("none");
                break;
            case e_ap_detailed_placer::Annealer:
                conv_value.set_value("annealer");
                break;
            default:
                VTR_ASSERT(false);
        }
        return conv_value;
    }

    std::vector<std::string> default_choices() {
        return {"none", "annealer"};
    }
};

struct ParseRoutePredictor {
    ConvertedValue<e_routing_failure_predictor> from_str(const std::string& str) {
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
    ConvertedValue<e_router_algorithm> from_str(const std::string& str) {
        ConvertedValue<e_router_algorithm> conv_value;
        if (str == "nested")
            conv_value.set_value(NESTED);
        else if (str == "parallel")
            conv_value.set_value(PARALLEL);
        else if (str == "parallel_decomp")
            conv_value.set_value(PARALLEL_DECOMP);
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
        if (val == NESTED)
            conv_value.set_value("nested");
        else if (val == PARALLEL)
            conv_value.set_value("parallel");
        else if (val == PARALLEL_DECOMP)
            conv_value.set_value("parallel_decomp");
        else {
            VTR_ASSERT(val == TIMING_DRIVEN);
            conv_value.set_value("timing_driven");
        }
        return conv_value;
    }

    std::vector<std::string> default_choices() {
        return {"parallel", "timing_driven"};
    }
};

struct ParseNodeReorderAlgorithm {
    ConvertedValue<e_rr_node_reorder_algorithm> from_str(const std::string& str) {
        ConvertedValue<e_rr_node_reorder_algorithm> conv_value;
        if (str == "none")
            conv_value.set_value(DONT_REORDER);
        else if (str == "degree_bfs")
            conv_value.set_value(DEGREE_BFS);
        else if (str == "random_shuffle")
            conv_value.set_value(RANDOM_SHUFFLE);
        else {
            std::stringstream msg;
            msg << "Invalid conversion from '" << str << "' to e_rr_node_reorder_algorithm (expected one of: " << argparse::join(default_choices(), ", ") << ")";
            conv_value.set_error(msg.str());
        }
        return conv_value;
    }

    ConvertedValue<std::string> to_str(e_rr_node_reorder_algorithm val) {
        ConvertedValue<std::string> conv_value;
        if (val == DONT_REORDER)
            conv_value.set_value("none");
        else if (val == DEGREE_BFS)
            conv_value.set_value("degree_bfs");
        else {
            VTR_ASSERT(val == RANDOM_SHUFFLE);
            conv_value.set_value("random_shuffle");
        }
        return conv_value;
    }

    std::vector<std::string> default_choices() {
        return {"none", "degree_bfs", "random_shuffle"};
    }
};

struct RouteBudgetsAlgorithm {
    ConvertedValue<e_routing_budgets_algorithm> from_str(const std::string& str) {
        ConvertedValue<e_routing_budgets_algorithm> conv_value;
        if (str == "minimax")
            conv_value.set_value(MINIMAX);
        else if (str == "yoyo")
            conv_value.set_value(YOYO);
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
        else if (val == YOYO)
            conv_value.set_value("yoyo");
        else if (val == DISABLE)
            conv_value.set_value("disable");
        else {
            VTR_ASSERT(val == SCALE_DELAY);
            conv_value.set_value("scale_delay");
        }
        return conv_value;
    }

    std::vector<std::string> default_choices() {
        return {"minimax", "yoyo", "scale_delay", "disable"};
    }
};

struct ParseRouteType {
    ConvertedValue<e_route_type> from_str(const std::string& str) {
        ConvertedValue<e_route_type> conv_value;
        if (str == "global")
            conv_value.set_value(e_route_type::GLOBAL);
        else if (str == "detailed")
            conv_value.set_value(e_route_type::DETAILED);
        else {
            std::stringstream msg;
            msg << "Invalid conversion from '" << str << "' to e_router_algorithm (expected one of: " << argparse::join(default_choices(), ", ") << ")";
            conv_value.set_error(msg.str());
        }
        return conv_value;
    }

    ConvertedValue<std::string> to_str(e_route_type val) {
        ConvertedValue<std::string> conv_value;
        if (val == e_route_type::GLOBAL)
            conv_value.set_value("global");
        else {
            VTR_ASSERT(val == e_route_type::DETAILED);
            conv_value.set_value("detailed");
        }
        return conv_value;
    }

    std::vector<std::string> default_choices() {
        return {"global", "detailed"};
    }
};

struct ParseBaseCost {
    ConvertedValue<e_base_cost_type> from_str(const std::string& str) {
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
        else if (str == "delay_normalized_length_bounded")
            conv_value.set_value(DELAY_NORMALIZED_LENGTH_BOUNDED);
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
        else if (val == DELAY_NORMALIZED_LENGTH_BOUNDED)
            conv_value.set_value("delay_normalized_length_bounded");
        else {
            VTR_ASSERT(val == DEMAND_ONLY);
            conv_value.set_value("demand_only");
        }
        return conv_value;
    }

    std::vector<std::string> default_choices() {
        return {"demand_only", "demand_only_normalized_length", "delay_normalized", "delay_normalized_length", "delay_normalized_length_bounded", "delay_normalized_frequency", "delay_normalized_length_frequency"};
    }
};

struct ParsePlaceDeltaDelayAlgorithm {
    ConvertedValue<e_place_delta_delay_algorithm> from_str(const std::string& str) {
        ConvertedValue<e_place_delta_delay_algorithm> conv_value;
        if (str == "astar")
            conv_value.set_value(e_place_delta_delay_algorithm::ASTAR_ROUTE);
        else if (str == "dijkstra")
            conv_value.set_value(e_place_delta_delay_algorithm::DIJKSTRA_EXPANSION);
        else {
            std::stringstream msg;
            msg << "Invalid conversion from '" << str << "' to e_place_delta_delay_algorithm (expected one of: " << argparse::join(default_choices(), ", ") << ")";
            conv_value.set_error(msg.str());
        }
        return conv_value;
    }

    ConvertedValue<std::string> to_str(e_place_delta_delay_algorithm val) {
        ConvertedValue<std::string> conv_value;
        if (val == e_place_delta_delay_algorithm::ASTAR_ROUTE)
            conv_value.set_value("astar");
        else {
            VTR_ASSERT(val == e_place_delta_delay_algorithm::DIJKSTRA_EXPANSION);
            conv_value.set_value("dijkstra");
        }
        return conv_value;
    }

    std::vector<std::string> default_choices() {
        return {"astar", "dijkstra"};
    }
};

struct ParsePlaceAlgorithm {
    ConvertedValue<e_place_algorithm> from_str(const std::string& str) {
        ConvertedValue<e_place_algorithm> conv_value;
        if (str == "bounding_box") {
            conv_value.set_value(e_place_algorithm::BOUNDING_BOX_PLACE);
        } else if (str == "criticality_timing") {
            conv_value.set_value(e_place_algorithm::CRITICALITY_TIMING_PLACE);
        } else if (str == "slack_timing") {
            conv_value.set_value(e_place_algorithm::SLACK_TIMING_PLACE);
        } else {
            std::stringstream msg;
            msg << "Invalid conversion from '" << str << "' to e_place_algorithm (expected one of: " << argparse::join(default_choices(), ", ") << ")";

            //Deprecated option: "path_timing_driven" -> PATH_DRIVEN_TIMING_PLACE
            //New option: "criticality_timing" -> CRITICALITY_TIMING_PLACE
            if (str == "path_timing_driven") {
                msg << "\nDeprecated option: 'path_timing_driven'. It has been renamed to 'criticality_timing'";
            }

            conv_value.set_error(msg.str());
        }
        return conv_value;
    }

    ConvertedValue<std::string> to_str(e_place_algorithm val) {
        ConvertedValue<std::string> conv_value;
        if (val == e_place_algorithm::BOUNDING_BOX_PLACE) {
            conv_value.set_value("bounding_box");
        } else if (val == e_place_algorithm::CRITICALITY_TIMING_PLACE) {
            conv_value.set_value("criticality_timing");
        } else {
            VTR_ASSERT(val == e_place_algorithm::SLACK_TIMING_PLACE);
            conv_value.set_value("slack_timing");
        }
        return conv_value;
    }

    std::vector<std::string> default_choices() {
        return {"bounding_box", "criticality_timing", "slack_timing"};
    }
};

struct ParsePlaceBoundingBox {
    ConvertedValue<e_place_bounding_box_mode> from_str(const std::string& str) {
        ConvertedValue<e_place_bounding_box_mode> conv_value;
        if (str == "auto_bb") {
            conv_value.set_value(e_place_bounding_box_mode::AUTO_BB);
        } else if (str == "cube_bb") {
            conv_value.set_value(e_place_bounding_box_mode::CUBE_BB);
        } else if (str == "per_layer_bb") {
            conv_value.set_value(e_place_bounding_box_mode::PER_LAYER_BB);
        } else {
            std::stringstream msg;
            msg << "Invalid conversion from '" << str << "' to e_place_algorithm (expected one of: " << argparse::join(default_choices(), ", ") << ")";
            conv_value.set_error(msg.str());
        }
        return conv_value;
    }

    ConvertedValue<std::string> to_str(e_place_bounding_box_mode val) {
        ConvertedValue<std::string> conv_value;
        if (val == e_place_bounding_box_mode::AUTO_BB) {
            conv_value.set_value("auto_bb");
        } else if (val == e_place_bounding_box_mode::CUBE_BB) {
            conv_value.set_value("cube_bb");
        } else {
            VTR_ASSERT(val == e_place_bounding_box_mode::PER_LAYER_BB);
            conv_value.set_value("per_layer_bb");
        }
        return conv_value;
    }

    std::vector<std::string> default_choices() {
        return {"auto_bb", "cube_bb", "per_layer_bb"};
    }
};

struct ParsePlaceAgentAlgorithm {
    ConvertedValue<e_agent_algorithm> from_str(const std::string& str) {
        ConvertedValue<e_agent_algorithm> conv_value;
        if (str == "e_greedy")
            conv_value.set_value(e_agent_algorithm::E_GREEDY);
        else if (str == "softmax")
            conv_value.set_value(e_agent_algorithm::SOFTMAX);
        else {
            std::stringstream msg;
            msg << "Invalid conversion from '" << str << "' to e_agent_algorithm (expected one of: " << argparse::join(default_choices(), ", ") << ")";
            conv_value.set_error(msg.str());
        }
        return conv_value;
    }

    ConvertedValue<std::string> to_str(e_agent_algorithm val) {
        ConvertedValue<std::string> conv_value;
        if (val == e_agent_algorithm::E_GREEDY)
            conv_value.set_value("e_greedy");
        else {
            VTR_ASSERT(val == e_agent_algorithm::SOFTMAX);
            conv_value.set_value("softmax");
        }
        return conv_value;
    }

    std::vector<std::string> default_choices() {
        return {"e_greedy", "softmax"};
    }
};

struct ParsePlaceAgentSpace {
    ConvertedValue<e_agent_space> from_str(const std::string& str) {
        ConvertedValue<e_agent_space> conv_value;
        if (str == "move_type")
            conv_value.set_value(e_agent_space::MOVE_TYPE);
        else if (str == "move_block_type")
            conv_value.set_value(e_agent_space::MOVE_BLOCK_TYPE);
        else {
            std::stringstream msg;
            msg << "Invalid conversion from '" << str << "' to e_agent_space (expected one of: " << argparse::join(default_choices(), ", ") << ")";
            conv_value.set_error(msg.str());
        }
        return conv_value;
    }

    ConvertedValue<std::string> to_str(e_agent_space val) {
        ConvertedValue<std::string> conv_value;
        if (val == e_agent_space::MOVE_TYPE)
            conv_value.set_value("move_type");
        else {
            VTR_ASSERT(val == e_agent_space::MOVE_BLOCK_TYPE);
            conv_value.set_value("move_block_type");
        }
        return conv_value;
    }

    std::vector<std::string> default_choices() {
        return {"move_type", "move_block_type"};
    }
};

struct ParseFixPins {
    ConvertedValue<e_pad_loc_type> from_str(const std::string& str) {
        ConvertedValue<e_pad_loc_type> conv_value;
        if (str == "free")
            conv_value.set_value(e_pad_loc_type::FREE);
        else if (str == "random")
            conv_value.set_value(e_pad_loc_type::RANDOM);
        else {
            std::stringstream msg;
            msg << "Invalid conversion from '" << str << "' to e_router_algorithm (expected one of: " << argparse::join(default_choices(), ", ") << ")";
            conv_value.set_error(msg.str());
        }
        return conv_value;
    }

    ConvertedValue<std::string> to_str(e_pad_loc_type val) {
        ConvertedValue<std::string> conv_value;
        if (val == e_pad_loc_type::FREE)
            conv_value.set_value("free");
        else {
            VTR_ASSERT(val == e_pad_loc_type::RANDOM);
            conv_value.set_value("random");
        }
        return conv_value;
    }

    std::vector<std::string> default_choices() {
        return {"free", "random"};
    }
};

struct ParseClusterSeed {
    ConvertedValue<e_cluster_seed> from_str(const std::string& str) {
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
    ConvertedValue<e_constant_net_method> from_str(const std::string& str) {
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
    ConvertedValue<e_timing_report_detail> from_str(const std::string& str) {
        ConvertedValue<e_timing_report_detail> conv_value;
        if (str == "netlist")
            conv_value.set_value(e_timing_report_detail::NETLIST);
        else if (str == "aggregated")
            conv_value.set_value(e_timing_report_detail::AGGREGATED);
        else if (str == "detailed")
            conv_value.set_value(e_timing_report_detail::DETAILED_ROUTING);
        else if (str == "debug")
            conv_value.set_value(e_timing_report_detail::DEBUG);
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
        else if (val == e_timing_report_detail::AGGREGATED) {
            conv_value.set_value("aggregated");
        } else if (val == e_timing_report_detail::DETAILED_ROUTING) {
            VTR_ASSERT(val == e_timing_report_detail::DETAILED_ROUTING);
            conv_value.set_value("detailed");
        } else {
            VTR_ASSERT(val == e_timing_report_detail::DEBUG);
            conv_value.set_value("debug");
        }
        return conv_value;
    }

    std::vector<std::string> default_choices() {
        return {"netlist", "aggregated", "detailed", "debug"};
    }
};

struct ParseClockModeling {
    ConvertedValue<e_clock_modeling> from_str(const std::string& str) {
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
    ConvertedValue<e_unrelated_clustering> from_str(const std::string& str) {
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
    ConvertedValue<e_balance_block_type_util> from_str(const std::string& str) {
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
    ConvertedValue<e_const_gen_inference> from_str(const std::string& str) {
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
    ConvertedValue<e_incr_reroute_delay_ripup> from_str(const std::string& str) {
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
    ConvertedValue<e_route_bb_update> from_str(const std::string& str) {
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
        std::transform(str.begin(), str.end(), str.begin(), ::tolower);
        ConvertedValue<e_router_lookahead> conv_value;
        if (str == "classic")
            conv_value.set_value(e_router_lookahead::CLASSIC);
        else if (str == "map")
            conv_value.set_value(e_router_lookahead::MAP);
        else if (str == "compressed_map")
            conv_value.set_value(e_router_lookahead::COMPRESSED_MAP);
        else if (str == "extended_map")
            conv_value.set_value(e_router_lookahead::EXTENDED_MAP);
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
        else if (val == e_router_lookahead::MAP) {
            conv_value.set_value("map");
        } else if (val == e_router_lookahead::COMPRESSED_MAP) {
            conv_value.set_value("compressed_map");
        } else {
            VTR_ASSERT(val == e_router_lookahead::EXTENDED_MAP);
            conv_value.set_value("extended_map");
        }
        return conv_value;
    }

    std::vector<std::string> default_choices() {
        return {"classic", "map", "compressed_map", "extended_map"};
    }
};

struct ParsePlaceDelayModel {
    ConvertedValue<PlaceDelayModelType> from_str(const std::string& str) {
        ConvertedValue<PlaceDelayModelType> conv_value;
        if (str == "simple") {
            conv_value.set_value(PlaceDelayModelType::SIMPLE);
        } else if (str == "delta")
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
        if (val == PlaceDelayModelType::SIMPLE)
            conv_value.set_value("simple");
        else if (val == PlaceDelayModelType::DELTA)
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
        return {"simple", "delta", "delta_override"};
    }
};

struct ParseReducer {
    ConvertedValue<e_reducer> from_str(const std::string& str) {
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

struct ParseRouterFirstIterTiming {
    ConvertedValue<e_router_initial_timing> from_str(const std::string& str) {
        ConvertedValue<e_router_initial_timing> conv_value;
        if (str == "all_critical")
            conv_value.set_value(e_router_initial_timing::ALL_CRITICAL);
        else if (str == "lookahead")
            conv_value.set_value(e_router_initial_timing::LOOKAHEAD);
        else {
            std::stringstream msg;
            msg << "Invalid conversion from '" << str << "' to e_router_initial_timing (expected one of: " << argparse::join(default_choices(), ", ") << ")";
            conv_value.set_error(msg.str());
        }
        return conv_value;
    }

    ConvertedValue<std::string> to_str(e_router_initial_timing val) {
        ConvertedValue<std::string> conv_value;
        if (val == e_router_initial_timing::ALL_CRITICAL)
            conv_value.set_value("all_critical");
        else {
            VTR_ASSERT(val == e_router_initial_timing::LOOKAHEAD);
            conv_value.set_value("lookahead");
        }
        return conv_value;
    }

    std::vector<std::string> default_choices() {
        return {"all_critical", "lookahead"};
    }
};

struct ParseRouterHeap {
    ConvertedValue<e_heap_type> from_str(const std::string& str) {
        ConvertedValue<e_heap_type> conv_value;
        if (str == "binary")
            conv_value.set_value(e_heap_type::BINARY_HEAP);
        else if (str == "four_ary")
            conv_value.set_value(e_heap_type::FOUR_ARY_HEAP);
        else {
            std::stringstream msg;
            msg << "Invalid conversion from '" << str << "' to e_heap_type (expected one of: " << argparse::join(default_choices(), ", ") << ")";
            conv_value.set_error(msg.str());
        }
        return conv_value;
    }

    ConvertedValue<std::string> to_str(e_heap_type val) {
        ConvertedValue<std::string> conv_value;
        if (val == e_heap_type::BINARY_HEAP)
            conv_value.set_value("binary");
        else {
            VTR_ASSERT(val == e_heap_type::FOUR_ARY_HEAP);
            conv_value.set_value("four_ary");
        }
        return conv_value;
    }

    std::vector<std::string> default_choices() {
        return {"binary", "four_ary", "bucket"};
    }
};

struct ParseCheckRoute {
    ConvertedValue<e_check_route_option> from_str(const std::string& str) {
        ConvertedValue<e_check_route_option> conv_value;
        if (str == "off")
            conv_value.set_value(e_check_route_option::OFF);
        else if (str == "quick")
            conv_value.set_value(e_check_route_option::QUICK);
        else if (str == "full")
            conv_value.set_value(e_check_route_option::FULL);
        else {
            std::stringstream msg;
            msg << "Invalid conversion from '" << str << "' to e_check_route_option (expected one of: " << argparse::join(default_choices(), ", ") << ")";
            conv_value.set_error(msg.str());
        }
        return conv_value;
    }

    ConvertedValue<std::string> to_str(e_check_route_option val) {
        ConvertedValue<std::string> conv_value;
        if (val == e_check_route_option::OFF)
            conv_value.set_value("off");
        else if (val == e_check_route_option::QUICK)
            conv_value.set_value("quick");
        else {
            VTR_ASSERT(val == e_check_route_option::FULL);
            conv_value.set_value("full");
        }
        return conv_value;
    }

    std::vector<std::string> default_choices() {
        return {"off", "quick", "full"};
    }
};

struct ParsePlaceEfforScaling {
    ConvertedValue<e_place_effort_scaling> from_str(const std::string& str) {
        ConvertedValue<e_place_effort_scaling> conv_value;
        if (str == "circuit")
            conv_value.set_value(e_place_effort_scaling::CIRCUIT);
        else if (str == "device_circuit")
            conv_value.set_value(e_place_effort_scaling::DEVICE_CIRCUIT);
        else {
            std::stringstream msg;
            msg << "Invalid conversion from '" << str << "' to e_place_effort_scaling (expected one of: " << argparse::join(default_choices(), ", ") << ")";
            conv_value.set_error(msg.str());
        }
        return conv_value;
    }

    ConvertedValue<std::string> to_str(e_place_effort_scaling val) {
        ConvertedValue<std::string> conv_value;
        if (val == e_place_effort_scaling::CIRCUIT)
            conv_value.set_value("circuit");
        else {
            VTR_ASSERT(val == e_place_effort_scaling::DEVICE_CIRCUIT);
            conv_value.set_value("device_circuit");
        }
        return conv_value;
    }

    std::vector<std::string> default_choices() {
        return {"circuit", "device_circuit"};
    }
};

struct ParseTimingUpdateType {
    ConvertedValue<e_timing_update_type> from_str(const std::string& str) {
        ConvertedValue<e_timing_update_type> conv_value;
        if (str == "auto")
            conv_value.set_value(e_timing_update_type::AUTO);
        else if (str == "full")
            conv_value.set_value(e_timing_update_type::FULL);
        else if (str == "incremental")
            conv_value.set_value(e_timing_update_type::INCREMENTAL);
        else {
            std::stringstream msg;
            msg << "Invalid conversion from '" << str << "' to e_timing_update_type (expected one of: " << argparse::join(default_choices(), ", ") << ")";
            conv_value.set_error(msg.str());
        }
        return conv_value;
    }

    ConvertedValue<std::string> to_str(e_timing_update_type val) {
        ConvertedValue<std::string> conv_value;
        if (val == e_timing_update_type::AUTO)
            conv_value.set_value("auto");
        if (val == e_timing_update_type::FULL)
            conv_value.set_value("full");
        else {
            VTR_ASSERT(val == e_timing_update_type::INCREMENTAL);
            conv_value.set_value("incremental");
        }
        return conv_value;
    }

    std::vector<std::string> default_choices() {
        return {"auto", "full", "incremental"};
    }
};

struct ParsePostSynthNetlistUnconnInputHandling {
    ConvertedValue<e_post_synth_netlist_unconn_handling> from_str(const std::string& str) {
        ConvertedValue<e_post_synth_netlist_unconn_handling> conv_value;
        if (str == "unconnected")
            conv_value.set_value(e_post_synth_netlist_unconn_handling::UNCONNECTED);
        else if (str == "nets")
            conv_value.set_value(e_post_synth_netlist_unconn_handling::NETS);
        else if (str == "gnd")
            conv_value.set_value(e_post_synth_netlist_unconn_handling::GND);
        else if (str == "vcc")
            conv_value.set_value(e_post_synth_netlist_unconn_handling::VCC);
        else {
            std::stringstream msg;
            msg << "Invalid conversion from '" << str << "' to e_post_synth_netlist_unconn_handling (expected one of: " << argparse::join(default_choices(), ", ") << ")";
            conv_value.set_error(msg.str());
        }
        return conv_value;
    }

    ConvertedValue<std::string> to_str(e_post_synth_netlist_unconn_handling val) {
        ConvertedValue<std::string> conv_value;
        if (val == e_post_synth_netlist_unconn_handling::NETS)
            conv_value.set_value("nets");
        else if (val == e_post_synth_netlist_unconn_handling::GND)
            conv_value.set_value("gnd");
        else if (val == e_post_synth_netlist_unconn_handling::VCC)
            conv_value.set_value("vcc");
        else {
            VTR_ASSERT(val == e_post_synth_netlist_unconn_handling::UNCONNECTED);
            conv_value.set_value("unconnected");
        }
        return conv_value;
    }

    std::vector<std::string> default_choices() {
        return {"unconnected", "nets", "gnd", "vcc"};
    }
};

struct ParsePostSynthNetlistUnconnOutputHandling {
    ConvertedValue<e_post_synth_netlist_unconn_handling> from_str(const std::string& str) {
        ConvertedValue<e_post_synth_netlist_unconn_handling> conv_value;
        if (str == "unconnected")
            conv_value.set_value(e_post_synth_netlist_unconn_handling::UNCONNECTED);
        else if (str == "nets")
            conv_value.set_value(e_post_synth_netlist_unconn_handling::NETS);
        else {
            std::stringstream msg;
            msg << "Invalid conversion from '" << str << "' to e_post_synth_netlist_unconn_handling (expected one of: " << argparse::join(default_choices(), ", ") << ")";
            conv_value.set_error(msg.str());
        }
        return conv_value;
    }

    ConvertedValue<std::string> to_str(e_post_synth_netlist_unconn_handling val) {
        ConvertedValue<std::string> conv_value;
        if (val == e_post_synth_netlist_unconn_handling::NETS)
            conv_value.set_value("nets");
        else {
            VTR_ASSERT(val == e_post_synth_netlist_unconn_handling::UNCONNECTED);
            conv_value.set_value("unconnected");
        }
        return conv_value;
    }

    std::vector<std::string> default_choices() {
        return {"unconnected", "nets"};
    }
};

argparse::ArgumentParser create_arg_parser(const std::string& prog_name, t_options& args) {
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
        .help(
            "FPGA Architecture description file\n"
            "   - XML: this is the default frontend format\n"
            "   - FPGA Interchange: device architecture file in the FPGA Interchange format");

    pos_grp.add_argument(args.CircuitName, "circuit")
        .help("Circuit file (or circuit name if --circuit_file specified)");

    auto& stage_grp = parser.add_argument_group("stage options");

    stage_grp.add_argument<bool, ParseOnOff>(args.do_packing, "--pack")
        .help("Run packing")
        .action(argparse::Action::STORE_TRUE)
        .default_value("off");

    stage_grp.add_argument<bool, ParseOnOff>(args.do_legalize, "--legalize")
        .help("Legalize a flat placement, i.e. reconstruct and place clusters based on a flat placement file, which lists cluster and intra-cluster placement coordinates for each primitive.")
        .action(argparse::Action::STORE_TRUE)
        .default_value("off");

    stage_grp.add_argument<bool, ParseOnOff>(args.do_placement, "--place")
        .help("Run placement")
        .action(argparse::Action::STORE_TRUE)
        .default_value("off");

    stage_grp.add_argument<bool, ParseOnOff>(args.do_analytical_placement, "--analytical_place")
        .help("Run analytical placement. Analytical Placement uses an integrated packing and placement algorithm, using information from the primitive level to improve clustering and placement.")
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

    gfx_grp.add_argument<bool, ParseOnOff>(args.save_graphics, "--save_graphics")
        .help("Save all graphical contents to PDF files")
        .default_value("off");

    gfx_grp.add_argument(args.graphics_commands, "--graphics_commands")
        .help(
            "A set of semi-colon seperated graphics commands. \n"
            "Commands must be surrounded by quotation marks (e.g. --graphics_commands \"save_graphics place.png\")\n"
            "   Commands:\n"
            "      * save_graphics <file>\n"
            "           Saves graphics to the specified file (.png/.pdf/\n"
            "           .svg). If <file> contains '{i}', it will be\n"
            "           replaced with an integer which increments\n"
            "           each time graphics is invoked.\n"
            "      * set_macros <int>\n"
            "           Sets the placement macro drawing state\n"
            "      * set_nets <int>\n"
            "           Sets the net drawing state\n"
            "      * set_cpd <int>\n"
            "           Sets the critical path delay drawing state\n"
            "      * set_routing_util <int>\n"
            "           Sets the routing utilization drawing state\n"
            "      * set_clip_routing_util <int>\n"
            "           Sets whether routing utilization values are\n"
            "           clipped to [0., 1.]. Useful when a consistent\n"
            "           scale is needed across images\n"
            "      * set_draw_block_outlines <int>\n"
            "           Sets whether blocks have an outline drawn around\n"
            "           them\n"
            "      * set_draw_block_text <int>\n"
            "           Sets whether blocks have label text drawn on them\n"
            "      * set_draw_block_internals <int>\n"
            "           Sets the level to which block internals are drawn\n"
            "      * set_draw_net_max_fanout <int>\n"
            "           Sets the maximum fanout for nets to be drawn (if\n"
            "           fanout is beyond this value the net will not be\n"
            "           drawn)\n"
            "      * set_congestion <int>\n"
            "           Sets the routing congestion drawing state\n"
            "      * exit <int>\n"
            "           Exits VPR with specified exit code\n"
            "\n"
            "   Example:\n"
            "     'save_graphics place.png; \\\n"
            "      set_nets 1; save_graphics nets1.png;\\\n"
            "      set_nets 2; save_graphics nets2.png; set_nets 0;\\\n"
            "      set_cpd 1; save_graphics cpd1.png; \\\n"
            "      set_cpd 3; save_graphics cpd3.png; set_cpd 0; \\\n"
            "      set_routing_util 5; save_graphics routing_util5.png; \\\n"
            "      set_routing_util 0; \\\n"
            "      set_congestion 1; save_graphics congestion1.png;'\n"
            "\n"
            "   The above toggles various graphics settings (e.g. drawing\n"
            "   nets, drawing critical path) and then saves the results to\n"
            "   .png files.\n"
            "\n"
            "   Note that drawing state is reset to its previous state after\n"
            "   these commands are invoked.\n"
            "\n"
            "   Like the interactive graphics --disp option, the --auto\n"
            "   option controls how often the commands specified with\n"
            "   this option are invoked.\n")
        .default_value("");

    auto& gen_grp = parser.add_argument_group("general options");

    gen_grp.add_argument(args.show_help, "--help", "-h")
        .help("Show this help message then exit")
        .action(argparse::Action::HELP);

    gen_grp.add_argument<bool, ParseOnOff>(args.show_version, "--version")
        .help("Show version information then exit")
        .action(argparse::Action::VERSION);

    gen_grp.add_argument<bool, ParseOnOff>(args.show_arch_resources, "--show_arch_resources")
        .help("Show architecture resources then exit")
        .action(argparse::Action::STORE_TRUE)
        .default_value("off");

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

    gen_grp.add_argument<e_timing_update_type, ParseTimingUpdateType>(args.timing_update_type, "--timing_update_type")
        .help(
            "Controls how timing analysis updates are performed:\n"
            " * auto: VPR decides\n"
            " * full: Full timing updates are performed (may be faster \n"
            "         if circuit timing has changed significantly)\n"
            " * incr: Incremental timing updates are performed (may be \n"
            "         faster in the face of smaller circuit timing changes)\n")
        .default_value("auto")
        .show_in(argparse::ShowIn::HELP_ONLY);

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

    gen_grp.add_argument<bool, ParseOnOff>(args.verify_route_file_switch_id, "--verify_route_file_switch_id")
        .help(
            "Verify that the switch IDs in the routing file are consistent with those in the RR Graph. "
            "Set this to false when switch IDs in the routing file may differ from the RR Graph. "
            "For example, when analyzing different timing corners using the same netlist, placement, and routing files, "
            "the RR switch IDs in the RR Graph may differ due to changes in delays. "
            "In such cases, set this option to false so that the switch IDs from the RR Graph are used, and those in the routing file are ignored.\n")
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
            " * route : Treat constant nets as normal nets (routed)\n")
        .default_value("global")
        .show_in(argparse::ShowIn::HELP_ONLY);

    gen_grp.add_argument<e_clock_modeling, ParseClockModeling>(args.clock_modeling, "--clock_modeling")
        .help(
            "Specifies how clock nets are handled\n"
            " * ideal: Treat clock pins as ideal\n"
            "          (i.e. no routing delays on clocks)\n"
            " * route: Treat the clock pins as normal nets\n"
            "          (i.e. routed using inter-block routing)\n"
            " * dedicated_network : Build a dedicated clock network based on the\n"
            "                       clock network specified in the architecture file\n")
        .default_value("ideal")
        .show_in(argparse::ShowIn::HELP_ONLY);

    gen_grp.add_argument<bool, ParseOnOff>(args.two_stage_clock_routing, "--two_stage_clock_routing")
        .help(
            "Routes clock nets in two stages if using a dedicated clock network.\n"
            " * First stage: From the Net source to a dedicated clock network source\n"
            " * Second stage: From the clock network source to net sinks\n")
        .default_value("off")
        .action(argparse::Action::STORE_TRUE)
        .show_in(argparse::ShowIn::HELP_ONLY);

    gen_grp.add_argument<bool, ParseOnOff>(args.exit_before_pack, "--exit_before_pack")
        .help("Causes VPR to exit before packing starts (useful for statistics collection)")
        .default_value("off")
        .show_in(argparse::ShowIn::HELP_ONLY);

    gen_grp.add_argument<bool, ParseOnOff>(args.strict_checks, "--strict_checks")
        .help(
            "Controls whether VPR enforces some consistency checks strictly (as errors) or treats them as warnings."
            " Usually these checks indicate an issue with either the targeted architecture, or consistency issues"
            " with VPR's internal data structures/algorithms (possibly harming optimization quality)."
            " In specific circumstances on specific architectures these checks may be too restrictive and can be turned off."
            " However exercise extreme caution when turning this option off -- be sure you completely understand why the issue"
            " is being flagged, and why it is OK to treat as a warning instead of an error.")
        .default_value("on")
        .show_in(argparse::ShowIn::HELP_ONLY);

    gen_grp.add_argument<std::string>(args.disable_errors, "--disable_errors")
        .help(
            "Parses a list of functions for which the errors are going to be treated as warnings.\n"
            "Each function in the list is delimited by `:`\n"
            "This option should be only used for development purposes.")
        .default_value("");

    gen_grp.add_argument<std::string>(args.suppress_warnings, "--suppress_warnings")
        .help(
            "Parses a list of functions for which the warnings will be suppressed on stdout.\n"
            "The first element of the list is the name of the output log file with the suppressed warnings.\n"
            "The output log file can be omitted to completely suppress warnings.\n"
            "The file name and the list of functions is separated by `,`. If no output log file is specified,\n"
            "the comma is not needed.\n"
            "Each function in the list is delimited by `:`\n"
            "This option should be only used for development purposes.")
        .default_value("");

    gen_grp.add_argument<bool, ParseOnOff>(args.allow_dangling_combinational_nodes, "--allow_dangling_combinational_nodes")
        .help(
            "Option to allow dangling combinational nodes in the timing graph.\n"
            "This option should normally be off, as dangling combinational nodes are unusual\n"
            "in the timing graph and may indicate a problem in the circuit or architecture.\n"
            "Unless you understand why your architecture/circuit can have valid dangling combinational nodes, this option should be off.\n"
            "In general this is a dev-only option and should not be turned on by the end-user.")
        .default_value("off")
        .show_in(argparse::ShowIn::HELP_ONLY);

    gen_grp.add_argument<bool, ParseOnOff>(args.terminate_if_timing_fails, "--terminate_if_timing_fails")
        .help(
            "During final timing analysis after routing, if a negative slack anywhere is returned and this option is set, \n"
            "VPR_FATAL_ERROR is called and processing ends.")
        .default_value("off");

    auto& file_grp = parser.add_argument_group("file options");

    file_grp.add_argument<e_arch_format, ParseArchFormat>(args.arch_format, "--arch_format")
        .help(
            "File format for the input atom-level circuit/netlist.\n"
            " * vtr: Architecture expressed in the explicit VTR format"
            " * fpga-interchage: Architecture expressed in the FPGA Interchange schema format\n")
        .default_value("vtr")
        .show_in(argparse::ShowIn::HELP_ONLY);

    file_grp.add_argument(args.CircuitFile, "--circuit_file")
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
            "           .attr  - Attribute on atom primitive\n"
            " * fpga-interchage: Logical netlist in FPGA Interchange schema format\n")
        .default_value("auto")
        .show_in(argparse::ShowIn::HELP_ONLY);

    file_grp.add_argument(args.NetFile, "--net_file")
        .help("Path to packed netlist file")
        .show_in(argparse::ShowIn::HELP_ONLY);

    file_grp.add_argument(args.FlatPlaceFile, "--flat_place_file")
        .help("Path to input flat placement file")
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
        .help("The routing resource graph file to load. "
              "The loaded routing resource graph overrides any routing architecture specified in the architecture file.")
        .metavar("RR_GRAPH_FILE")
        .show_in(argparse::ShowIn::HELP_ONLY);

    file_grp.add_argument(args.read_rr_edge_override_file, "--read_rr_edge_override")
        .help("The routing resource edge attributes override file to load. "
              "This file overrides edge attributes in the routing resource graph. "
              "The user can use the architecture file to specify nominal switch delays, "
              "while this file can be used to override the nominal delays to make it more accurate "
              "for specific edges.")
        .show_in(argparse::ShowIn::HELP_ONLY);

    file_grp.add_argument(args.write_rr_graph_file, "--write_rr_graph")
        .help("Writes the routing resource graph to the specified file.")
        .metavar("RR_GRAPH_FILE")
        .show_in(argparse::ShowIn::HELP_ONLY);

    file_grp.add_argument(args.write_initial_place_file, "--write_initial_place_file")
        .help("Writes out the the placement chosen by the initial placement algorithm to the specified file.")
        .metavar("INITIAL_PLACE_FILE")
        .show_in(argparse::ShowIn::HELP_ONLY);

    file_grp.add_argument(args.read_initial_place_file, "--read_initial_place_file")
        .help("Reads the initial placement and continues the rest of the placement process from there.")
        .metavar("INITIAL_PLACE_FILE")
        .show_in(argparse::ShowIn::HELP_ONLY);

    file_grp.add_argument(args.read_vpr_constraints_file, "--read_vpr_constraints")
        .help("Reads the floorplanning constraints that packing and placement must respect from the specified XML file.")
        .show_in(argparse::ShowIn::HELP_ONLY);

    file_grp.add_argument(args.write_vpr_constraints_file, "--write_vpr_constraints")
        .help("Writes out new floorplanning constraints based on current placement to the specified XML file.")
        .show_in(argparse::ShowIn::HELP_ONLY);

    file_grp.add_argument(args.write_constraints_file, "--write_fix_clusters")
        .help(
            "Output file containing fixed locations of legalized input clusters - does not include clusters without placement coordinates; this file is used during post-legalization placement in order to hold input placement coordinates fixed while VPR places legalizer-generated orphan clusters.")
        .default_value("fix_clusters.out")
        .show_in(argparse::ShowIn::HELP_ONLY);

    file_grp.add_argument(args.read_flat_place_file, "--read_flat_place")
        .help(
            "Reads VPR's (or reconstructed external) placement solution in flat placement file format; this file lists cluster and intra-cluster placement coordinates for each atom and can be used to reconstruct a clustering and placement solution.")
        .show_in(argparse::ShowIn::HELP_ONLY);

    file_grp.add_argument(args.write_flat_place_file, "--write_flat_place")
        .help(
            "VPR's (or reconstructed external) placement solution in flat placement file format; this file lists cluster and intra-cluster placement coordinates for each atom and can be used to reconstruct a clustering and placement solution.")
        .show_in(argparse::ShowIn::HELP_ONLY);

    file_grp.add_argument(args.read_router_lookahead, "--read_router_lookahead")
        .help(
            "Reads the lookahead data from the specified file instead of computing it.")
        .show_in(argparse::ShowIn::HELP_ONLY);

    file_grp.add_argument(args.read_intra_cluster_router_lookahead, "--read_intra_cluster_router_lookahead")
        .help("Reads the intra-cluster lookahead data from the specified file.")
        .show_in(argparse::ShowIn::HELP_ONLY);

    file_grp.add_argument(args.write_router_lookahead, "--write_router_lookahead")
        .help("Writes the lookahead data to the specified file.")
        .show_in(argparse::ShowIn::HELP_ONLY);

    file_grp.add_argument(args.write_intra_cluster_router_lookahead, "--write_intra_cluster_router_lookahead")
        .help("Writes the intra-cluster lookahead data to the specified file.")
        .show_in(argparse::ShowIn::HELP_ONLY);

    file_grp.add_argument(args.read_placement_delay_lookup, "--read_placement_delay_lookup")
        .help(
            "Reads the placement delay lookup from the specified file instead of computing it.")
        .show_in(argparse::ShowIn::HELP_ONLY);

    file_grp.add_argument(args.write_placement_delay_lookup, "--write_placement_delay_lookup")
        .help("Writes the placement delay lookup to the specified file.")
        .show_in(argparse::ShowIn::HELP_ONLY);

    file_grp.add_argument(args.out_file_prefix, "--outfile_prefix")
        .help("Prefix for output files")
        .show_in(argparse::ShowIn::HELP_ONLY);

    file_grp.add_argument(args.write_block_usage, "--write_block_usage")
        .help("Writes the cluster-level block types usage summary to the specified JSON, XML or TXT file.")
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

    auto& ap_grp = parser.add_argument_group("analytical placement options");

    ap_grp.add_argument<e_ap_analytical_solver, ParseAPAnalyticalSolver>(args.ap_analytical_solver, "--ap_analytical_solver")
        .help(
            "Controls which Analytical Solver the Global Placer will use in the AP Flow.\n"
            " * qp-hybrid: olves for a placement that minimizes the quadratic HPWL of the flat placement using a hybrid clique/star net model.\n"
            " * lp-b2b: Solves for a placement that minimizes the linear HPWL of theflat placement using the Bound2Bound net model.")
        .default_value("lp-b2b")
        .show_in(argparse::ShowIn::HELP_ONLY);

    ap_grp.add_argument<e_ap_partial_legalizer, ParseAPPartialLegalizer>(args.ap_partial_legalizer, "--ap_partial_legalizer")
        .help(
            "Controls which Partial Legalizer the Global Placer will use in the AP Flow.\n"
            " * bipartitioning: Creates minimum windows around over-dense regions of the device bi-partitions the atoms in these windows such that the region is no longer over-dense and the atoms are in tiles that they can be placed into.\n"
            " * flow-based: Flows atoms from regions that are overfilled to regions that are underfilled.")
        .default_value("bipartitioning")
        .show_in(argparse::ShowIn::HELP_ONLY);

    ap_grp.add_argument<e_ap_full_legalizer, ParseAPFullLegalizer>(args.ap_full_legalizer, "--ap_full_legalizer")
        .help(
            "Controls which Full Legalizer to use in the AP Flow.\n"
            " * naive: Use a Naive Full Legalizer which will try to create clusters exactly where their atoms are placed.\n"
            " * appack: Use APPack, which takes the Packer in VPR and uses the flat atom placement to create better clusters.\n"
            " * basic-min-disturbance: Use the Basic Min. Disturbance Full Legalizer which tries to reconstruct a clustered placement that is as close to the incoming flat placement as possible.")
        .default_value("appack")
        .show_in(argparse::ShowIn::HELP_ONLY);

    ap_grp.add_argument<e_ap_detailed_placer, ParseAPDetailedPlacer>(args.ap_detailed_placer, "--ap_detailed_placer")
        .help(
            "Controls which Detailed Placer to use in the AP Flow.\n"
            " * none: Do not perform any detailed placement. i.e. the output of the full legalizer will be produced by the AP flow without modification.\n"
            " * annealer: Use the Annealer from the Placement stage as a Detailed Placer. This will use the same Placer Options from the Place stage to configure the annealer.")
        .default_value("annealer")
        .show_in(argparse::ShowIn::HELP_ONLY);

    ap_grp.add_argument<float>(args.ap_timing_tradeoff, "--ap_timing_tradeoff")
        .help(
            "Controls the trade-off between wirelength (HPWL) and delay minimization in the AP flow.\n"
            "A value of 0.0 makes the AP flow focus completely on wirelength minimization, while a value of 1.0 makes the AP flow focus completely on timing optimization.")
        .default_value("0.5")
        .show_in(argparse::ShowIn::HELP_ONLY);

    ap_grp.add_argument<int>(args.ap_high_fanout_threshold, "--ap_high_fanout_threshold")
        .help(
            "Defines the threshold for high fanout nets within AP flow.\n"
            "Ignores the nets that have higher fanouts than the threshold for the analytical solver.")
        .default_value("256")
        .show_in(argparse::ShowIn::HELP_ONLY);

    ap_grp.add_argument(args.ap_partial_legalizer_target_density, "--ap_partial_legalizer_target_density")
        .help(
            "Sets the target density of different physical tiles on the FPGA device "
            "for the partial legalizer in the AP flow. The partial legalizer will "
            "try to fill tiles up to (but not beyond) this target density. This "
            "is used as a guide, the legalizer may not follow this if it must fill "
            "the tile more."
            "\n"
            "When this option is set ot auto, VPR will select good values for the "
            "target density of tiles."
            "\n"
            "This option is similar to appack_max_dist_th, where a regex string "
            "is used to set the target density of different physical tiles. See "
            "the documentation for more information.")
        .nargs('+')
        .default_value({"auto"})
        .show_in(argparse::ShowIn::HELP_ONLY);

    ap_grp.add_argument(args.appack_max_dist_th, "--appack_max_dist_th")
        .help(
            "Sets the maximum candidate distance thresholds for the logical block types"
            "used by APPack. APPack uses the primitive-level placement produced by the"
            "global placer to cluster primitives together. APPack uses the thresholds"
            "here to ignore primitives which are too far away from the cluster being formed."
            "\n"
            "When this option is set to auto, VPR will select good values for these"
            "thresholds based on the primitives contained within each logical block type."
            "\n"
            "Using this option, the user can set the maximum candidate distance threshold"
            "of logical block types to something else. The strings passed in by the user"
            "should be of the form <regex>:<float>,<float> where the regex string is"
            "used to match the name of the logical block type to set, the first float"
            "is a scaling term, and the second float is an offset. The threshold will"
            "be set to max(scale * (W + H), offset), where W and H are the width and height"
            "of the device. This allows the user to specify a threshold based on the"
            "size of the device, while also preventing the number from going below offset"
            "When multiple strings are provided, the thresholds are set from left to right,"
            "and any logical block types which have been unset will be set to their auto"
            "values.")
        .nargs('+')
        .default_value({"auto"})
        .show_in(argparse::ShowIn::HELP_ONLY);

    ap_grp.add_argument<int>(args.ap_verbosity, "--ap_verbosity")
        .help(
            "Controls how verbose the AP flow's log messages will be. Higher "
            "values produce more output (useful for debugging the AP "
            "algorithms).")
        .default_value("1")
        .show_in(argparse::ShowIn::HELP_ONLY);

    ap_grp.add_argument<bool, ParseOnOff>(args.ap_generate_mass_report, "--ap_generate_mass_report")
        .help(
            "Controls whether to generate a report on how the partial legalizer "
            "within the AP flow calculates the mass of primitives and the "
            "capacity of tiles on the device. This report is useful when "
            "debugging the partial legalizer.")
        .default_value("off")
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

    pack_grp.add_argument(args.timing_gain_weight, "--timing_gain_weight")
        .help(
            "Parameter that weights the optimization of timing vs area. 0.0 focuses solely on"
            " area, 1.0 solely on timing.")
        .default_value("0.75")
        .show_in(argparse::ShowIn::HELP_ONLY);

    pack_grp.add_argument(args.connection_gain_weight, "--connection_gain_weight")
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

    pack_grp.add_argument(args.pack_feasible_block_array_size, "--pack_feasible_block_array_size")
        .help(
            "This value is used to determine the max size of the\n"
            "priority queue for candidates that pass the early filter\n"
            "legality test but not the more detailed routing test\n")
        .default_value("30")
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

    place_grp.add_argument<e_place_delta_delay_algorithm, ParsePlaceDeltaDelayAlgorithm>(
                 args.place_delta_delay_matrix_calculation_method,
                 "--place_delta_delay_matrix_calculation_method")
        .help(
            "What algorithm should be used to compute the place delta matrix.\n"
            "\n"
            " * astar : Find delta delays between OPIN's and IPIN's using\n"
            "           the router with the current --router_profiler_astar_fac.\n"
            " * dijkstra : Use Dijkstra's algorithm to find all shortest paths \n"
            "              from sampled OPIN's to all IPIN's.\n")
        .default_value("astar")
        .show_in(argparse::ShowIn::HELP_ONLY);

    place_grp.add_argument(args.PlaceInnerNum, "--inner_num")
        .help("Controls number of moves per temperature: inner_num * num_blocks ^ (4/3)")
        .default_value("0.5")
        .show_in(argparse::ShowIn::HELP_ONLY);

    place_grp.add_argument<e_place_effort_scaling, ParsePlaceEfforScaling>(args.place_effort_scaling, "--place_effort_scaling")
        .help(
            "Controls how the number of placer moves level scales with circuit\n"
            " and device size:\n"
            "  * circuit: proportional to circuit size (num_blocks ^ 4/3)\n"
            "  * device_circuit: proportional to device and circuit size\n"
            "                    (grid_size ^ 2/3 * num_blocks ^ 2/3)\n")
        .default_value("circuit")
        .show_in(argparse::ShowIn::HELP_ONLY);

    place_grp.add_argument(args.place_auto_init_t_scale, "--anneal_auto_init_t_scale")
        .help(
            "A scale on the starting temperature of the anneal for the automatic annealing "
            "schedule.\n"
            "\n"
            "When in the automatic annealing schedule, the annealer will select a good "
            "initial temperature based on the quality of the initial placement. This option "
            "allows you to scale that initial temperature up or down by multiplying the "
            "initial temperature by the given scale. Increasing this number "
            "will increase the initial temperature which will have the annealer potentially "
            "explore more of the space at the expense of run time. Depending on the quality "
            "of the initial placement, this may improve or hurt the quality of the final "
            "placement.")
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

    place_grp.add_argument<e_pad_loc_type, ParseFixPins>(args.pad_loc_type, "--fix_pins")
        .help(
            "Fixes I/O pad locations randomly during placement. Valid options:\n"
            " * 'free' allows placement to optimize pad locations\n"
            " * 'random' fixes pad locations to arbitrary locations\n.")
        .default_value("free")
        .choices({"free", "random"})
        .show_in(argparse::ShowIn::HELP_ONLY);

    place_grp.add_argument(args.constraints_file, "--fix_clusters")
        .help(
            "Fixes block locations during placement. Valid options:\n"
            " * path to a file specifying block locations (.place format with block locations specified).")
        .default_value("")
        .show_in(argparse::ShowIn::HELP_ONLY);

    place_grp.add_argument<e_place_algorithm, ParsePlaceAlgorithm>(args.PlaceAlgorithm, "--place_algorithm")
        .help(
            "Controls which placement algorithm is used. Valid options:\n"
            " * bounding_box: Focuses purely on minimizing the bounding box wirelength of the circuit. Turns off timing analysis if specified.\n"
            " * criticality_timing: Focuses on minimizing both the wirelength and the connection timing costs (criticality * delay).\n"
            " * slack_timing: Focuses on improving the circuit slack values to reduce critical path delay.\n")
        .default_value("criticality_timing")
        .choices({"bounding_box", "criticality_timing", "slack_timing"})
        .show_in(argparse::ShowIn::HELP_ONLY);

    place_grp.add_argument<e_place_algorithm, ParsePlaceAlgorithm>(args.PlaceQuenchAlgorithm, "--place_quench_algorithm")
        .help(
            "Controls which placement algorithm is used during placement quench.\n"
            "If specified, it overrides the option --place_algorithm during placement quench.\n"
            "Valid options:\n"
            " * bounding_box: Focuses purely on minimizing the bounding box wirelength of the circuit. Turns off timing analysis if specified.\n"
            " * criticality_timing: Focuses on minimizing both the wirelength and the connection timing costs (criticality * delay).\n"
            " * slack_timing: Focuses on improving the circuit slack values to reduce critical path delay.\n")
        .default_value("criticality_timing")
        .choices({"bounding_box", "criticality_timing", "slack_timing"})
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

    place_grp.add_argument(args.place_move_stats_file, "--place_move_stats")
        .help(
            "File to write detailed placer move statistics to")
        .default_value("")
        .show_in(argparse::ShowIn::HELP_ONLY);

    place_grp.add_argument(args.placement_saves_per_temperature, "--save_placement_per_temperature")
        .help(
            "Controls how often VPR saves the current placement to a file per temperature (may be helpful for debugging)."
            " The value specifies how many times the placement should be saved (values less than 1 disable this feature).")
        .default_value("0")
        .show_in(argparse::ShowIn::HELP_ONLY);

    place_grp.add_argument(args.place_static_move_prob, "--place_static_move_prob")
        .help(
            "The percentage probabilities of different moves in Simulated Annealing placement. "
            "For non-timing-driven placement, only the first 3 probabilities should be provided. "
            "For timing-driven placement, all probabilities should be provided. "
            "When the number of provided probabilities is less then the number of move types, zero probability "
            "is assumed."
            "The numbers listed are interpreted as the percentage probabilities of {UniformMove, MedianMove, CentroidMove, "
            "WeightedCentroid, WeightedMedian, Critical UniformMove, Timing feasible Region(TFR)}, in that order.")
        .nargs('+')
        .default_value({"100"})
        .show_in(argparse::ShowIn::HELP_ONLY);

    place_grp.add_argument(args.place_high_fanout_net, "--place_high_fanout_net")
        .help(
            "Sets the assumed high fanout net during placement. "
            "Any net with higher fanout would be ignored while calculating some of the directed moves: Median and WeightedMedian")
        .default_value("10")
        .show_in(argparse::ShowIn::HELP_ONLY);

    place_grp.add_argument<e_place_bounding_box_mode, ParsePlaceBoundingBox>(args.place_bounding_box_mode, "--place_bounding_box_mode")
        .help(
            "Specifies the type of bounding box to be used in 3D architectures.\n"
            "\n"
            "MODE options:\n"
            "  auto_bb     : Automatically determine the appropriate bounding box based on the connections between layers.\n"
            "  cube_bb            : Use 3D bounding boxes.\n"
            "  per_layer_bb     : Use per-layer bounding boxes.\n"
            "\n"
            "Choose one of the available modes to define the behavior of bounding boxes in your 3D architecture. The default mode is 'automatic'.")
        .default_value("auto_bb")
        .choices({"auto_bb", "cube_bb", "per_layer_bb"})
        .show_in(argparse::ShowIn::HELP_ONLY);

    place_grp.add_argument<bool, ParseOnOff>(args.RL_agent_placement, "--RL_agent_placement")
        .help(
            "Uses a Reinforcement Learning (RL) agent in choosing the appropriate move type in placement."
            "It activates the RL agent placement instead of using fixed probability for each move type.")
        .default_value("on")
        .show_in(argparse::ShowIn::HELP_ONLY);

    place_grp.add_argument<bool, ParseOnOff>(args.place_agent_multistate, "--place_agent_multistate")
        .help(
            "Enable multistate agent. "
            "A second state will be activated late in the annealing and in the Quench that includes all the timing driven directed moves.")
        .default_value("on")
        .show_in(argparse::ShowIn::HELP_ONLY);

    place_grp.add_argument<bool, ParseOnOff>(args.place_checkpointing, "--place_checkpointing")
        .help(
            "Enable Placement checkpoints. This means saving the placement and restore it if it's better than later placements."
            "Only effective if agent's 2nd state is activated.")
        .default_value("on")
        .show_in(argparse::ShowIn::HELP_ONLY);

    place_grp.add_argument(args.place_agent_epsilon, "--place_agent_epsilon")
        .help(
            "Placement RL agent's epsilon for epsilon-greedy agent."
            "Epsilon represents the percentage of exploration actions taken vs the exploitation ones.")
        .default_value("0.3")
        .show_in(argparse::ShowIn::HELP_ONLY);

    place_grp.add_argument(args.place_agent_gamma, "--place_agent_gamma")
        .help(
            "Controls how quickly the agent's memory decays. "
            "Values between [0., 1.] specify the fraction of weight in the exponentially weighted reward average applied to moves which occurred greater than moves_per_temp moves ago."
            "Values < 0 cause the unweighted reward sample average to be used (all samples are weighted equally)")
        .default_value("0.05")
        .show_in(argparse::ShowIn::HELP_ONLY);

    place_grp.add_argument(args.place_dm_rlim, "--place_dm_rlim")
        .help(
            "The maximum range limit of any directed move other than the uniform move. "
            "It also shrinks with the default rlim")
        .default_value("3.0")
        .show_in(argparse::ShowIn::HELP_ONLY);

    place_grp.add_argument(args.place_reward_fun, "--place_reward_fun")
        .help(
            "The reward function used by placement RL agent."
            "The available values are: basic, nonPenalizing_basic, runtime_aware, WLbiased_runtime_aware"
            "The latter two are only available for timing-driven placement.")
        .default_value("WLbiased_runtime_aware")
        .show_in(argparse::ShowIn::HELP_ONLY);

    place_grp.add_argument(args.place_crit_limit, "--place_crit_limit")
        .help(
            "The criticality limit to count a block as a critical one (or have a critical connection). "
            "It used in some directed moves that only move critical blocks like critical uniform and feasible region. "
            "Its range equals to [0., 1.].")
        .default_value("0.7")
        .show_in(argparse::ShowIn::HELP_ONLY);

    place_grp.add_argument(args.place_constraint_expand, "--place_constraint_expand")
        .help(
            "The value used to decide how much to expand the floorplan constraint region when writing "
            "a floorplan constraint XML file. Takes in an integer value from zero to infinity. "
            "If the value is zero, the block stays at the same x, y location. If it is "
            "greater than zero the constraint region expands by the specified value in each direction. "
            "For example, if 1 was specified, a block at the x, y location (1, 1) would have a constraint region "
            "of 2x2 centered around (1, 1), from (0, 0) to (2, 2).")
        .default_value("0")
        .show_in(argparse::ShowIn::HELP_ONLY);

    place_grp.add_argument<bool, ParseOnOff>(args.place_constraint_subtile, "--place_constraint_subtile")
        .help(
            "The bool used to say whether to print subtile constraints when printing a floorplan constraints XML file. "
            "If it is off, no subtile locations are specified when printing the floorplan constraints. "
            "If it is on, the floorplan constraints are printed with the subtiles from current placement. ")
        .default_value("off")
        .show_in(argparse::ShowIn::HELP_ONLY);

    place_grp.add_argument(args.floorplan_num_horizontal_partitions, "--floorplan_num_horizontal_partitions")
        .help(
            "An argument used for generating test constraints files. Specifies how many partitions to "
            "make in the horizontal dimension. Must be used in conjunction with "
            "--floorplan_num_vertical_partitions")
        .default_value("0")
        .show_in(argparse::ShowIn::HELP_ONLY);

    place_grp.add_argument(args.floorplan_num_vertical_partitions, "--floorplan_num_vertical_partitions")
        .help(
            "An argument used for generating test constraints files. Specifies how many partitions to "
            "make in the vertical dimension. Must be used in conjunction with "
            "--floorplan_num_horizontal_partitions")
        .default_value("0")
        .show_in(argparse::ShowIn::HELP_ONLY);

    place_grp.add_argument<bool, ParseOnOff>(args.place_quench_only, "--place_quench_only")
        .help(
            "Skip the placement annealing phase and go straight to the placement quench.")
        .default_value("off")
        .show_in(argparse::ShowIn::HELP_ONLY);

    place_grp.add_argument<e_agent_algorithm, ParsePlaceAgentAlgorithm>(args.place_agent_algorithm, "--place_agent_algorithm")
        .help("Controls which placement RL agent is used")
        .default_value("softmax")
        .choices({"e_greedy", "softmax"})
        .show_in(argparse::ShowIn::HELP_ONLY);

    place_grp.add_argument<e_agent_space, ParsePlaceAgentSpace>(args.place_agent_space, "--place_agent_space")
        .help(
            "Agent exploration space can be either based on only move types or also consider different block types\n"
            "The available values are: move_type, move_block_type")
        .default_value("move_block_type")
        .choices({"move_type", "move_block_type"})
        .show_in(argparse::ShowIn::HELP_ONLY);

    place_grp.add_argument(args.placer_debug_block, "--placer_debug_block")
        .help(
            " Controls when placer debugging is enabled for blocks.\n"
            " * For values >= 0, the value is taken as the block ID for\n"
            "   which to enable placer debug output.\n"
            " * For value == -1, placer debug output is enabled for\n"
            "   all blocks.\n"
            " * For values < -1, all block-based placer debug output is disabled.\n"
            "Note if VPR as compiled without debug logging enabled this will produce only limited output.\n")
        .default_value("-2")
        .show_in(argparse::ShowIn::HELP_ONLY);

    place_grp.add_argument(args.placer_debug_net, "--placer_debug_net")
        .help(
            "Controls when placer debugging is enabled for nets.\n"
            " * For values >= 0, the value is taken as the net ID for\n"
            "   which to enable placer debug output.\n"
            " * For value == -1, placer debug output is enabled for\n"
            "   all nets.\n"
            " * For values < -1, all net-based placer debug output is disabled.\n"
            "Note if VPR as compiled without debug logging enabled this will produce only limited output.\n")
        .default_value("-2")
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
        .help("Controls how many timing analyses are performed per temperature during placement")
        .default_value("0")
        .show_in(argparse::ShowIn::HELP_ONLY);

    place_timing_grp.add_argument(args.quench_recompute_divider, "--quench_recompute_divider")
        .help(
            "Controls how many timing analyses are performed during the final placement quench (t=0)."
            " If unspecified, uses the value from --inner_loop_recompute_divider")
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
            " * 'simple' uses map router lookahead\n"
            " * 'delta' uses differences in position only\n"
            " * 'delta_override' uses differences in position with overrides for direct connects\n")
        .default_value("simple")
        .show_in(argparse::ShowIn::HELP_ONLY);

    place_timing_grp.add_argument<e_reducer, ParseReducer>(args.place_delay_model_reducer, "--place_delay_model_reducer")
        .help("When calculating delta delays for the placement delay model how are multiple values combined?")
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
        .help("Name of the post-placement timing report file (not generated if unspecified)")
        .default_value("")
        .show_in(argparse::ShowIn::HELP_ONLY);

    place_timing_grp.add_argument(args.allowed_tiles_for_delay_model, "--allowed_tiles_for_delay_model")
        .help(
            "Names of allowed tile types that can be sampled during delay "
            "modelling.  Default is to allow all tiles. Can be used to "
            "exclude specialized tiles from placer delay sampling.")
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

    route_grp.add_argument(args.max_pres_fac, "-max_pres_fac")
        .help("Sets the maximum present overuse penalty factor")
        .default_value("1000.0")
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
            " * delay_normalized_length_bounded: like delay_normalized but\n"
            "      scaled by routing resource length.  Scaling is normalized\n"
            "      between 1 to 4, with min lengths getting scaled at 1,\n"
            "      and max lengths getting scaled at 4.\n"
            " * delay_normalized_frequency: like delay_normalized\n"
            "      but scaled inversely by segment type frequency\n"
            " * delay_normalized_length_frequency: like delay_normalized\n"
            "      but scaled by routing resource length, and inversely\n"
            "      by segment type frequency\n"
            "(Default: delay_normalized_length)")
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
            " * timing driven: focuses on routability and circuit speed [default]\n"
            " * parallel: timing_driven with nets in different regions of the chip routed in parallel\n"
            " * parallel_decomp: timing_driven with additional parallelism obtained by decomposing high-fanout nets, possibly reducing quality\n"
            " * nested: parallel with parallelized path search\n")
        .default_value("timing_driven")
        .choices({"nested", "parallel", "parallel_decomp", "timing_driven"})
        .show_in(argparse::ShowIn::HELP_ONLY);

    route_grp.add_argument(args.min_incremental_reroute_fanout, "--min_incremental_reroute_fanout")
        .help("The net fanout threshold above which nets will be re-routed incrementally.")
        .default_value("16")
        .show_in(argparse::ShowIn::HELP_ONLY);

    route_grp.add_argument<bool, ParseOnOff>(args.exit_after_first_routing_iteration, "--exit_after_first_routing_iteration")
        .help("Causes VPR to exit after the first routing iteration (useful for saving graphics)")
        .default_value("off")
        .show_in(argparse::ShowIn::HELP_ONLY);

    route_grp.add_argument(args.max_logged_overused_rr_nodes, "--max_logged_overused_rr_nodes")
        .help("Maximum number of overused RR nodes logged each time the routing fails")
        .default_value("20")
        .show_in(argparse::ShowIn::HELP_ONLY);

    route_grp.add_argument<bool, ParseOnOff>(args.generate_rr_node_overuse_report, "--generate_rr_node_overuse_report")
        .help("Generate detailed reports on overused rr nodes and congested nets should the routing fails")
        .default_value("off")
        .show_in(argparse::ShowIn::HELP_ONLY);

    route_grp.add_argument<e_rr_node_reorder_algorithm, ParseNodeReorderAlgorithm>(args.reorder_rr_graph_nodes_algorithm, "--reorder_rr_graph_nodes_algorithm")
        .help(
            "Specifies the node reordering algorithm to use.\n"
            " * none: don't reorder nodes\n"
            " * degree_bfs: sort by degree and then by BFS\n"
            " * random_shuffle: a random shuffle\n")
        .default_value("none")
        .choices({"none", "degree_bfs", "random_shuffle"})
        .show_in(argparse::ShowIn::HELP_ONLY);

    route_grp.add_argument(args.reorder_rr_graph_nodes_threshold, "--reorder_rr_graph_nodes_threshold")
        .help(
            "Reorder rr_graph nodes to optimize memory layout above this number of nodes.")
        .default_value("0")
        .show_in(argparse::ShowIn::HELP_ONLY);

    route_grp.add_argument(args.reorder_rr_graph_nodes_seed, "--reorder_rr_graph_nodes_seed")
        .help(
            "Pseudo-random number generator seed used for the random_shuffle reordering algorithm")
        .default_value("1")
        .show_in(argparse::ShowIn::HELP_ONLY);

    route_grp.add_argument<bool, ParseOnOff>(args.flat_routing, "--flat_routing")
        .help("Enable VPR's flat routing (routing the nets from the source primitive to the destination primitive)")
        .default_value("off")
        .show_in(argparse::ShowIn::HELP_ONLY);

    route_grp.add_argument<bool, ParseOnOff>(args.router_opt_choke_points, "--router_opt_choke_points")
        .help(
            ""
            "Some FPGA architectures with limited fan-out options within a cluster (e.g. fracturable LUTs with shared pins) do"
            " not converge well in routing unless these fan-out choke points are discovered and optimized for during net routing."
            " This option helps router convergence for such architectures.")
        .default_value("on")
        .show_in(argparse::ShowIn::HELP_ONLY);

    route_grp.add_argument<int>(args.route_verbosity, "--route_verbosity")
        .help("Controls the verbosity of routing's output. Higher values produce more output (useful for debugging routing problems)")
        .default_value("1")
        .show_in(argparse::ShowIn::HELP_ONLY);
    route_grp.add_argument(args.custom_3d_sb_fanin_fanout, "--custom_3d_sb_fanin_fanout")
        .help(
            "Specifies the number of tracks that can drive a 3D switch block connection"
            "and the number of tracks that can be driven by a 3D switch block connection")
        .default_value("1")
        .show_in(argparse::ShowIn::HELP_ONLY);

    auto& route_timing_grp = parser.add_argument_group("timing-driven routing options");

    route_timing_grp.add_argument(args.astar_fac, "--astar_fac")
        .help(
            "Controls the directedness of the timing-driven router's exploration."
            " Values between 1 and 2 are resonable; higher values trade some quality for reduced run-time")
        .default_value("1.2")
        .show_in(argparse::ShowIn::HELP_ONLY);

    route_timing_grp.add_argument(args.astar_offset, "--astar_offset")
        .help(
            "Controls the directedness of the timing-driven router's exploration."
            " It is a subtractive adjustment to the lookahead heuristic."
            " Values between 0 and 1e-9 are resonable; higher values may increase quality at the expense of run-time.")
        .default_value("0.0")
        .show_in(argparse::ShowIn::HELP_ONLY);

    route_timing_grp.add_argument(args.router_profiler_astar_fac, "--router_profiler_astar_fac")
        .help(
            "Controls the directedness of the timing-driven router's exploration"
            " when doing router delay profiling of an architecture."
            " The router delay profiling step is currently used to calculate the place delay matrix lookup."
            " Values between 1 and 2 are resonable; higher values trade some quality for reduced run-time")
        .default_value("1.2")
        .show_in(argparse::ShowIn::HELP_ONLY);

    route_timing_grp.add_argument<bool, ParseOnOff>(args.enable_parallel_connection_router, "--enable_parallel_connection_router")
        .help(
            "Controls whether the MultiQueue-based parallel connection router is used during a single connection"
            " routing. When enabled, the parallel connection router accelerates the path search for individual"
            " source-sink connections using multi-threading without altering the net routing order.")
        .default_value("off")
        .show_in(argparse::ShowIn::HELP_ONLY);

    route_timing_grp.add_argument(args.post_target_prune_fac, "--post_target_prune_fac")
        .help(
            "Controls the post-target pruning heuristic calculation in the parallel connection router."
            " This parameter is used as a multiplicative factor applied to the VPR heuristic"
            " (not guaranteed to be admissible, i.e., might over-predict the cost to the sink)"
            " to calculate the 'stopping heuristic' when pruning nodes after the target has been"
            " reached. The 'stopping heuristic' must be admissible for the path search algorithm"
            " to guarantee optimal paths and be deterministic. Values of this parameter are"
            " architecture-specific and have to be empirically found."
            " This parameter has no effect if --enable_parallel_connection_router is not set.")
        .default_value("1.2")
        .show_in(argparse::ShowIn::HELP_ONLY);

    route_timing_grp.add_argument(args.post_target_prune_offset, "--post_target_prune_offset")
        .help(
            "Controls the post-target pruning heuristic calculation in the parallel connection router."
            " This parameter is used as a subtractive offset together with --post_target_prune_fac"
            " to apply an affine transformation on the VPR heuristic to calculate the 'stopping"
            " heuristic'. The 'stopping heuristic' must be admissible for the path search"
            " algorithm to guarantee optimal paths and be deterministic. Values of this"
            " parameter are architecture-specific and have to be empirically found."
            " This parameter has no effect if --enable_parallel_connection_router is not set.")
        .default_value("0.0")
        .show_in(argparse::ShowIn::HELP_ONLY);

    route_timing_grp.add_argument<int>(args.multi_queue_num_threads, "--multi_queue_num_threads")
        .help(
            "Controls the number of threads used by MultiQueue-based parallel connection router."
            " If not explicitly specified, defaults to 1, implying the parallel connection router"
            " works in 'serial' mode using only one main thread to route."
            " This parameter has no effect if --enable_parallel_connection_router is not set.")
        .default_value("1")
        .show_in(argparse::ShowIn::HELP_ONLY);

    route_timing_grp.add_argument<int>(args.multi_queue_num_queues, "--multi_queue_num_queues")
        .help(
            "Controls the number of queues used by MultiQueue in the parallel connection router."
            " Must be set >= 2. A common configuration for this parameter is the number of threads"
            " used by MultiQueue * 4 (the number of queues per thread)."
            " This parameter has no effect if --enable_parallel_connection_router is not set.")
        .default_value("2")
        .show_in(argparse::ShowIn::HELP_ONLY);

    route_timing_grp.add_argument<bool, ParseOnOff>(args.multi_queue_direct_draining, "--multi_queue_direct_draining")
        .help(
            "Controls whether to enable queue draining optimization for MultiQueue-based parallel connection"
            " router. When enabled, queues can be emptied quickly by draining all elements if no further"
            " solutions need to be explored in the path search to guarantee optimality or determinism after"
            " reaching the target. This parameter has no effect if --enable_parallel_connection_router is not set.")
        .default_value("off")
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
            "Controls how the routing budgets are created and applied.\n"
            " * yoyo: Allocates budgets using minimax algorithm, and enables hold slack resolution in the router using the RCV algorithm. [EXPERIMENTAL]\n"
            " * minimax: Sets the budgets depending on the amount slack between connections and the current delay values. [EXPERIMENTAL]\n"
            " * scale_delay: Sets the minimum budgets to 0 and the maximum budgets as a function of delay and criticality (net delay/ pin criticality) [EXPERIMENTAL]\n"
            " * disable: Removes the routing budgets, use the default VPR and ignore hold time constraints\n")
        .default_value("disable")
        .choices({"minimax", "scale_delay", "yoyo", "disable"})
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

    route_timing_grp.add_argument<float>(args.router_high_fanout_max_slope, "--router_high_fanout_max_slope")
        .help(
            "Minimum routing progress where high fanout routing is enabled."
            " This is a ratio of the actual congestion reduction to what is expected based in the history.\n"
            " 1.0 is normal progress, 0 is no progress.")
        .default_value("0.1")
        .show_in(argparse::ShowIn::HELP_ONLY);

    route_timing_grp.add_argument<e_router_lookahead, ParseRouterLookahead>(args.router_lookahead_type, "--router_lookahead")
        .help(
            "Controls what lookahead the router uses to calculate cost of completing a connection.\n"
            " * classic: The classic VPR lookahead (may perform better on un-buffered routing\n"
            "            architectures)\n"
            " * map: An advanced lookahead which accounts for diverse wire type\n"
            " * compressed_map: The algorithm is similar to map lookahead with the exception of sparse sampling of the chip"
            " to reduce the run-time to build the router lookahead and also its memory footprint\n"
            " * extended_map: A more advanced and extended lookahead which accounts for a more\n"
            "                 exhaustive node sampling method\n"
            "\n"
            " The extended map differs from the map lookahead in the lookahead computation.\n"
            " It is better suited for architectures that have specialized routing for specific\n"
            " kinds of connections, but note that the time and memory necessary to compute the\n"
            " extended lookahead map are greater than the basic lookahead map.\n")
        .default_value("map")
        .show_in(argparse::ShowIn::HELP_ONLY);

    route_timing_grp.add_argument<bool, ParseOnOff>(args.generate_router_lookahead_report, "--generate_router_lookahead_report")
        .help("If turned on, generates a detailed report on the router lookahead: report_router_lookahead.rpt\n"
              "\n"
              "This report contains information on how accurate the router lookahead is and "
              "if and when it overestimates the cost from a node to a target node. It does "
              "this by doing a set of trial routes and comparing the estimated cost from the "
              "router lookahead to the actual cost of the route path.")
        .default_value("off")
        .show_in(argparse::ShowIn::HELP_ONLY);

    route_timing_grp.add_argument<double>(args.router_initial_acc_cost_chan_congestion_threshold, "--router_initial_acc_cost_chan_congestion_threshold")
        .help("Utilization threshold above which initial accumulated routing cost (acc_cost) "
              "is increased to penalize congested channels. Used to bias routing away from "
              "highly utilized regions during early routing iterations.")
        .default_value("0.5")
        .show_in(argparse::ShowIn::HELP_ONLY);

    route_timing_grp.add_argument<double>(args.router_initial_acc_cost_chan_congestion_weight, "--router_initial_acc_cost_chan_congestion_weight")
        .help("Weight applied to the excess channel utilization (above threshold) "
              "when computing the initial accumulated cost (acc_cost)of routing resources. "
              "Higher values make the router more sensitive to early congestion.")
        .default_value("0.5")
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

    route_timing_grp.add_argument<e_router_initial_timing, ParseRouterFirstIterTiming>(args.router_initial_timing, "--router_initial_timing")
        .help(
            "Controls how criticality is determined at the start of the first routing iteration.\n"
            " * all_critical: All connections are considered timing\n"
            "                 critical.\n"
            " * lookahead   : Connection criticalities are determined\n"
            "                 from timing analysis assuming best-case\n"
            "                 connection delays as estimated by the\n"
            "                 router's lookahead.\n"
            "(Default: 'lookahead' if a non-classic router lookahead is\n"
            "           used, otherwise 'all_critical')\n")
        .default_value("lookahead")
        .show_in(argparse::ShowIn::HELP_ONLY);

    route_timing_grp.add_argument<bool, ParseOnOff>(args.router_update_lower_bound_delays, "--router_update_lower_bound_delays")
        .help("Controls whether the router updates lower bound connection delays after the 1st routing iteration.")
        .default_value("on")
        .show_in(argparse::ShowIn::HELP_ONLY);

    route_timing_grp.add_argument<e_heap_type, ParseRouterHeap>(args.router_heap, "--router_heap")
        .help(
            "Controls what type of heap to use for timing driven router.\n"
            " * binary: A binary heap is used.\n"
            " * four_ary: A four_ary heap is used.\n"
            " * bucket: A bucket heap approximation is used. The bucket heap\n"
            " *         is faster because it is only a heap approximation.\n"
            " *         Testing has shown the approximation results in\n"
            " *         similar QoR with less CPU work.\n")
        .default_value("four_ary")
        .show_in(argparse::ShowIn::HELP_ONLY);

    route_timing_grp.add_argument(args.router_first_iteration_timing_report_file, "--router_first_iter_timing_report")
        .help("Name of the post first routing iteration timing report file (not generated if unspecified)")
        .default_value("")
        .show_in(argparse::ShowIn::HELP_ONLY);

    route_timing_grp.add_argument<bool, ParseOnOff>(args.read_rr_edge_metadata, "--read_rr_edge_metadata")
        .help("Read RR edge metadata from --read_rr_graph.  RR edge metadata is not used in core VPR algorithms, and is typically not read to save runtime and memory. (Default: off).")
        .default_value("off")
        .show_in(argparse::ShowIn::HELP_ONLY);

    route_timing_grp.add_argument<e_check_route_option, ParseCheckRoute>(args.check_route, "--check_route")
        .help(
            "Options to run check route in three different modes.\n"
            " * off    : check route is completely disabled.\n"
            " * quick  : runs check route with slow checks disabled.\n"
            " * full   : runs the full check route step.\n")
        .default_value("full")
        .show_in(argparse::ShowIn::HELP_ONLY);

    route_timing_grp.add_argument(args.router_debug_net, "--router_debug_net")
        .help(
            "Controls when router debugging is enabled for nets.\n"
            " * For values >= 0, the value is taken as the net ID for\n"
            "   which to enable router debug output.\n"
            " * For value == -1, router debug output is enabled for\n"
            "   all nets.\n"
            " * For values < -1, all net-based router debug output is disabled.\n"
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

    route_timing_grp.add_argument(args.router_debug_iteration, "--router_debug_iteration")
        .help(
            "Controls when router debugging is enabled for the specific router iteration.\n"
            " * For values >= 0, the value is taken as the iteration number for\n"
            "   which to enable router debug output.\n"
            " * For values < 0, all iteration-based router debug output is disabled.\n"
            "Note if VPR as compiled without debug logging enabled this will produce only limited output.\n")
        .default_value("-2")
        .show_in(argparse::ShowIn::HELP_ONLY);

    route_timing_grp.add_argument<bool, ParseOnOff>(args.check_rr_graph, "--check_rr_graph")
        .help("Controls whether to check the rr graph when reading from disk.")
        .default_value("on")
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

    analysis_grp.add_argument<bool, ParseOnOff>(args.Generate_Post_Implementation_Merged_Netlist, "--gen_post_implementation_merged_netlist")
        .help(
            "Generates the post-implementation netlist with merged top module ports"
            " Used for post-implementation simulation and verification")
        .default_value("off")
        .show_in(argparse::ShowIn::HELP_ONLY);

    analysis_grp.add_argument<bool, ParseOnOff>(args.generate_post_implementation_sdc, "--gen_post_implementation_sdc")
        .help(
            "Generates an SDC file including a list of constraints that would "
            "replicate the timing constraints that the timing analysis within "
            "VPR followed during the flow. This can be helpful for flows that "
            "use external timing analysis tools that have additional capabilities "
            "or more detailed delay models than what VPR uses")
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
            " * aggregated: Like 'netlist', but also shows aggregated intra-block/inter-block delays\n"
            " * detailed: Like 'aggregated' but shows detailed routing instead of aggregated inter-block delays\n"
            " * debug: Like 'detailed' but shows additional tool internal debug information\n")
        .default_value("netlist")
        .show_in(argparse::ShowIn::HELP_ONLY);

    analysis_grp.add_argument<bool, ParseOnOff>(args.timing_report_skew, "--timing_report_skew")
        .help("Controls whether skew timing reports are generated\n")
        .default_value("off")
        .show_in(argparse::ShowIn::HELP_ONLY);

    analysis_grp.add_argument(args.echo_dot_timing_graph_node, "--echo_dot_timing_graph_node")
        .help(
            "Controls how the timing graph echo file in DOT/GraphViz format is created when\n"
            "'--echo_file on' is set:\n"
            " * -1: All nodes are dumped into the DOT file\n"
            " * >= 0: Only the transitive fanin/fanout of the node is dumped (easier to view)\n"
            " * a string: Interpretted as a VPR pin name which is converted to a node id, and dumped as above\n")
        .default_value("-1")
        .show_in(argparse::ShowIn::HELP_ONLY);

    analysis_grp.add_argument<e_post_synth_netlist_unconn_handling, ParsePostSynthNetlistUnconnInputHandling>(args.post_synth_netlist_unconn_input_handling, "--post_synth_netlist_unconn_inputs")
        .help(
            "Controls how unconnected input cell ports are handled in the post-synthesis netlist\n"
            " * unconnected: leave unconnected\n"
            " * nets: connect each unconnected input pin to its own separate\n"
            "         undriven net named: __vpr__unconn<ID>, where <ID> is index\n"
            "         assigned to this occurrence of unconnected port in design\n"
            " * gnd: tie all to ground (1'b0)\n"
            " * vcc: tie all to VCC (1'b1)\n")
        .default_value("unconnected")
        .show_in(argparse::ShowIn::HELP_ONLY);

    analysis_grp.add_argument<e_post_synth_netlist_unconn_handling, ParsePostSynthNetlistUnconnOutputHandling>(args.post_synth_netlist_unconn_output_handling, "--post_synth_netlist_unconn_outputs")
        .help(
            "Controls how unconnected output cell ports are handled in the post-synthesis netlist\n"
            " * unconnected: leave unconnected\n"
            " * nets: connect each unconnected input pin to its own separate\n"
            "         undriven net named: __vpr__unconn<ID>, where <ID> is index\n"
            "         assigned to this occurrence of unconnected port in design\n")
        .default_value("unconnected")
        .show_in(argparse::ShowIn::HELP_ONLY);

    analysis_grp.add_argument<bool, ParseOnOff>(args.post_synth_netlist_module_parameters, "--post_synth_netlist_module_parameters")
        .help(
            "Controls whether the post-synthesis netlist output by VTR can use Verilog parameters "
            "or not. When using the post-synthesis netlist for external timing analysis, "
            "some tools cannot accept the netlist if it contains parameters. By setting "
            "this option to off, VPR will try to represent the netlist using non-parameterized "
            "modules\n")
        .default_value("on")
        .show_in(argparse::ShowIn::HELP_ONLY);

    analysis_grp.add_argument(args.write_timing_summary, "--write_timing_summary")
        .help("Writes implemented design final timing summary to the specified JSON, XML or TXT file.")
        .show_in(argparse::ShowIn::HELP_ONLY);

    analysis_grp.add_argument<bool, ParseOnOff>(args.skip_sync_clustering_and_routing_results, "--skip_sync_clustering_and_routing_results")
        .help(
            "Select to skip the synchronization on clustering results based on routing optimization results."
            "Note that when this sync-up is disabled, clustering results may be wrong (leading to incorrect bitstreams)!")
        .default_value("off")
        .show_in(argparse::ShowIn::HELP_ONLY);

    analysis_grp.add_argument<bool, ParseOnOff>(args.generate_net_timing_report, "--generate_net_timing_report")
        .help(
            "Generates a net timing report in CSV format, reporting the delay and slack\n"
            "for every routed connection in the design.\n"
            "The report is saved as 'report_net_timing.csv'.")
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

    auto& noc_grp = parser.add_argument_group("noc options");

    noc_grp.add_argument<bool, ParseOnOff>(args.noc, "--noc")
        .help(
            "Enables a NoC-driven placer that optimizes the placement of routers on the NoC. "
            "Also enables an option in the graphical display that can be used to display the NoC on the FPGA. "
            "This should be on only when the FPGA device contains a NoC and the provided netlist connects to the NoC.")
        .default_value("off")
        .show_in(argparse::ShowIn::HELP_ONLY);

    noc_grp.add_argument<std::string>(args.noc_flows_file, "--noc_flows_file")
        .help(
            "XML file containing the list of traffic flows within the NoC (communication between routers)."
            "This is required if the --noc option is turned on.")
        .default_value("")
        .show_in(argparse::ShowIn::HELP_ONLY);

    noc_grp.add_argument<std::string>(args.noc_routing_algorithm, "--noc_routing_algorithm")
        .help(
            "Controls the algorithm used by the NoC to route packets.\n"
            "* xy_routing: Uses the direction oriented routing algorithm. This is recommended to be used with mesh NoC topologies.\n"
            "* bfs_routing: Uses the breadth first search algorithm. The objective is to find a route that uses a minimum number of links. "
            " This algorithm is not guaranteed to generate deadlock-free traffic flow routes, but can be used with any NoC topology\n"
            "* west_first_routing: Uses the west-first routing algorithm. This is recommended to be used with mesh NoC topologies.\n"
            "* north_last_routing: Uses the north-last routing algorithm. This is recommended to be used with mesh NoC topologies.\n"
            "* negative_first_routing: Uses the negative-first routing algorithm. This is recommended to be used with mesh NoC topologies.\n"
            "* odd_even_routing: Uses the odd-even routing algorithm. This is recommended to be used with mesh NoC topologies.\n")
        .default_value("bfs_routing")
        .choices({"xy_routing", "bfs_routing", "west_first_routing", "north_last_routing", "negative_first_routing",
                  "odd_even_routing"})
        .show_in(argparse::ShowIn::HELP_ONLY);

    noc_grp.add_argument<double>(args.noc_placement_weighting, "--noc_placement_weighting")
        .help(
            "Controls the importance of the NoC placement parameters relative to timing and wirelength of the design. "
            "This value can be >=0, where 0 would mean the placement is based solely on timing and wirelength. "
            "A value of 1 would mean noc placement is considered equal to timing and wirelength "
            "A value greater than 1 would mean the placement is increasingly dominated by NoC parameters.")
        .default_value("5.0")
        .show_in(argparse::ShowIn::HELP_ONLY);

    noc_grp.add_argument<double>(args.noc_agg_bandwidth_weighting, "--noc_aggregate_bandwidth_weighting")
        .help(
            "Controls the importance of minimizing the NoC aggregate bandwidth.\n"
            "This value can be >=0, where 0 would mean the aggregate bandwidth has no relevance to placement.\n"
            "Other positive numbers specify the importance of minimizing the NoC aggregate bandwidth to other NoC-related cost terms.\n"
            "Weighting factors for NoC-related cost terms are normalized internally. Therefore, their absolute values are not important, and "
            "only their relative ratios determine the importance of each cost term.")
        .default_value("0.38")
        .show_in(argparse::ShowIn::HELP_ONLY);

    noc_grp.add_argument<double>(args.noc_latency_constraints_weighting, "--noc_latency_constraints_weighting")
        .help(
            "Controls the importance of meeting all the NoC traffic flow latency constraints.\n"
            "This value can be >=0, where 0 would mean the latency constraints have no relevance to placement.\n"
            "Other positive numbers specify the importance of meeting latency constraints to other NoC-related cost terms.\n"
            "Weighting factors for NoC-related cost terms are normalized internally. Therefore, their absolute values are not important, and "
            "only their relative ratios determine the importance of each cost term.")
        .default_value("0.6")
        .show_in(argparse::ShowIn::HELP_ONLY);

    noc_grp.add_argument<double>(args.noc_latency_weighting, "--noc_latency_weighting")
        .help(
            "Controls the importance of reducing the latencies of the NoC traffic flows.\n"
            "This value can be >=0, where 0 would mean the latencies have no relevance to placement.\n"
            "Other positive numbers specify the importance of minimizing aggregate latency to other NoC-related cost terms.\n"
            "Weighting factors for NoC-related cost terms are normalized internally. Therefore, their absolute values are not important, and "
            "only their relative ratios determine the importance of each cost term.")
        .default_value("0.02")
        .show_in(argparse::ShowIn::HELP_ONLY);

    noc_grp.add_argument<double>(args.noc_congestion_weighting, "--noc_congestion_weighting")
        .help(
            "Controls the importance of reducing the congestion of the NoC links.\n"
            "This value can be >=0, where 0 would mean the congestion has no relevance to placement.\n"
            "Other positive numbers specify the importance of minimizing congestion to other NoC-related cost terms.\n"
            "Weighting factors for NoC-related cost terms are normalized internally. Therefore, their absolute values are not important, and "
            "only their relative ratios determine the importance of each cost term.")
        .default_value("0.25")
        .show_in(argparse::ShowIn::HELP_ONLY);

    noc_grp.add_argument<double>(args.noc_centroid_weight, "--noc_centroid_weight")
        .help(
            "Sets the minimum fraction of swaps attempted by the placer that are NoC blocks."
            "This value is an integer ranging from 0-100. 0 means NoC blocks will be moved at the same rate as other blocks. 100 means all swaps attempted by the placer are NoC router blocks.")
        .default_value("0")
        .show_in(argparse::ShowIn::HELP_ONLY);

    noc_grp.add_argument<double>(args.noc_swap_percentage, "--noc_swap_percentage")
        .help(
            "Sets the minimum fraction of swaps attempted by the placer that are NoC blocks. "
            "This value is an integer ranging from 0-100. 0 means NoC blocks will be moved at the same rate as other blocks. 100 means all swaps attempted by the placer are NoC router blocks.")
        .default_value("0")
        .show_in(argparse::ShowIn::HELP_ONLY);

    noc_grp.add_argument<int>(args.noc_sat_routing_bandwidth_resolution, "--noc_sat_routing_bandwidth_resolution")
        .help(
            "Specifies the resolution by which traffic flow bandwidths are converted into integers in SAT routing algorithm.\n"
            "The higher this number is, the more accurate the congestion estimation and aggregate bandwidth minimization is.\n"
            "Higher resolution for bandwidth conversion increases the number of variables in the SAT formulation.")
        .default_value("128")
        .show_in(argparse::ShowIn::HELP_ONLY);

    noc_grp.add_argument<int>(args.noc_sat_routing_latency_overrun_weighting_factor, "--noc_sat_routing_latency_overrun_weighting_factor")
        .help(
            "Controls the importance of reducing traffic flow latency overrun in SAT routing.")
        .default_value("1024")
        .show_in(argparse::ShowIn::HELP_ONLY);

    noc_grp.add_argument<int>(args.noc_sat_routing_congestion_weighting_factor, "--noc_sat_routing_congestion_weighting_factor")
        .help(
            "Controls the importance of reducing the number of congested NoC links in SAT routing.")
        .default_value("16384")
        .show_in(argparse::ShowIn::HELP_ONLY);

    noc_grp.add_argument<int>(args.noc_sat_routing_num_workers, "--noc_sat_routing_num_workers")
        .help(
            "The maximum number of parallel threads that the SAT solver can use to explore the solution space.\n"
            "If not explicitly specified by the user, VPR will set the number parallel SAT solver workers to the value "
            "specified by -j command line option.")
        .show_in(argparse::ShowIn::HELP_ONLY);

    noc_grp.add_argument<bool, ParseOnOff>(args.noc_sat_routing_log_search_progress, "--noc_sat_routing_log_search_progress")
        .help(
            "Print the detailed log of the SAT solver's search progress.")
        .default_value("off")
        .show_in(argparse::ShowIn::HELP_ONLY);

    noc_grp.add_argument<std::string>(args.noc_placement_file_name, "--noc_placement_file_name")
        .help(
            "Name of the output file that contains the NoC placement information."
            "The default name is 'vpr_noc_placement_output.txt'")
        .default_value("vpr_noc_placement_output.txt")
        .show_in(argparse::ShowIn::HELP_ONLY);

#ifndef NO_SERVER
    auto& server_grp = parser.add_argument_group("server options");

    server_grp.add_argument<bool, ParseOnOff>(args.is_server_mode_enabled, "--server")
        .help("Run in server mode."
              "Accept client application connection and respond to requests.")
        .action(argparse::Action::STORE_TRUE)
        .default_value("off");

    server_grp.add_argument<int>(args.server_port_num, "--port")
        .help("Server port number.")
        .default_value("60555")
        .show_in(argparse::ShowIn::HELP_ONLY);
#endif /* NO_SERVER */

    return parser;
}

void set_conditional_defaults(t_options& args) {
    //Some arguments are set conditionally based on other options.
    //These are resolved here.

    /*
     * Filenames
     */

    //We may have received the full circuit filepath in the circuit name,
    //remove the extension and any leading path elements
    VTR_ASSERT(args.CircuitName.provenance() == Provenance::SPECIFIED);
    auto name_ext = vtr::split_ext(args.CircuitName);

    if (args.CircuitFile.provenance() != Provenance::SPECIFIED) {
        //If the blif file wasn't explicitly specified, interpret the circuit name
        //as the blif file, and split off the extension
        args.CircuitName.set(vtr::basename(name_ext[0]), Provenance::SPECIFIED);
    }

    std::string default_output_name = args.CircuitName;

    if (args.CircuitFile.provenance() != Provenance::SPECIFIED) {
        //Use the full path specified in the original circuit name,
        //and append the expected .blif extension
        std::string blif_file = name_ext[0] + name_ext[1];
        args.CircuitFile.set(blif_file, Provenance::INFERRED);
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

    if (args.FlatPlaceFile.provenance() != Provenance::SPECIFIED) {
        std::string flat_place_file = args.out_file_prefix;
        flat_place_file += default_output_name + ".flat_place";
        args.FlatPlaceFile.set(flat_place_file, Provenance::INFERRED);
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
            args.PlaceAlgorithm.set(e_place_algorithm::CRITICALITY_TIMING_PLACE, Provenance::INFERRED);
        } else {
            args.PlaceAlgorithm.set(e_place_algorithm::BOUNDING_BOX_PLACE, Provenance::INFERRED);
        }
    }

    // If MAP Router lookahead is not used, we cannot use simple place delay lookup
    if (args.place_delay_model.provenance() != Provenance::SPECIFIED) {
        if (args.router_lookahead_type != e_router_lookahead::MAP) {
            args.place_delay_model.set(PlaceDelayModelType::DELTA, Provenance::INFERRED);
        }
    }

    // Check for correct options combinations
    // If you are running WLdriven placement, the RL reward function should be
    // either basic or nonPenalizing basic
    if (args.RL_agent_placement && (args.PlaceAlgorithm == e_place_algorithm::BOUNDING_BOX_PLACE || !args.timing_analysis)) {
        if (args.place_reward_fun.value() != "basic" && args.place_reward_fun.value() != "nonPenalizing_basic") {
            VTR_LOG_WARN(
                "To use RLPlace for WLdriven placements, the reward function should be basic or nonPenalizing_basic.\n"
                "you can specify the reward function using --place_reward_fun.\n"
                "Setting the placement reward function to \"basic\"\n");
            args.place_reward_fun.set("basic", Provenance::INFERRED);
        }
    }

    //Which placement algorithm to use during placement quench?
    if (args.PlaceQuenchAlgorithm.provenance() != Provenance::SPECIFIED) {
        args.PlaceQuenchAlgorithm.set(args.PlaceAlgorithm, Provenance::INFERRED);
    }

    //Place chan width follows Route chan width if unspecified
    if (args.PlaceChanWidth.provenance() != Provenance::SPECIFIED && args.RouteChanWidth.provenance() == Provenance::SPECIFIED) {
        args.PlaceChanWidth.set(args.RouteChanWidth.value(), Provenance::INFERRED);
    }

    //Do we calculate timing info during placement?
    if (args.ShowPlaceTiming.provenance() != Provenance::SPECIFIED) {
        args.ShowPlaceTiming.set(args.timing_analysis, Provenance::INFERRED);
    }

    //Slave quench recompute divider of inner loop recompute divider unless specified
    if (args.quench_recompute_divider.provenance() != Provenance::SPECIFIED) {
        args.quench_recompute_divider.set(args.inner_loop_recompute_divider, Provenance::INFERRED);
    }

    //Which schedule?
    if (args.PlaceInitT.provenance() == Provenance::SPECIFIED // Any of these flags select a manual schedule
        || args.PlaceExitT.provenance() == Provenance::SPECIFIED
        || args.PlaceAlphaT.provenance() == Provenance::SPECIFIED) {
        args.anneal_sched_type.set(e_sched_type::USER_SCHED, Provenance::INFERRED);
    } else {
        args.anneal_sched_type.set(e_sched_type::AUTO_SCHED, Provenance::INFERRED); // Otherwise use the automatic schedule
    }

    /*
     * Routing
     */
    //Base cost type
    if (args.base_cost_type.provenance() != Provenance::SPECIFIED) {
        if (args.RouteType == e_route_type::DETAILED) {
            if (args.timing_analysis) {
                args.base_cost_type.set(DELAY_NORMALIZED_LENGTH, Provenance::INFERRED);
            } else {
                args.base_cost_type.set(DEMAND_ONLY_NORMALIZED_LENGTH, Provenance::INFERRED);
            }
        } else {
            VTR_ASSERT(args.RouteType == e_route_type::GLOBAL);
            //Global RR graphs don't have valid timing, so use demand base cost
            args.base_cost_type.set(DEMAND_ONLY_NORMALIZED_LENGTH, Provenance::INFERRED);
        }
    }

    //Bend cost
    if (args.bend_cost.provenance() != Provenance::SPECIFIED) {
        if (args.RouteType == e_route_type::GLOBAL) {
            args.bend_cost.set(1., Provenance::INFERRED);
        } else {
            VTR_ASSERT(args.RouteType == e_route_type::DETAILED);
            args.bend_cost.set(0., Provenance::INFERRED);
        }
    }

    //Initial timing estimate
    if (args.router_initial_timing.provenance() != Provenance::SPECIFIED) {
        if (args.router_lookahead_type != e_router_lookahead::CLASSIC) {
            args.router_initial_timing.set(e_router_initial_timing::LOOKAHEAD, Provenance::INFERRED);
        } else {
            args.router_initial_timing.set(e_router_initial_timing::ALL_CRITICAL, Provenance::INFERRED);
        }
    }
}

bool verify_args(const t_options& args) {
    /*
     * Check for conflicting parameters or dependencies where one parameter set requires another parameter to be included
     */
    if (args.read_rr_graph_file.provenance() == Provenance::SPECIFIED
        && args.RouteChanWidth.provenance() != Provenance::SPECIFIED) {
        VPR_FATAL_ERROR(VPR_ERROR_OTHER,
                        "--route_chan_width option must be specified if --read_rr_graph is requested (%s)\n",
                        args.read_rr_graph_file.argument_name().c_str());
    }

    if (!args.enable_clustering_pin_feasibility_filter && (args.target_external_pin_util.provenance() == Provenance::SPECIFIED)) {
        VPR_FATAL_ERROR(VPR_ERROR_OTHER,
                        "%s option must be enabled for %s to have any effect\n",
                        args.enable_clustering_pin_feasibility_filter.argument_name().c_str(),
                        args.target_external_pin_util.argument_name().c_str());
    }

    if (args.router_initial_timing == e_router_initial_timing::LOOKAHEAD && args.router_lookahead_type == e_router_lookahead::CLASSIC) {
        VPR_FATAL_ERROR(VPR_ERROR_OTHER,
                        "%s option value 'lookahead' is not compatible with %s 'classic'\n",
                        args.router_initial_timing.argument_name().c_str(),
                        args.router_lookahead_type.argument_name().c_str());
    }

    /**
     * @brief If the user provided the "--noc" command line option, then there
     * must be a NoC in the FPGA and the netlist must include NoC routers.
     * Therefore, the user must also provide a noc traffic flows file to
     * describe the communication within the NoC. We ensure that a noc traffic
     * flows file is provided when the "--noc" option is used. If it is not
     * provided, we throw an error.
     * 
     */
    if (args.noc.provenance() == Provenance::SPECIFIED && args.noc_flows_file.provenance() != Provenance::SPECIFIED) {
        VPR_FATAL_ERROR(VPR_ERROR_OTHER,
                        "--noc_flows_file option must be specified if --noc is turned on.\n");
    }

    return true;
}
