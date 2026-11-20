/**
 * @file
 * @brief Unit tests for early/late clock paths (min/max delays as on-chip variation) and common clock
 *        path pessimism removal (CCPP), in both the analyzer and the timing reports.
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

using tatum::DomainId;
using tatum::EdgeId;
using tatum::EdgeType;
using tatum::NodeId;
using tatum::NodeType;
using tatum::Time;
using tatum::TimingPath;

/**
 * @brief A small, fully analyzed, single clock circuit with min != max delays on its clock paths.
 *
 *      clk_src -> cpin1 (ff1) : max 1.0 ns, min 0.8 ns
 *      clk_src -> cpin2 (ff2) : max 1.0 ns, min 0.8 ns
 *      ff1_q   -> ff2_d       : 0.5 ns (min == max)
 *
 * Tcq is 0.1 ns, setup and hold times are 0.05 ns, and the clock period is 2 ns. Half of each clock
 * path's variation (0.1 ns) is shared by both clock paths (e.g. a clock input pad's delay). The
 * circuit is analyzed with that 0.1 ns credit unless ccpp_credit is given.
 */
class CcppTestCircuit {
  public:
    /// @brief Builds, constrains and analyzes the circuit with the given CCPP credit (in seconds).
    explicit CcppTestCircuit(double ccpp_credit = 0.1e-9);

    /// @brief Returns the worst setup path ending at ff2_d.
    TimingPath setup_path() const;

    /// @brief Returns the worst hold path ending at ff2_d.
    TimingPath hold_path() const;

  public:
    tatum::TimingGraph tg;                                    ///< The circuit's timing graph.
    tatum::TimingConstraints tc;                              ///< The circuit's timing constraints.
    std::unique_ptr<tatum::FixedDelayCalculator> delay_calc;  ///< The delays of every edge in tg.
    std::unique_ptr<tatum::SetupHoldTimingAnalyzer> analyzer; ///< The (updated) analyzer of the circuit.

    NodeId ff2_d; ///< Data input (SINK) of flip-flop 2.

  private:
    /// @brief Adds an edge with the given max and min delays (or setup and hold times, for capture edges)
    ///        and returns its sink node.
    NodeId add_edge_(EdgeType type, NodeId src, NodeId sink, double max_delay, double min_delay);

    /// @brief The max and min delays of each edge added to tg, indexed by edge id.
    std::vector<double> max_delays_;
    std::vector<double> min_delays_;
};

CcppTestCircuit::CcppTestCircuit(double ccpp_credit) {
    NodeId clk_src = tg.add_node(NodeType::SOURCE);
    NodeId cpin1 = add_edge_(EdgeType::INTERCONNECT, clk_src, tg.add_node(NodeType::CPIN), 1.0e-9, 0.8e-9);
    NodeId cpin2 = add_edge_(EdgeType::INTERCONNECT, clk_src, tg.add_node(NodeType::CPIN), 1.0e-9, 0.8e-9);

    NodeId ff1_q = add_edge_(EdgeType::PRIMITIVE_CLOCK_LAUNCH, cpin1, tg.add_node(NodeType::SOURCE), 0.1e-9, 0.1e-9);
    add_edge_(EdgeType::PRIMITIVE_CLOCK_CAPTURE, cpin1, tg.add_node(NodeType::SINK), 0.05e-9, 0.05e-9);
    ff2_d = add_edge_(EdgeType::PRIMITIVE_CLOCK_CAPTURE, cpin2, tg.add_node(NodeType::SINK), 0.05e-9, 0.05e-9);

    NodeId ipin = add_edge_(EdgeType::INTERCONNECT, ff1_q, tg.add_node(NodeType::IPIN), 0.5e-9, 0.5e-9);
    add_edge_(EdgeType::PRIMITIVE_COMBINATIONAL, ipin, ff2_d, 0., 0.);

    tg.levelize();

    DomainId clk = tc.create_clock_domain("clk");
    tc.set_clock_domain_source(clk_src, clk);
    tc.set_setup_constraint(clk, clk, Time(2.0e-9));
    tc.set_hold_constraint(clk, clk, Time(0.));
    tc.set_common_clock_path_pessimism(clk, Time(ccpp_credit));

    // The delay calculator takes the setup/hold times of capture edges separately from the other edge delays.
    size_t num_edges = max_delays_.size();
    tatum::util::linear_map<EdgeId, Time> max_edge_delays(num_edges);
    tatum::util::linear_map<EdgeId, Time> min_edge_delays(num_edges);
    tatum::util::linear_map<EdgeId, Time> setup_times(num_edges);
    tatum::util::linear_map<EdgeId, Time> hold_times(num_edges);
    for (size_t i = 0; i < num_edges; ++i) {
        EdgeId edge(i);
        if (tg.edge_type(edge) == EdgeType::PRIMITIVE_CLOCK_CAPTURE) {
            setup_times[edge] = Time(max_delays_[i]);
            hold_times[edge] = Time(min_delays_[i]);
        } else {
            max_edge_delays[edge] = Time(max_delays_[i]);
            min_edge_delays[edge] = Time(min_delays_[i]);
        }
    }
    delay_calc = std::make_unique<tatum::FixedDelayCalculator>(max_edge_delays, setup_times, min_edge_delays, hold_times);

    analyzer = tatum::AnalyzerFactory<tatum::SetupHoldAnalysis>::make(tg, tc, *delay_calc);
    analyzer->update_timing();
}

TimingPath CcppTestCircuit::setup_path() const {
    for (const TimingPath& path : tatum::TimingPathCollector().collect_worst_setup_timing_paths(tg, *analyzer, 100)) {
        if (path.path_info().endpoint() == ff2_d) return path;
    }
    FAIL("No setup path ends at ff2_d");
    return TimingPath();
}

TimingPath CcppTestCircuit::hold_path() const {
    for (const TimingPath& path : tatum::TimingPathCollector().collect_worst_hold_timing_paths(tg, *analyzer, 100)) {
        if (path.path_info().endpoint() == ff2_d) return path;
    }
    FAIL("No hold path ends at ff2_d");
    return TimingPath();
}

NodeId CcppTestCircuit::add_edge_(EdgeType type, NodeId src, NodeId sink, double max_delay, double min_delay) {
    EdgeId edge = tg.add_edge(type, src, sink);
    REQUIRE(size_t(edge) == max_delays_.size());
    max_delays_.push_back(max_delay);
    min_delays_.push_back(min_delay);
    return sink;
}

/// @brief Requires the path's slack to be the expected value (in seconds).
void require_slack(const TimingPath& path, double expected) {
    REQUIRE_THAT(path.slack_tag().time().value(), Catch::Matchers::WithinAbs(expected, 1e-15));
}

TEST_CASE("ccpp_setup_slack", "[tatum]") {
    // Launched late: 1.0 + 0.1 + 0.5 = 1.6 ns.
    // Captured early: 2.0 + 0.8 - 0.05 = 2.75 ns, plus the credit.
    SECTION("with credit") {
        CcppTestCircuit circuit;
        require_slack(circuit.setup_path(), 1.25e-9); // 2.75 + 0.1 - 1.6
    }
    SECTION("without credit (fully pessimistic)") {
        CcppTestCircuit circuit(0.);
        require_slack(circuit.setup_path(), 1.15e-9); // 2.75 - 1.6
    }
}

TEST_CASE("ccpp_hold_slack", "[tatum]") {
    // Launched early: 0.8 + 0.1 + 0.5 = 1.4 ns.
    // Captured late: 0.0 + 1.0 + 0.05 = 1.05 ns, minus the credit.
    SECTION("with credit") {
        CcppTestCircuit circuit;
        require_slack(circuit.hold_path(), 0.45e-9); // 1.4 - (1.05 - 0.1)
    }
    SECTION("without credit (fully pessimistic)") {
        CcppTestCircuit circuit(0.);
        require_slack(circuit.hold_path(), 0.35e-9); // 1.4 - 1.05
    }
}

TEST_CASE("ccpp_credit_needs_shared_clock_source", "[tatum]") {
    tatum::TimingConstraints tc;
    NodeId src_a(0);
    NodeId src_b(1);

    DomainId clk_a = tc.create_clock_domain("clk_a");
    DomainId clk_a2 = tc.create_clock_domain("clk_a2"); // A second clock defined on the same source
    DomainId clk_b = tc.create_clock_domain("clk_b");
    DomainId virt = tc.create_clock_domain("virt");     // Virtual clock: no source
    tc.set_clock_domain_source(src_a, clk_a);
    tc.set_clock_domain_source(src_a, clk_a2);
    tc.set_clock_domain_source(src_b, clk_b);

    tc.set_common_clock_path_pessimism(clk_a, Time(0.2e-9));
    tc.set_common_clock_path_pessimism(clk_a2, Time(0.1e-9));
    tc.set_common_clock_path_pessimism(clk_b, Time(0.3e-9));

    REQUIRE(tc.clock_domain_common_clock_path_pessimism(clk_a).value() == Time(0.2e-9).value());
    REQUIRE(tc.clock_domain_common_clock_path_pessimism(virt).value() == 0.); // Never set

    REQUIRE(tc.common_clock_path_pessimism(clk_a, clk_a).value() == Time(0.2e-9).value());
    REQUIRE(tc.common_clock_path_pessimism(clk_a, clk_a2).value() == Time(0.1e-9).value()); // Smaller of the two
    REQUIRE(tc.common_clock_path_pessimism(clk_a, clk_b).value() == 0.);
    REQUIRE(tc.common_clock_path_pessimism(clk_b, virt).value() == 0.);
    REQUIRE(tc.common_clock_path_pessimism(virt, virt).value() == 0.);
}

TEST_CASE("ccpp_report", "[tatum]") {
    CcppTestCircuit circuit;
    tatum::NodeNumResolver resolver(circuit.tg, *circuit.delay_calc, /*verbose=*/false);
    tatum::TimingReporter reporter(resolver, circuit.tg, circuit.tc);

    // The reports check their own arrival, required and skew values against the analyzer's, so they throw
    // if the reported capture clock path or credit doesn't match what the analyzer used.
    std::stringstream report;
    REQUIRE_NOTHROW(reporter.report_timing_setup(report, *circuit.analyzer));
    REQUIRE_NOTHROW(reporter.report_timing_hold(report, *circuit.analyzer));
    REQUIRE_NOTHROW(reporter.report_skew_setup(report, *circuit.analyzer));
    REQUIRE_NOTHROW(reporter.report_skew_hold(report, *circuit.analyzer));

    REQUIRE_THAT(report.str(), Catch::Matchers::ContainsSubstring("common clock path pessimism"));
}

} // namespace
