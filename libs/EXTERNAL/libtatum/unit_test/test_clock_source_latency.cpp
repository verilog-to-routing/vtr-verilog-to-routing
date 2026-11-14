/**
 * @file
 * @brief Unit tests for clock source latency with different early and late values
 *        (i.e. set_clock_latency -source -early/-late), in both the analyzer and the timing reports.
 */

#include <memory>
#include <sstream>
#include <vector>

#include "catch2/catch_test_macros.hpp"
#include "catch2/matchers/catch_matchers_floating_point.hpp"
#include "catch2/matchers/catch_matchers_string.hpp"

#include "tatum/TimingGraph.hpp"
#include "tatum/TimingConstraints.hpp"
#include "tatum/TimingReporter.hpp"
#include "tatum/analyzer_factory.hpp"
#include "tatum/delay_calc/FixedDelayCalculator.hpp"
#include "tatum/report/NodeNumNameResolver.hpp"
#include "tatum/report/TimingPathCollector.hpp"

namespace {

using tatum::ArrivalType;
using tatum::DelayType;
using tatum::DomainId;
using tatum::EdgeId;
using tatum::EdgeType;
using tatum::NodeId;
using tatum::NodeType;
using tatum::Time;
using tatum::TimingPath;

constexpr double EARLY_LATENCY = 0.3e-9;
constexpr double LATE_LATENCY = 0.7e-9;

/**
 * @brief A small, fully analyzed, single clock circuit whose clock has different early and late source latencies.
 *
 *      in_a  -> ff1_d   (in2reg)
 *      ff1_q -> ff2_d   (reg2reg)
 *      ff1_q -> out_x   (reg2out)
 *
 * Launch clocks use the late latency for setup and the early latency for hold, while capture clocks
 * use the opposite. Primary outputs have no clock capture path, so their required time depends only
 * on the source latency, and not on any clock tag propagated by the analyzer.
 */
class LatencyTestCircuit {
  public:
    /// @brief Builds, constrains and analyzes the circuit.
    LatencyTestCircuit();

    /// @brief Returns the worst setup path ending at the given node.
    TimingPath setup_path_to(NodeId endpoint) const;

    /// @brief Returns the worst hold path ending at the given node.
    TimingPath hold_path_to(NodeId endpoint) const;

  public:
    tatum::TimingGraph tg;                                    ///< The circuit's timing graph.
    tatum::TimingConstraints tc;                              ///< The circuit's timing constraints.
    std::unique_ptr<tatum::FixedDelayCalculator> delay_calc;  ///< The delays of every edge in tg.
    std::unique_ptr<tatum::SetupHoldTimingAnalyzer> analyzer; ///< The (updated) analyzer of the circuit.

    NodeId ff1_q; ///< Data output (SOURCE) of flip-flop 1.
    NodeId ff1_d; ///< Data input (SINK) of flip-flop 1.
    NodeId ff2_d; ///< Data input (SINK) of flip-flop 2.
    NodeId in_a;  ///< Primary input driving ff1_d.
    NodeId out_x; ///< Primary output driven by ff1_q.

  private:
    /// @brief Adds an edge with the given delay (or setup/hold time, for capture edges) and returns its sink node.
    NodeId add_edge_(EdgeType type, NodeId src, NodeId sink, double delay);

    /// @brief Connects src to sink through an intermediate IPIN with the given delay.
    void add_data_path_(NodeId src, NodeId sink, double delay);

    /// @brief Returns the path ending at the given node.
    static TimingPath path_to_(const std::vector<TimingPath>& paths, NodeId endpoint);

    /// @brief The delay of each edge added to tg, indexed by edge id.
    std::vector<double> edge_delays_;
};

LatencyTestCircuit::LatencyTestCircuit() {
    NodeId clk_src = tg.add_node(NodeType::SOURCE);
    NodeId cpin1 = add_edge_(EdgeType::INTERCONNECT, clk_src, tg.add_node(NodeType::CPIN), 0.1e-9);
    NodeId cpin2 = add_edge_(EdgeType::INTERCONNECT, clk_src, tg.add_node(NodeType::CPIN), 0.3e-9);

    // Clock-to-q delay of 0.05 ns, and setup/hold times of 0.02 ns
    ff1_q = add_edge_(EdgeType::PRIMITIVE_CLOCK_LAUNCH, cpin1, tg.add_node(NodeType::SOURCE), 0.05e-9);
    ff1_d = add_edge_(EdgeType::PRIMITIVE_CLOCK_CAPTURE, cpin1, tg.add_node(NodeType::SINK), 0.02e-9);
    ff2_d = add_edge_(EdgeType::PRIMITIVE_CLOCK_CAPTURE, cpin2, tg.add_node(NodeType::SINK), 0.02e-9);

    in_a = tg.add_node(NodeType::SOURCE);
    out_x = tg.add_node(NodeType::SINK);

    add_data_path_(in_a, ff1_d, 0.2e-9);
    add_data_path_(ff1_q, ff2_d, 0.2e-9);
    add_data_path_(ff1_q, out_x, 0.2e-9);

    tg.levelize();

    DomainId clk = tc.create_clock_domain("clk");
    tc.set_clock_domain_source(clk_src, clk);
    tc.set_setup_constraint(clk, clk, Time(1.0e-9));
    tc.set_hold_constraint(clk, clk, Time(0.));
    tc.set_source_latency(clk, ArrivalType::EARLY, Time(EARLY_LATENCY));
    tc.set_source_latency(clk, ArrivalType::LATE, Time(LATE_LATENCY));
    tc.set_input_constraint(in_a, clk, DelayType::MAX, Time(0.));
    tc.set_input_constraint(in_a, clk, DelayType::MIN, Time(0.));
    tc.set_output_constraint(out_x, clk, DelayType::MAX, Time(0.1e-9));
    tc.set_output_constraint(out_x, clk, DelayType::MIN, Time(0.));

    // The delay calculator takes the setup times of capture edges separately from the other edge delays.
    // The min delays and hold times are the same as the max delays and setup times.
    tatum::util::linear_map<EdgeId, Time> max_edge_delays(edge_delays_.size());
    tatum::util::linear_map<EdgeId, Time> setup_times(edge_delays_.size());
    for (size_t i = 0; i < edge_delays_.size(); ++i) {
        EdgeId edge(i);
        if (tg.edge_type(edge) == EdgeType::PRIMITIVE_CLOCK_CAPTURE) {
            setup_times[edge] = Time(edge_delays_[i]);
        } else {
            max_edge_delays[edge] = Time(edge_delays_[i]);
        }
    }
    delay_calc = std::make_unique<tatum::FixedDelayCalculator>(max_edge_delays, setup_times);

    analyzer = tatum::AnalyzerFactory<tatum::SetupHoldAnalysis>::make(tg, tc, *delay_calc);
    analyzer->update_timing();
}

TimingPath LatencyTestCircuit::setup_path_to(NodeId endpoint) const {
    return path_to_(tatum::TimingPathCollector().collect_worst_setup_timing_paths(tg, *analyzer, 100), endpoint);
}

TimingPath LatencyTestCircuit::hold_path_to(NodeId endpoint) const {
    return path_to_(tatum::TimingPathCollector().collect_worst_hold_timing_paths(tg, *analyzer, 100), endpoint);
}

NodeId LatencyTestCircuit::add_edge_(EdgeType type, NodeId src, NodeId sink, double delay) {
    EdgeId edge = tg.add_edge(type, src, sink);
    REQUIRE(size_t(edge) == edge_delays_.size());
    edge_delays_.push_back(delay);
    return sink;
}

void LatencyTestCircuit::add_data_path_(NodeId src, NodeId sink, double delay) {
    NodeId ipin = add_edge_(EdgeType::INTERCONNECT, src, tg.add_node(NodeType::IPIN), delay);
    add_edge_(EdgeType::PRIMITIVE_COMBINATIONAL, ipin, sink, 0.);
}

TimingPath LatencyTestCircuit::path_to_(const std::vector<TimingPath>& paths, NodeId endpoint) {
    for (const TimingPath& path : paths) {
        if (path.path_info().endpoint() == endpoint) {
            return path;
        }
    }
    FAIL("No timing path ends at node " << size_t(endpoint));
    return TimingPath();
}

/// @brief Requires the path's slack to be the expected value (in seconds).
void require_slack(const TimingPath& path, double expected) {
    REQUIRE_THAT(path.slack_tag().time().value(), Catch::Matchers::WithinAbs(expected, 1e-15));
}

TEST_CASE("clock_source_latency_setup_slack", "[tatum]") {
    LatencyTestCircuit circuit;

    // in2reg: launched by the late clock (0.7 + 0.2 = 0.9), captured by the early one (1.0 + 0.3 + 0.1 - 0.02 = 1.38)
    require_slack(circuit.setup_path_to(circuit.ff1_d), 0.48e-9);

    // reg2reg: launched late (0.7 + 0.1 + 0.05 + 0.2 = 1.05), captured early (1.0 + 0.3 + 0.3 - 0.02 = 1.58)
    require_slack(circuit.setup_path_to(circuit.ff2_d), 0.53e-9);

    // reg2out: launched late (1.05), captured early (1.0 + 0.3 - 0.1 = 1.2)
    require_slack(circuit.setup_path_to(circuit.out_x), 0.15e-9);
}

TEST_CASE("clock_source_latency_hold_slack", "[tatum]") {
    LatencyTestCircuit circuit;

    // in2reg: launched by the early clock (0.3 + 0.2 = 0.5), captured by the late one (0.7 + 0.1 + 0.02 = 0.82)
    require_slack(circuit.hold_path_to(circuit.ff1_d), -0.32e-9);

    // reg2reg: launched early (0.3 + 0.1 + 0.05 + 0.2 = 0.65), captured late (0.7 + 0.3 + 0.02 = 1.02)
    require_slack(circuit.hold_path_to(circuit.ff2_d), -0.37e-9);

    // reg2out: launched early (0.65), captured late (0.7 - 0 = 0.7)
    require_slack(circuit.hold_path_to(circuit.out_x), -0.05e-9);
}

TEST_CASE("clock_source_latency_report", "[tatum]") {
    LatencyTestCircuit circuit;
    tatum::NodeNumResolver resolver(circuit.tg, *circuit.delay_calc, /*verbose=*/false);
    tatum::TimingReporter reporter(resolver, circuit.tg, circuit.tc);

    // The reports check their own arrival, required and skew values against the analyzer's. These used
    // to disagree for primary outputs, since the reports applied the launch latency to the capture clock.
    std::stringstream report;
    REQUIRE_NOTHROW(reporter.report_timing_setup(report, *circuit.analyzer));
    REQUIRE_NOTHROW(reporter.report_timing_hold(report, *circuit.analyzer));
    REQUIRE_NOTHROW(reporter.report_skew_setup(report, *circuit.analyzer));
    REQUIRE_NOTHROW(reporter.report_skew_hold(report, *circuit.analyzer));
}

} // namespace
