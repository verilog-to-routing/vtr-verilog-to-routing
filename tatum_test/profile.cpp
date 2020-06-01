#include <iostream>
#include <cmath>
#include <chrono>
#include <random>
#include <queue>
#include "profile.hpp"
#include "verify.hpp"

#include "tatum/TimingGraph.hpp"
#include "tatum/analyzer_factory.hpp"
#include "tatum/base/sta_util.hpp"

#ifdef TATUM_TEST_PROFILE_VTUNE
#include "ittnotify.h"
#endif

#ifdef TATUM_TEST_PROFILE_CALLGRIND
#include <valgrind/callgrind.h>
#endif


typedef std::chrono::duration<double> dsec;
typedef std::chrono::high_resolution_clock Clock;

std::map<std::string,std::vector<double>> profile(size_t num_iterations, std::shared_ptr<tatum::TimingAnalyzer> serial_analyzer) {
    //To selectively profile using callgrind:
    //  valgrind --tool=callgrind --collect-atstart=no --instr-atstart=no --cache-sim=yes --cacheuse=yes ./command

    std::map<std::string,std::vector<double>> prof_data;

#ifdef TATUM_TEST_PROFILE_CALLGRIND
    CALLGRIND_START_INSTRUMENTATION;
#endif

    for(size_t i = 0; i < num_iterations; i++) {
        //Analyze

#ifdef TATUM_TEST_PROFILE_VTUNE
        __itt_resume();
#endif

#ifdef TATUM_TEST_PROFILE_CALLGRIND
        CALLGRIND_TOGGLE_COLLECT;
#endif

        serial_analyzer->update_timing();

#ifdef TATUM_TEST_PROFILE_CALLGRIND
        CALLGRIND_TOGGLE_COLLECT;
#endif

#ifdef TATUM_TEST_PROFILE_VTUNE
        __itt_pause();
#endif

        for(auto key : {"arrival_pre_traversal_sec", "arrival_traversal_sec", "required_pre_traversal_sec", "required_traversal_sec", "reset_sec", "update_slack_sec", "analysis_sec"}) {
            prof_data[key].push_back(serial_analyzer->get_profiling_data(key));
        }


        std::cout << ".";
        std::cout.flush();
    }

#ifdef TATUM_TEST_PROFILE_CALLGRIND
    CALLGRIND_STOP_INSTRUMENTATION;
#endif

    return prof_data;
}

bool profile_incr(size_t num_iterations,
                  float edge_change_prob,
                  bool verify,
                  const tatum::TimingGraph& tg,
                  std::shared_ptr<tatum::TimingAnalyzer> check_analyzer,
                  std::shared_ptr<tatum::TimingAnalyzer> ref_analyzer,
                  tatum::FixedDelayCalculator& delay_calc,
                  std::map<std::string,std::vector<double>>& prof_data) {

    std::minstd_rand rng;
    std::uniform_int_distribution<size_t> uniform_distr(0, tg.edges().size() - 1);
    std::normal_distribution<float> normal_distr(0, 1e-9);

    //Record clock-related edges
    std::set<tatum::EdgeId> clk_edges;
    std::queue<tatum::EdgeId> q;
    for (tatum::NodeId node : tg.nodes()) {
        if (tg.node_type(node) == tatum::NodeType::SOURCE
            || tg.node_type(node) == tatum::NodeType::SINK) {
            for (tatum::EdgeId edge : tg.node_in_edges(node)) {
                clk_edges.insert(edge);
            }
        }
    }

    struct timespec verify_start;
    struct timespec verify_end;

#ifdef TATUM_TEST_PROFILE_CALLGRIND
    std::cout << "Callgrind Start Instrumentation\n";
    CALLGRIND_START_INSTRUMENTATION;
#endif

    for(size_t i = 0; i < num_iterations; i++) {

        if (i > 0) {
            //Randomly invalidate edges

            size_t EDGES_TO_INVALIDATE = edge_change_prob * tg.edges().size();
            for (size_t j = 0; j < EDGES_TO_INVALIDATE; j++) {
                size_t iedge = uniform_distr(rng);
                tatum::EdgeId edge(iedge);

                //Invalidate
                check_analyzer->invalidate_edge(edge);
                ref_analyzer->invalidate_edge(edge);

                //Set new delays
                //std::cout << "New Delay: " << edge;
                if (tg.edge_type(edge) == tatum::EdgeType::PRIMITIVE_CLOCK_CAPTURE) {
                    float new_setup = std::max<float>(0, delay_calc.setup_time(tg, edge).value() + normal_distr(rng));
                    float new_hold = std::max<float>(0, delay_calc.hold_time(tg, edge).value() + normal_distr(rng));
                    //std::cout << " setup: " << new_setup << " hold: " << new_hold;
                    delay_calc.set_setup_time(tg, edge, tatum::Time(new_setup));
                    delay_calc.set_hold_time(tg, edge, tatum::Time(new_hold));
                } else {
                    float new_max = std::max<float>(0, delay_calc.max_edge_delay(tg, edge).value() + normal_distr(rng));
                    float new_min = std::max<float>(0, delay_calc.min_edge_delay(tg, edge).value() + normal_distr(rng));
                    //std::cout << " min: " << new_min << " max: " << new_max << " (was: " << delay_calc.min_edge_delay(tg, edge).value() << ", " << delay_calc.max_edge_delay(tg, edge).value() << ")";
                    delay_calc.set_max_edge_delay(tg, edge, tatum::Time(new_max));
                    delay_calc.set_min_edge_delay(tg, edge, tatum::Time(new_min));
                }
                //std::cout << "\n";
            }
        }

        //Analyze

#ifdef TATUM_TEST_PROFILE_CALLGRIND
        std::cout << "Toggle Collect (Start)\n";
        CALLGRIND_TOGGLE_COLLECT;
#endif
        check_analyzer->update_timing();

#ifdef TATUM_TEST_PROFILE_CALLGRIND
        CALLGRIND_TOGGLE_COLLECT;
        std::cout << "Toggle Collect (End)\n";
        std::stringstream ss;
        ss << "incr_update" << i;
        std::cout << "Callgrind Dump Stats " << ss.str() << "\n";
        CALLGRIND_DUMP_STATS_AT(ss.str().c_str());
#endif
        ref_analyzer->update_timing();

        //Verify
        clock_gettime(CLOCK_MONOTONIC, &verify_start);

        if (verify) {
            auto res = verify_equivalent_analysis(tg, delay_calc, ref_analyzer, check_analyzer);

            if (res.second) {
                std::cout << "Equivalent\n";
            } else {
                std::cout << "Not equivalent\n";
                return false;
            }

        }
        clock_gettime(CLOCK_MONOTONIC, &verify_end);

        std::cout << "Arr: incr=" << check_analyzer->get_profiling_data("arrival_traversal_sec") << " ref=" << ref_analyzer->get_profiling_data("arrival_traversal_sec") << "\n";
        std::cout << "Req: incr=" << check_analyzer->get_profiling_data("required_traversal_sec") << " ref=" << ref_analyzer->get_profiling_data("required_traversal_sec") << "\n";
        for(auto key : {"arrival_pre_traversal_sec", "arrival_traversal_sec", "required_pre_traversal_sec", "required_traversal_sec", "reset_sec", "update_slack_sec", "analysis_sec"}) {
            prof_data[key].push_back(check_analyzer->get_profiling_data(key));
            prof_data[std::string("ref_") + key].push_back(ref_analyzer->get_profiling_data(key));
        }
        prof_data["verify_sec"].push_back(tatum::time_sec(verify_start, verify_end));

        std::cout << ".";
        std::cout.flush();
    }

#ifdef TATUM_TEST_PROFILE_CALLGRIND
    std::cout << "Callgrind Stop Instrumentation\n";
    CALLGRIND_STOP_INSTRUMENTATION;
#endif

    return true;
}
