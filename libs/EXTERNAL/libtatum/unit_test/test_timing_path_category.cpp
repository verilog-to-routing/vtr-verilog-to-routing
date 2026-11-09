/**
 * @file
 * @brief Unit tests for Tatum's timing path categories (in2reg, reg2reg, reg2out, in2out),
 *        and the filtering of collected and reported timing paths by category.
 */

#include <memory>
#include <sstream>
#include <vector>

#include "catch2/catch_test_macros.hpp"
#include "catch2/matchers/catch_matchers_string.hpp"

#include "tatum/TimingGraph.hpp"
#include "tatum/TimingConstraints.hpp"
#include "tatum/TimingReporter.hpp"
#include "tatum/analyzer_factory.hpp"
#include "tatum/delay_calc/FixedDelayCalculator.hpp"
#include "tatum/report/NodeNumNameResolver.hpp"
#include "tatum/report/TimingPathCategory.hpp"
#include "tatum/report/TimingPathCollector.hpp"

namespace {

using tatum::DelayType;
using tatum::DomainId;
using tatum::EdgeId;
using tatum::EdgeType;
using tatum::NodeId;
using tatum::NodeType;
using tatum::SkewPath;
using tatum::Time;
using tatum::TimingPath;
using tatum::TimingPathCategory;
using tatum::TimingPathCategoryFilter;

/**
 * @brief A small, fully analyzed, single clock circuit with paths of each category.
 *
 * All inputs, outputs and registers are constrained on the same clock:
 *
 *      in_a  -> ff1_d              (in2reg)
 *      ff1_q -> ff2_d              (reg2reg)
 *      ff2_q -> out_x              (reg2out)
 *      in_b  -> out_y              (in2out)
 *      in_c  -> ff3_d <- ff1_q     (in2reg for setup, reg2reg for hold)
 *
 * The last case has two paths into the same endpoint. Since the analyzer only records the
 * worst arrival per clock domain pair, the endpoint's category is that of its worst path:
 * the slow path from in_c for setup, and the fast path from ff1_q for hold.
 */
class CategoryTestCircuit {
  public:
    /// @brief Builds, constrains and analyzes the circuit.
    CategoryTestCircuit();

    /// @brief Returns the worst setup paths in the categories accepted by the filter.
    std::vector<TimingPath> setup_paths(const TimingPathCategoryFilter& filter, size_t npaths = 100) const;

    /// @brief Returns the worst hold paths in the categories accepted by the filter.
    std::vector<TimingPath> hold_paths(const TimingPathCategoryFilter& filter, size_t npaths = 100) const;

    /// @brief Returns the worst setup skew paths in the categories accepted by the filter.
    std::vector<SkewPath> setup_skew_paths(const TimingPathCategoryFilter& filter) const;

  public:
    tatum::TimingGraph tg;                                    ///< The circuit's timing graph.
    tatum::TimingConstraints tc;                              ///< The circuit's timing constraints.
    std::unique_ptr<tatum::FixedDelayCalculator> delay_calc;  ///< The delays of every edge in tg.
    std::unique_ptr<tatum::SetupHoldTimingAnalyzer> analyzer; ///< The (updated) analyzer of the circuit.

    NodeId ff1_q; ///< Data output (SOURCE) of flip-flop 1.
    NodeId ff2_q; ///< Data output (SOURCE) of flip-flop 2.
    NodeId ff1_d; ///< Data input (SINK) of flip-flop 1.
    NodeId ff2_d; ///< Data input (SINK) of flip-flop 2.
    NodeId ff3_d; ///< Data input (SINK) of flip-flop 3, reached from both in_c and ff1_q.
    NodeId in_a;  ///< Primary input driving ff1_d.
    NodeId in_b;  ///< Primary input driving out_y.
    NodeId in_c;  ///< Primary input driving ff3_d.
    NodeId out_x; ///< Primary output driven by ff2_q.
    NodeId out_y; ///< Primary output driven by in_b.

  private:
    /// @brief Adds an edge with the given delay (or setup time, for capture edges) and returns its sink node.
    NodeId add_edge_(EdgeType type, NodeId src, NodeId sink, double delay);

    /// @brief Adds a flip-flop data output (SOURCE) clocked by the given clock pin.
    NodeId add_ff_source_(NodeId cpin);

    /// @brief Adds a flip-flop data input (SINK) clocked by the given clock pin.
    NodeId add_ff_sink_(NodeId cpin);

    /// @brief Connects src to sink through an intermediate IPIN with the given delay.
    void add_data_path_(NodeId src, NodeId sink, double delay);

    /// @brief The delay of each edge added to tg, indexed by edge id.
    std::vector<double> edge_delays_;
};

CategoryTestCircuit::CategoryTestCircuit() {
    NodeId clk_src = tg.add_node(NodeType::SOURCE);

    // Give each flip-flop a different clock latency, so the reg2reg path has non-zero skew
    NodeId cpin1 = add_edge_(EdgeType::INTERCONNECT, clk_src, tg.add_node(NodeType::CPIN), 0.1e-9);
    NodeId cpin2 = add_edge_(EdgeType::INTERCONNECT, clk_src, tg.add_node(NodeType::CPIN), 0.3e-9);
    NodeId cpin3 = add_edge_(EdgeType::INTERCONNECT, clk_src, tg.add_node(NodeType::CPIN), 0.2e-9);

    ff1_q = add_ff_source_(cpin1);
    ff2_q = add_ff_source_(cpin2);
    ff1_d = add_ff_sink_(cpin1);
    ff2_d = add_ff_sink_(cpin2);
    ff3_d = add_ff_sink_(cpin3);

    in_a = tg.add_node(NodeType::SOURCE);
    in_b = tg.add_node(NodeType::SOURCE);
    in_c = tg.add_node(NodeType::SOURCE);
    out_x = tg.add_node(NodeType::SINK);
    out_y = tg.add_node(NodeType::SINK);

    add_data_path_(in_a, ff1_d, 0.2e-9);
    add_data_path_(ff1_q, ff2_d, 0.2e-9);
    add_data_path_(ff2_q, out_x, 0.2e-9);
    add_data_path_(in_b, out_y, 0.2e-9);

    // The path from in_c is the slowest into ff3_d, while the path from ff1_q is the fastest
    NodeId ipin_e = tg.add_node(NodeType::IPIN);
    add_edge_(EdgeType::INTERCONNECT, in_c, ipin_e, 0.6e-9);
    add_edge_(EdgeType::INTERCONNECT, ff1_q, ipin_e, 0.1e-9);
    add_edge_(EdgeType::PRIMITIVE_COMBINATIONAL, ipin_e, ff3_d, 0.);

    tg.levelize();

    DomainId clk = tc.create_clock_domain("clk");
    tc.set_clock_domain_source(clk_src, clk);
    tc.set_setup_constraint(clk, clk, Time(1.0e-9));
    tc.set_hold_constraint(clk, clk, Time(0.));
    for (NodeId input : {in_a, in_b, in_c}) {
        tc.set_input_constraint(input, clk, DelayType::MAX, Time(0.));
        tc.set_input_constraint(input, clk, DelayType::MIN, Time(0.));
    }
    for (NodeId output : {out_x, out_y}) {
        tc.set_output_constraint(output, clk, DelayType::MAX, Time(0.));
        tc.set_output_constraint(output, clk, DelayType::MIN, Time(0.));
    }

    // The delay calculator takes the setup times of capture edges separately from the other edge delays
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

std::vector<TimingPath> CategoryTestCircuit::setup_paths(const TimingPathCategoryFilter& filter, size_t npaths) const {
    return tatum::TimingPathCollector().collect_worst_setup_timing_paths(tg, *analyzer, npaths, filter);
}

std::vector<TimingPath> CategoryTestCircuit::hold_paths(const TimingPathCategoryFilter& filter, size_t npaths) const {
    return tatum::TimingPathCollector().collect_worst_hold_timing_paths(tg, *analyzer, npaths, filter);
}

std::vector<SkewPath> CategoryTestCircuit::setup_skew_paths(const TimingPathCategoryFilter& filter) const {
    return tatum::TimingPathCollector().collect_worst_setup_skew_paths(tg, tc, *analyzer, 100, filter);
}

NodeId CategoryTestCircuit::add_edge_(EdgeType type, NodeId src, NodeId sink, double delay) {
    EdgeId edge = tg.add_edge(type, src, sink);
    REQUIRE(size_t(edge) == edge_delays_.size());
    edge_delays_.push_back(delay);
    return sink;
}

NodeId CategoryTestCircuit::add_ff_source_(NodeId cpin) {
    // The launch edge delay is the clock-to-q delay
    return add_edge_(EdgeType::PRIMITIVE_CLOCK_LAUNCH, cpin, tg.add_node(NodeType::SOURCE), 0.05e-9);
}

NodeId CategoryTestCircuit::add_ff_sink_(NodeId cpin) {
    // The capture edge delay is the setup time
    return add_edge_(EdgeType::PRIMITIVE_CLOCK_CAPTURE, cpin, tg.add_node(NodeType::SINK), 0.02e-9);
}

void CategoryTestCircuit::add_data_path_(NodeId src, NodeId sink, double delay) {
    NodeId ipin = add_edge_(EdgeType::INTERCONNECT, src, tg.add_node(NodeType::IPIN), delay);
    add_edge_(EdgeType::PRIMITIVE_COMBINATIONAL, ipin, sink, 0.);
}

TEST_CASE("timing_path_category_filter", "[tatum]") {
    TimingPathCategoryFilter all;
    REQUIRE(all.accepts_all());
    REQUIRE(all.to_string() == "in2reg, reg2reg, reg2out, in2out");

    TimingPathCategoryFilter none = TimingPathCategoryFilter::none();
    REQUIRE(!none.accepts_endpoint(/*end_is_register=*/true));
    REQUIRE(!none.accepts_endpoint(/*end_is_register=*/false));
    REQUIRE(none.to_string().empty());

    TimingPathCategoryFilter reg2reg({TimingPathCategory::REG2REG});
    REQUIRE(!reg2reg.accepts_all());
    REQUIRE(reg2reg.contains(TimingPathCategory::REG2REG));
    REQUIRE(!reg2reg.contains(TimingPathCategory::IN2REG));
    REQUIRE(reg2reg.accepts_endpoint(/*end_is_register=*/true));
    REQUIRE(!reg2reg.accepts_endpoint(/*end_is_register=*/false));

    reg2reg.add(TimingPathCategory::REG2OUT).remove(TimingPathCategory::REG2REG);
    REQUIRE(reg2reg.to_string() == "reg2out");
}

TEST_CASE("timing_path_category_classify", "[tatum]") {
    CategoryTestCircuit circuit;
    const tatum::TimingGraph& tg = circuit.tg;

    REQUIRE(tatum::classify_timing_path(tg, circuit.in_a, circuit.ff1_d) == TimingPathCategory::IN2REG);
    REQUIRE(tatum::classify_timing_path(tg, circuit.ff1_q, circuit.ff2_d) == TimingPathCategory::REG2REG);
    REQUIRE(tatum::classify_timing_path(tg, circuit.ff2_q, circuit.out_x) == TimingPathCategory::REG2OUT);
    REQUIRE(tatum::classify_timing_path(tg, circuit.in_b, circuit.out_y) == TimingPathCategory::IN2OUT);

    // All paths are collected by default
    REQUIRE(circuit.setup_paths(TimingPathCategoryFilter()).size() == 5);
    REQUIRE(circuit.hold_paths(TimingPathCategoryFilter()).size() == 5);
}

TEST_CASE("timing_path_category_collect_setup", "[tatum]") {
    CategoryTestCircuit circuit;

    std::vector<TimingPath> reg2reg = circuit.setup_paths({TimingPathCategory::REG2REG});
    REQUIRE(reg2reg.size() == 1);
    REQUIRE(reg2reg[0].path_info().startpoint() == circuit.ff1_q);
    REQUIRE(reg2reg[0].path_info().endpoint() == circuit.ff2_d);

    // ff3_d's worst setup path starts at in_c, so it is only reported as in2reg
    std::vector<TimingPath> in2reg = circuit.setup_paths({TimingPathCategory::IN2REG});
    REQUIRE(in2reg.size() == 2);
    REQUIRE(in2reg[0].path_info().startpoint() == circuit.in_c);
    REQUIRE(in2reg[0].path_info().endpoint() == circuit.ff3_d);
    REQUIRE(in2reg[1].path_info().startpoint() == circuit.in_a);

    std::vector<TimingPath> reg2out = circuit.setup_paths({TimingPathCategory::REG2OUT});
    REQUIRE(reg2out.size() == 1);
    REQUIRE(reg2out[0].path_info().endpoint() == circuit.out_x);

    std::vector<TimingPath> in2out = circuit.setup_paths({TimingPathCategory::IN2OUT});
    REQUIRE(in2out.size() == 1);
    REQUIRE(in2out[0].path_info().endpoint() == circuit.out_y);

    std::vector<TimingPath> ending_at_registers = circuit.setup_paths({TimingPathCategory::IN2REG, TimingPathCategory::REG2REG});
    REQUIRE(ending_at_registers.size() == 3);
}

TEST_CASE("timing_path_category_filter_before_npaths", "[tatum]") {
    CategoryTestCircuit circuit;

    // The reg2reg path has the most slack, so it is the last of all paths...
    std::vector<TimingPath> all = circuit.setup_paths(TimingPathCategoryFilter());
    REQUIRE(all.back().path_info().endpoint() == circuit.ff2_d);

    // ...but must still be found when only the worst path is requested
    std::vector<TimingPath> reg2reg = circuit.setup_paths({TimingPathCategory::REG2REG}, /*npaths=*/1);
    REQUIRE(reg2reg.size() == 1);
    REQUIRE(reg2reg[0].path_info().endpoint() == circuit.ff2_d);
}

TEST_CASE("timing_path_category_collect_hold", "[tatum]") {
    CategoryTestCircuit circuit;

    // ff3_d's worst (fastest) hold path starts at ff1_q, so it is a reg2reg path for hold
    std::vector<TimingPath> reg2reg = circuit.hold_paths({TimingPathCategory::REG2REG});
    REQUIRE(reg2reg.size() == 2);
    for (const TimingPath& path : reg2reg) {
        REQUIRE(path.path_info().startpoint() == circuit.ff1_q);
    }

    std::vector<TimingPath> in2reg = circuit.hold_paths({TimingPathCategory::IN2REG});
    REQUIRE(in2reg.size() == 1);
    REQUIRE(in2reg[0].path_info().startpoint() == circuit.in_a);
}

TEST_CASE("timing_path_category_collect_skew", "[tatum]") {
    CategoryTestCircuit circuit;

    std::vector<SkewPath> all = circuit.setup_skew_paths(TimingPathCategoryFilter());
    REQUIRE(all.size() == 5);

    std::vector<SkewPath> reg2reg = circuit.setup_skew_paths({TimingPathCategory::REG2REG});
    REQUIRE(reg2reg.size() == 1);
    REQUIRE(reg2reg[0].data_launch_node == circuit.ff1_q);
    REQUIRE(reg2reg[0].data_capture_node == circuit.ff2_d);
    REQUIRE(tatum::classify_skew_path(circuit.tg, reg2reg[0]) == TimingPathCategory::REG2REG);

    std::vector<SkewPath> io = circuit.setup_skew_paths({TimingPathCategory::IN2REG, TimingPathCategory::REG2OUT, TimingPathCategory::IN2OUT});
    REQUIRE(io.size() == 4);
    for (const SkewPath& path : io) {
        REQUIRE(tatum::classify_skew_path(circuit.tg, path) != TimingPathCategory::REG2REG);
    }
}

TEST_CASE("timing_path_category_report", "[tatum]") {
    using Catch::Matchers::ContainsSubstring;

    CategoryTestCircuit circuit;
    tatum::NodeNumResolver resolver(circuit.tg, *circuit.delay_calc, /*verbose=*/false);
    tatum::TimingReporter reporter(resolver, circuit.tg, circuit.tc);

    std::stringstream all_report;
    reporter.report_timing_setup(all_report, *circuit.analyzer);
    REQUIRE_THAT(all_report.str(), ContainsSubstring("#Timing report of worst 5 path(s)"));
    REQUIRE_THAT(all_report.str(), ContainsSubstring("Category  : reg2out"));
    REQUIRE_THAT(all_report.str(), !ContainsSubstring("# Path categories"));

    std::stringstream reg2reg_report;
    reporter.report_timing_setup(reg2reg_report, *circuit.analyzer, 100, {TimingPathCategory::REG2REG});
    REQUIRE_THAT(reg2reg_report.str(), ContainsSubstring("#Timing report of worst 1 path(s)"));
    REQUIRE_THAT(reg2reg_report.str(), ContainsSubstring("# Path categories: reg2reg"));
    REQUIRE_THAT(reg2reg_report.str(), ContainsSubstring("Category  : reg2reg"));
    REQUIRE_THAT(reg2reg_report.str(), !ContainsSubstring("Category  : in2reg"));

    std::stringstream skew_report;
    reporter.report_skew_hold(skew_report, *circuit.analyzer, 100, {TimingPathCategory::REG2REG});
    REQUIRE_THAT(skew_report.str(), ContainsSubstring("# Path categories: reg2reg"));
    REQUIRE_THAT(skew_report.str(), ContainsSubstring("Category  : reg2reg"));
    REQUIRE_THAT(skew_report.str(), !ContainsSubstring("Category  : in2reg"));
}

} // namespace
