#include "pathhelper.h"
#include "globals.h"
#include "vpr_net_pins_matrix.h"
#include "VprTimingGraphResolver.h"
#include "tatum/TimingReporter.hpp"

#include "draw_types.h"
#include "draw_global.h"

#include <sstream>
#include <cassert>

std::string getPathsStr(const std::vector<tatum::TimingPath>& paths, const std::string& detailsLevel, bool is_flat_routing) 
{
    // shortcuts
    auto& atom_ctx = g_vpr_ctx.atom();
    auto& timing_ctx = g_vpr_ctx.timing();
    // shortcuts
    
    // build deps
    NetPinsMatrix<float> net_delay = make_net_pins_matrix<float>(atom_ctx.nlist);
    auto analysis_delay_calc = std::make_shared<AnalysisDelayCalculator>(atom_ctx.nlist, atom_ctx.lookup, net_delay, is_flat_routing);
    
    VprTimingGraphResolver resolver(atom_ctx.nlist, atom_ctx.lookup, *timing_ctx.graph, *analysis_delay_calc.get(), is_flat_routing);
    e_timing_report_detail detailesLevelEnum = e_timing_report_detail::NETLIST;
    if (detailsLevel == "netlist") {
        detailesLevelEnum = e_timing_report_detail::NETLIST;
    } else if (detailsLevel == "aggregated") {
        detailesLevelEnum = e_timing_report_detail::AGGREGATED;
    } else if (detailsLevel == "detailed") {
        detailesLevelEnum = e_timing_report_detail::DETAILED_ROUTING;
    } else if (detailsLevel == "debug") {
        detailesLevelEnum = e_timing_report_detail::DEBUG;
    } else {
        std::cerr << "unhandled option" << detailsLevel << std::endl;
    }
    resolver.set_detail_level(detailesLevelEnum);
    //

    std::stringstream ss;
    tatum::TimingReporter timing_reporter(resolver, *timing_ctx.graph, *timing_ctx.constraints);
    timing_reporter.report_timing(ss, paths);

    return ss.str();
}

void calcCritPath(int numMax, const std::string& type)
{
    g_vpr_ctx.critical_path_num = numMax;
    g_vpr_ctx.path_type = type;

    tatum::TimingPathCollector path_collector;

    t_draw_state* draw_state = get_draw_state_vars();
    auto& timing_ctx = g_vpr_ctx.timing();

    //Get the worst timing path
    if (type == "setup") {
        g_vpr_ctx.crit_paths = path_collector.collect_worst_setup_timing_paths(
            *timing_ctx.graph,
            *(draw_state->setup_timing_info->setup_analyzer()), numMax);
    } else if (type == "hold") {
        /* ###########################
        // TODO: recalculate g_vpr_ctx.hold_timing_info, because it may cause the assert path check for index 0
        // ###########################

        // std::vector<TimingPath> TimingPathCollector::collect_worst_hold_timing_paths(const TimingGraph& timing_graph, const tatum::HoldTimingAnalyzer& hold_analyzer, size_t npaths) const {
        // auto& atom_ctx = g_vpr_ctx.atom();

        // NetPinsMatrix<float> net_delay = make_net_pins_matrix<float>(net_list);
        // load_net_delay_from_routing(net_list,
        //                             net_delay);

        // auto analysis_delay_calc = std::make_shared<AnalysisDelayCalculator>(atom_ctx.nlist, atom_ctx.lookup, net_delay, vpr_setup.RouterOpts.flat_routing);
        // auto timing_info = make_setup_hold_timing_info(analysis_delay_calc, vpr_setup.AnalysisOpts.timing_update_type);
        // timing_info->update();
        /* */

        if (g_vpr_ctx.hold_timing_info) {
            g_vpr_ctx.crit_paths = path_collector.collect_worst_hold_timing_paths(
                *timing_ctx.graph,
                *(g_vpr_ctx.hold_timing_info->hold_analyzer()), numMax);
        } else {
            std::cerr << "g_vpr_ctx.hold_timing_info is nullptr" << std::endl;
        }
    } 
}



// void draw_crit_path(ezgl::renderer* g) {
//     std::cout << "~~~ draw_crit_path start" << std::endl;
//     tatum::TimingPathCollector path_collector;

//     t_draw_state* draw_state = get_draw_state_vars();
//     auto& timing_ctx = g_vpr_ctx.timing();

//     if (draw_state->show_crit_path == DRAW_NO_CRIT_PATH) {
//         return;
//     }

//     if (!draw_state->setup_timing_info) {
//         return; //No timing to draw
//     }

//     //Get the worst timing path
//     auto paths = path_collector.collect_worst_setup_timing_paths(
//         *timing_ctx.graph,
//         *(draw_state->setup_timing_info->setup_analyzer()), 100);

//     std::string pathIdsStr = ""; //read_shared_string();
//     std::vector<std::string> selectedPathIds; // = splitString(pathIdsStr, ';');
//     auto& server = g_vpr_ctx.server();
//     std::stringstream ss;
//     dumpPathsToStr(ss, paths, false);
//     //server.init();
//     //server.setPathsStr(ss.str());
//     //server.tick();

//     //vpr_setup.AnalysisOpts, vpr_setup.RouterOpts.flat_routing

//     // name resolving
//     //bool is_flat = vpr_setup.RouterOpts.flat_routing;
//     // bool is_flat = true;
//     // const Netlist<>& net_list = is_flat ? (const Netlist<>&)g_vpr_ctx.atom().nlist : (const Netlist<>&)g_vpr_ctx.clustering().clb_nlist;

//     auto& atom_ctx = g_vpr_ctx.atom();
//     NetPinsMatrix<float> net_delay = make_net_pins_matrix<float>(atom_ctx.nlist);
//         // load_net_delay_from_routing(net_list,
//         //                             net_delay);

//     auto analysis_delay_calc = std::make_shared<AnalysisDelayCalculator>(atom_ctx.nlist, atom_ctx.lookup, net_delay, true/*vpr_setup.RouterOpts.flat_routing*/);
//     VprTimingGraphResolver name_resolver(atom_ctx.nlist, atom_ctx.lookup, *timing_ctx.graph, *analysis_delay_calc.get(), true/*is_flat*/);
//     //

//     //std::unordered_set<std::string> _set;
//     int index = 0;
//     for (const tatum::TimingPath& p: paths) {
//         tatum::TimingPathInfo pi = p.path_info();
//         //std::cout << "~~~~~~~~~~" << std::endl;
//         //std::cout << "pi.type=" << (int)pi.type() << std::endl;
//         //std::cout << "pi.delay=" << pi.delay() << std::endl;
//         //std::cout << "pi.slack=" << pi.slack() << std::endl;

//         //std::cout << "pi.startpoint=" << pi.startpoint() << std::endl;
//         //std::cout << "pi.endpoint=" << pi.endpoint() << std::endl;
//         //std::cout << "pi.launch_domain=" << pi.launch_domain() << std::endl;
//         //std::cout << "pi.capture_domain=" << pi.capture_domain() << std::endl;

//         // size_t t1 = static_cast<std::size_t>(pi.startpoint());
//         // size_t t2 = static_cast<std::size_t>(pi.endpoint());

//         // std::string st1 = std::to_string(t1);
//         // std::string st2 = std::to_string(t2);

//         std::string st1 = name_resolver.node_name(pi.startpoint());
//         std::string st2 = name_resolver.node_name(pi.endpoint());

//         std::string pathId = st1 + ":" + st2;
//         for (const std::string& selectedPathId: selectedPathIds) {
//             if (selectedPathId == pathId) {
//                 draw_state->crit_path_index = index;
//             }
//         }
//         //_set.insert(pathId);
//         index++;
//     }
//     // for (const std::string& key: _set) {
//     //     std::cout << "~~~ key = " << key << std::endl;
//     // }
//     //std::cout << "_set.size()=" << _set.size() << std::endl;

//     // if (!extStr.empty()) {
//     //     int indexCandidate = std::atoi(extStr.c_str());
//     //     if (indexCandidate <= paths.size()) {
//     //         draw_state->crit_path_index = indexCandidate; 
//     //     }
//     // }

//     tatum::TimingPath path = paths[draw_state->crit_path_index];
//     tatum::TimingPathInfo pi = path.path_info();
//     // size_t t1 = static_cast<std::size_t>(pi.startpoint());
//     // size_t t2 = static_cast<std::size_t>(pi.endpoint());
//     // std::string st1 = std::to_string(t1);
//     // std::string st2 = std::to_string(t2);
//     std::string st1 = name_resolver.node_name(pi.startpoint());
//     std::string st2 = name_resolver.node_name(pi.endpoint());
        
//     std::string pathId = st1 + ":" + st2;

//     std::cout << "~~~~~~~~~~~~~~~~ num of pathes=" << paths.size() << " index=" << draw_state->crit_path_index << " is drawn, id=" << pathId << std::endl;

//     //Walk through the timing path drawing each edge
//     tatum::NodeId prev_node;
//     float prev_arr_time = std::numeric_limits<float>::quiet_NaN();
//     int i = 0;
//     for (tatum::TimingPathElem elem : path.data_arrival_path().elements()) {
//         tatum::NodeId node = elem.node();
//         float arr_time = elem.tag().time();
//         if (prev_node) {
//             //We draw each 'edge' in a different color, this allows users to identify the stages and
//             //any routing which corresponds to the edge
//             //
//             //We pick colors from the kelly max-contrast list, for long paths there may be repeats
//             ezgl::color color = kelly_max_contrast_colors[i++
//                                                           % kelly_max_contrast_colors.size()];

//             float delay = arr_time - prev_arr_time;
//             if (draw_state->show_crit_path == DRAW_CRIT_PATH_FLYLINES
//                 || draw_state->show_crit_path
//                        == DRAW_CRIT_PATH_FLYLINES_DELAYS) {
//                 g->set_color(color);
//                 g->set_line_dash(ezgl::line_dash::none);
//                 g->set_line_width(4);
//                 draw_flyline_timing_edge(tnode_draw_coord(prev_node),
//                                          tnode_draw_coord(node), delay, g);
//             } else {
//                 VTR_ASSERT(draw_state->show_crit_path != DRAW_NO_CRIT_PATH);

//                 //Draw the routed version of the timing edge
//                 draw_routed_timing_edge(prev_node, node, delay, color, g);
//             }
//         }
//         prev_node = node;
//         prev_arr_time = arr_time;
//     }

//     std::cout << "~~~ draw_crit_path end" << std::endl;
// }
