/**
 * @file
 * @brief Implementation of timing path categories and the timing path category filter.
 */

#include "tatum/report/TimingPathCategory.hpp"

#include <ostream>
#include <sstream>
#include <stdexcept>

#include "tatum/TimingGraph.hpp"
#include "tatum/report/TimingPath.hpp"
#include "tatum/report/SkewPath.hpp"
#include "tatum/util/tatum_assert.hpp"

namespace tatum {

std::ostream& operator<<(std::ostream& os, const TimingPathCategory category) {
    switch (category) {
        case TimingPathCategory::IN2REG:
            os << "in2reg";
            break;
        case TimingPathCategory::REG2REG:
            os << "reg2reg";
            break;
        case TimingPathCategory::REG2OUT:
            os << "reg2out";
            break;
        case TimingPathCategory::IN2OUT:
            os << "in2out";
            break;
        default:
            throw std::domain_error("Unrecognized TimingPathCategory");
    }
    return os;
}

std::string to_string(const TimingPathCategory category) {
    std::stringstream ss;
    ss << category;
    return ss.str();
}

bool is_register_startpoint(const TimingGraph& timing_graph, const NodeId startpoint) {
    TATUM_ASSERT(timing_graph.node_type(startpoint) == NodeType::SOURCE);
    return bool(timing_graph.node_clock_launch_edge(startpoint));
}

bool is_register_endpoint(const TimingGraph& timing_graph, const NodeId endpoint) {
    TATUM_ASSERT(timing_graph.node_type(endpoint) == NodeType::SINK);
    return bool(timing_graph.node_clock_capture_edge(endpoint));
}

TimingPathCategory make_timing_path_category(bool start_is_register, bool end_is_register) {
    if (start_is_register) {
        return end_is_register ? TimingPathCategory::REG2REG : TimingPathCategory::REG2OUT;
    } else {
        return end_is_register ? TimingPathCategory::IN2REG : TimingPathCategory::IN2OUT;
    }
}

TimingPathCategory classify_timing_path(const TimingGraph& timing_graph, const NodeId startpoint, const NodeId endpoint) {
    return make_timing_path_category(is_register_startpoint(timing_graph, startpoint),
                                     is_register_endpoint(timing_graph, endpoint));
}

TimingPathCategory classify_timing_path(const TimingGraph& timing_graph, const TimingPathInfo& path_info) {
    TATUM_ASSERT_MSG(path_info.startpoint(), "Timing path must be fully traced to be classified");
    return classify_timing_path(timing_graph, path_info.startpoint(), path_info.endpoint());
}

TimingPathCategory classify_skew_path(const TimingGraph& timing_graph, const SkewPath& skew_path) {
    return classify_timing_path(timing_graph, skew_path.data_launch_node, skew_path.data_capture_node);
}

TimingPathCategoryFilter::TimingPathCategoryFilter()
    : TimingPathCategoryFilter({TimingPathCategory::IN2REG,
                                TimingPathCategory::REG2REG,
                                TimingPathCategory::REG2OUT,
                                TimingPathCategory::IN2OUT}) {}

TimingPathCategoryFilter::TimingPathCategoryFilter(std::initializer_list<TimingPathCategory> categories) {
    for (TimingPathCategory category : categories) {
        add(category);
    }
}

TimingPathCategoryFilter TimingPathCategoryFilter::all() {
    return TimingPathCategoryFilter();
}

TimingPathCategoryFilter TimingPathCategoryFilter::none() {
    return TimingPathCategoryFilter(std::initializer_list<TimingPathCategory>());
}

bool TimingPathCategoryFilter::contains(const TimingPathCategory category) const {
    return categories_ & category_bit_(category);
}

bool TimingPathCategoryFilter::accepts_all() const {
    return categories_ == TimingPathCategoryFilter::all().categories_;
}

bool TimingPathCategoryFilter::accepts_endpoint(bool end_is_register) const {
    return contains(make_timing_path_category(/*start_is_register=*/false, end_is_register))
           || contains(make_timing_path_category(/*start_is_register=*/true, end_is_register));
}

std::string TimingPathCategoryFilter::to_string() const {
    std::stringstream ss;
    bool first = true;
    for (size_t i = 0; i < size_t(TimingPathCategory::NUM_CATEGORIES); ++i) {
        TimingPathCategory category = TimingPathCategory(i);
        if (!contains(category))
            continue;

        if (!first)
            ss << ", ";
        ss << category;
        first = false;
    }
    return ss.str();
}

TimingPathCategoryFilter& TimingPathCategoryFilter::add(const TimingPathCategory category) {
    categories_ |= category_bit_(category);
    return *this;
}

TimingPathCategoryFilter& TimingPathCategoryFilter::remove(const TimingPathCategory category) {
    categories_ &= ~category_bit_(category);
    return *this;
}

unsigned char TimingPathCategoryFilter::category_bit_(const TimingPathCategory category) {
    TATUM_ASSERT(category != TimingPathCategory::NUM_CATEGORIES);
    return 1 << size_t(category);
}

} // namespace tatum
